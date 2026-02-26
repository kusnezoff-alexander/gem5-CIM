/**
 * CPU-only KNN demo for Iris dataset
 *
 * This is a pure CPU implementation using standard C++.
 * No PIM or GPU calls - just regular CPU computation.
 */

#include <array>
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
#include <cmath>

using namespace std;

constexpr size_t N_FEATURES = 4; // Iris features: sepal/petal length/width
constexpr size_t K = 3;          // number of neighbors

struct Dataset {
    // AoS layout: each sample has all features in a struct
    vector<array<int32_t, N_FEATURES>> features;
    vector<int> labels;
    size_t n_samples = 0;
};

// Map labels
map<string,int> label_map = {{"Iris-setosa",0},{"Iris-versicolor",1},{"Iris-virginica",2}};

Dataset load_iris_csv(const string& path) {
    ifstream fin(path);
    if (!fin.is_open()) {
        cerr << "Cannot open " << path << "\n";
        exit(1);
    }

    // First pass: count samples
    string line;
    getline(fin, line); // skip header
    size_t n_samples = 0;
    while (getline(fin, line)) {
        if (!line.empty()) n_samples++;
    }

    Dataset ds;
    ds.n_samples = n_samples;
    ds.features.resize(n_samples);
    ds.labels.resize(n_samples);

    // Second pass: load data
    fin.clear();
    fin.seekg(0);
    getline(fin, line); // skip header again

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

        // Store features
        for (size_t f = 0; f < N_FEATURES; ++f) {
            ds.features[sample_idx][f] = static_cast<int32_t>(stoi(tokens[f + 1]));
        }

        string label_str = tokens.back();
        if (label_map.find(label_str) == label_map.end()) {
            cerr << "Unknown label: " << label_str << "\n";
            exit(1);
        }

        ds.labels[sample_idx] = label_map[label_str];
        sample_idx++;
    }

    cout << "Loaded dataset with " << n_samples << " samples (CPU layout)\n";
    return ds;
}


int knn_predict(const Dataset& train, const array<int32_t, N_FEATURES>& query) {
    const size_t n = train.n_samples;

    // Compute distances using CPU
    vector<pair<int32_t, int>> dist_label(n);
    for (size_t i = 0; i < n; ++i) {
        int32_t dist_sq = 0;
        for (size_t f = 0; f < N_FEATURES; ++f) {
            int32_t diff = train.features[i][f] - query[f];
            dist_sq += diff * diff;
        }
        dist_label[i] = {dist_sq, train.labels[i]};
    }

    // Find K nearest
    nth_element(dist_label.begin(), dist_label.begin() + K, dist_label.end());

    // Majority vote
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

int main() {
    cout << "Running CPU-only KNN on Iris dataset\n";

    Dataset train = load_iris_csv("tests/test-progs/cim/data/iris/train.csv");
    Dataset test  = load_iris_csv("tests/test-progs/cim/data/iris/test.csv");

    cout << "Loaded training set: " << train.n_samples << " samples\n";
    cout << "Loaded test set:     " << test.n_samples << " samples\n";

    size_t correct = 0;
    for (size_t i = 0; i < test.n_samples; ++i) {
        int pred = knn_predict(train, test.features[i]);
        if (pred == test.labels[i]) correct++;
    }

    float accuracy = 100.0f * correct / test.n_samples;
    cout << "Accuracy: " << accuracy << "% ("
        << correct << "/" << test.n_samples << ")\n";

    return 0;
}
