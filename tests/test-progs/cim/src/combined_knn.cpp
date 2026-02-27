#ifndef N_RUNS
#define N_RUNS 10
#endif

#ifndef RUN_CHECKS
#define RUN_CHECKS 1
#endif

#include "pim_core.h"
#include <gem5/m5ops.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <random>

using namespace pim_core;
using namespace std;

template<typename T>
T* pim_alloc_safe(size_t size_bytes, size_t& next_subarray) {
    T* ptr = nullptr;

    do {
        ptr = static_cast<T*>(pim_malloc(size_bytes, next_subarray));
        if (ptr != nullptr) break;
        next_subarray++;
    } while (next_subarray < NR_SUBARRAYS);

    if (ptr == nullptr) {
        cerr << "ERROR: not enough PIM space for "
             << size_bytes << " bytes\n";
        exit(1);
    }

    return ptr;
}

constexpr size_t N_FEATURES = 4;
constexpr size_t K = 3;

size_t next_subarray = 0;

struct Dataset {
    int32_t* features[N_FEATURES];
    vector<int> labels;
    size_t n_samples = 0;
};

struct KNNWorkspace {
    int32_t* distances;
    int32_t* diff;
    int32_t* diff_sq;
    int32_t* query_features[N_FEATURES];

    KNNWorkspace(size_t n_samples) {
        distances = pim_alloc_safe<int32_t>(n_samples * sizeof(int32_t), next_subarray);
        diff      = pim_alloc_safe<int32_t>(n_samples * sizeof(int32_t), next_subarray);
        diff_sq   = pim_alloc_safe<int32_t>(n_samples * sizeof(int32_t), next_subarray);

        for (size_t f = 0; f < N_FEATURES; ++f) {
            query_features[f] = pim_alloc_safe<int32_t>(n_samples * sizeof(int32_t), next_subarray);
        }
    }

    ~KNNWorkspace() {
        pim_free(distances);
        pim_free(diff);
        pim_free(diff_sq);
        for (size_t f = 0; f < N_FEATURES; ++f) {
            pim_free(query_features[f]);
        }
    }
};

map<string,int> label_map = {{"Iris-setosa",0},{"Iris-versicolor",1},{"Iris-virginica",2}};

Dataset load_iris_csv(const string& path) {
    ifstream fin(path);
    if (!fin.is_open()) {
        cerr << "Cannot open " << path << "\n";
        exit(1);
    }

    string line;
    getline(fin, line);
    size_t n_samples = 0;
    while (getline(fin, line)) {
        if (!line.empty()) n_samples++;
    }

    Dataset ds;
    ds.n_samples = n_samples;

    for (size_t f = 0; f < N_FEATURES; ++f) {
        ds.features[f] = pim_alloc_safe<int32_t>(n_samples * sizeof(int32_t), next_subarray);
    }
    ds.labels.resize(n_samples);

    fin.clear();
    fin.seekg(0);
    getline(fin, line);

    size_t sample_idx = 0;
    while (getline(fin, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string token;
        vector<string> tokens;

        while (getline(ss, token, ',')) {
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            tokens.push_back(token);
        }

        if (tokens.size() < N_FEATURES + 2) continue;

        for (size_t f = 0; f < N_FEATURES; ++f) {
            ds.features[f][sample_idx] = static_cast<int32_t>(stoi(tokens[f + 1]));
        }

        string label_str = tokens.back();
        if (label_map.find(label_str) == label_map.end()) {
            cerr << "Unknown label: " << label_str << "\n";
            exit(1);
        }

        ds.labels[sample_idx] = label_map[label_str];
        sample_idx++;
    }

    cout << "Loaded dataset with " << n_samples << " samples in SoA layout\n";
    return ds;
}


int knn_predict(const Dataset& train, const int32_t* query, KNNWorkspace& ws) {
    const size_t n = train.n_samples;

    for (size_t f = 0; f < N_FEATURES; ++f) {
        for (size_t i = 0; i < n; ++i) {
            ws.query_features[f][i] = query[f];
        }
    }

    for (size_t i = 0; i < n; ++i)
        ws.distances[i] = 0;

    for (size_t f = 0; f < N_FEATURES; ++f) {
        rowsub(ws.diff, train.features[f], ws.query_features[f], n, 32);
        rowmult(ws.diff_sq, ws.diff, ws.diff, n, 32);
        rowadd(ws.distances, ws.distances, ws.diff_sq, n, 32);
    }

    vector<pair<int32_t,int>> dist_label(n);
    for (size_t i = 0; i < n; ++i)
        dist_label[i] = {ws.distances[i], train.labels[i]};

    nth_element(dist_label.begin(), dist_label.begin() + K, dist_label.end());

    map<int,int> votes;
    for (size_t i = 0; i < K; ++i) {
        votes[dist_label[i].second]++;
    }

    int predicted_label = -1;
    int max_votes = 0;
    for (auto& kv : votes) {
        if (kv.second > max_votes) {
            max_votes = kv.second;
            predicted_label = kv.first;
        }
    }

    return predicted_label;
}

bool test_knn_run(int run_id)
{
    Dataset train = load_iris_csv("tests/test-progs/cim/data/iris/train.csv");
    Dataset test  = load_iris_csv("tests/test-progs/cim/data/iris/test.csv");

    cout << "Loaded training set: " << train.n_samples << " samples\n";
    cout << "Loaded test set:     " << test.n_samples << " samples\n";

    KNNWorkspace workspace(train.n_samples);

    m5_reset_stats(0, 0);

    size_t correct = 0;
    for (size_t i = 0; i < test.n_samples; ++i) {
        int32_t query[N_FEATURES];
        for (size_t f = 0; f < N_FEATURES; ++f) {
            query[f] = test.features[f][i];
        }

        int pred = knn_predict(train, query, workspace);
        if (pred == test.labels[i]) correct++;
    }

    m5_dump_stats(0, 0);

#if RUN_CHECKS
    float accuracy = 100.0f * correct / test.n_samples;
    cout << "Run " << run_id << " Accuracy: " << accuracy << "% ("
        << correct << "/" << test.n_samples << ")\n";
#else
    cout << "Run " << run_id << " Skipped accuracy check (RUN_CHECKS=0)\n";
#endif

    for (size_t f = 0; f < N_FEATURES; ++f) {
        pim_free(train.features[f]);
        pim_free(test.features[f]);
    }

    return accuracy > 0;
}

int main() {
    cout << "Running PIM + CPU KNN on Iris dataset (SoA layout), " << N_RUNS << " runs\n";

    int passed_runs = 0;
    for (int run = 0; run < N_RUNS; run++) {
        printf("\n=== Run %d/%d ===\n", run + 1, N_RUNS);

        bool result = test_knn_run(run + 1);
        if (result) {
            passed_runs++;
        }
    }

    printf("\n=== Summary: %d/%d runs passed ===\n", passed_runs, N_RUNS);
    if (passed_runs == N_RUNS) {
        return 0;
    } else {
        return 1;
    }
}
