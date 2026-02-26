/**
 * GPU-only KNN demo for Iris dataset using CUDA
 *
 * This is a pure GPU implementation using CUDA kernels.
 * No PIM or CPU computation - all on GPU.
 */

#include <cuda_runtime.h>
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

using namespace std;

constexpr size_t N_FEATURES = 4;
constexpr size_t K = 3;

#define CUDA_CHECK(cmd) \
    do { \
        cudaError_t error = cmd; \
        if (error != cudaSuccess) { \
            fprintf(stderr, "CUDA error: '%s' (%d) at %s:%d\n", \
                    cudaGetErrorString(error), error, __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

struct Dataset {
    vector<int32_t> features_flat;
    vector<int> labels;
    size_t n_samples = 0;
};

__global__ void compute_distances_kernel(
    int32_t* __restrict__ distances,
    const int32_t* __restrict__ train_features,
    const int32_t* __restrict__ query_features,
    size_t n_samples,
    size_t n_features
) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_samples) return;

    int32_t dist_sq = 0;
    for (size_t f = 0; f < n_features; ++f) {
        int32_t diff = train_features[idx * n_features + f] - query_features[f];
        dist_sq += diff * diff;
    }
    distances[idx] = dist_sq;
}

__global__ void find_top_k_kernel(
    int32_t* __restrict__ top_k_distances,
    int* __restrict__ top_k_indices,
    const int32_t* __restrict__ distances,
    size_t n_samples,
    size_t k
) {
    for (size_t i = 0; i < k; ++i) {
        top_k_distances[i] = distances[i];
        top_k_indices[i] = i;
    }
    
    for (size_t i = k; i < n_samples; ++i) {
        size_t max_idx = 0;
        for (size_t j = 1; j < k; ++j) {
            if (top_k_distances[j] > top_k_distances[max_idx]) {
                max_idx = j;
            }
        }
        
        if (distances[i] < top_k_distances[max_idx]) {
            top_k_distances[max_idx] = distances[i];
            top_k_indices[max_idx] = i;
        }
    }
}

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
    ds.features_flat.resize(n_samples * N_FEATURES);
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
            ds.features_flat[sample_idx * N_FEATURES + f] = static_cast<int32_t>(stoi(tokens[f + 1]));
        }

        string label_str = tokens.back();
        if (label_map.find(label_str) == label_map.end()) {
            cerr << "Unknown label: " << label_str << "\n";
            exit(1);
        }

        ds.labels[sample_idx] = label_map[label_str];
        sample_idx++;
    }

    cout << "Loaded dataset with " << n_samples << " samples (GPU layout)\n";
    return ds;
}

int knn_predict_gpu(const Dataset& train, const int32_t* query_features_device) {
    const size_t n = train.n_samples;

    int32_t* d_train_features;
    int32_t* d_distances;
    int32_t* d_top_k_distances;
    int* d_top_k_indices;

    CUDA_CHECK(cudaMalloc(&d_train_features, train.features_flat.size() * sizeof(int32_t)));
    CUDA_CHECK(cudaMalloc(&d_distances, n * sizeof(int32_t)));
    CUDA_CHECK(cudaMalloc(&d_top_k_distances, K * sizeof(int32_t)));
    CUDA_CHECK(cudaMalloc(&d_top_k_indices, K * sizeof(int)));

    CUDA_CHECK(cudaMemcpy(d_train_features, train.features_flat.data(), 
                        train.features_flat.size() * sizeof(int32_t), cudaMemcpyHostToDevice));

    int blockSize = 256;
    int gridSize = (n + blockSize - 1) / blockSize;
    compute_distances_kernel<<<gridSize, blockSize>>>(
        d_distances, d_train_features, query_features_device, n, N_FEATURES);
    CUDA_CHECK(cudaDeviceSynchronize());

    find_top_k_kernel<<<1, 1>>>(d_top_k_distances, d_top_k_indices, d_distances, n, K);
    CUDA_CHECK(cudaDeviceSynchronize());

    vector<int> top_k_indices(K);
    CUDA_CHECK(cudaMemcpy(top_k_indices.data(), d_top_k_indices, K * sizeof(int), cudaMemcpyDeviceToHost));

    map<int, int> votes;
    for (size_t i = 0; i < K; ++i) {
        votes[train.labels[top_k_indices[i]]]++;
    }

    int predicted_label = -1;
    int max_votes = 0;
    for (auto& kv : votes) {
        if (kv.second > max_votes) {
            max_votes = kv.second;
            predicted_label = kv.first;
        }
    }

    CUDA_CHECK(cudaFree(d_train_features));
    CUDA_CHECK(cudaFree(d_distances));
    CUDA_CHECK(cudaFree(d_top_k_distances));
    CUDA_CHECK(cudaFree(d_top_k_indices));

    return predicted_label;
}

int main() {
    cout << "Running GPU-only KNN on Iris dataset (CUDA)\n";

    Dataset train = load_iris_csv("tests/test-progs/cim/data/iris/train.csv");
    Dataset test  = load_iris_csv("tests/test-progs/cim/data/iris/test.csv");

    cout << "Loaded training set: " << train.n_samples << " samples\n";
    cout << "Loaded test set:     " << test.n_samples << " samples\n";

    int32_t* d_query;
    CUDA_CHECK(cudaMalloc(&d_query, N_FEATURES * sizeof(int32_t)));

    size_t correct = 0;
    for (size_t i = 0; i < test.n_samples; ++i) {
        CUDA_CHECK(cudaMemcpy(d_query, &test.features_flat[i * N_FEATURES], 
                            N_FEATURES * sizeof(int32_t), cudaMemcpyHostToDevice));

        int pred = knn_predict_gpu(train, d_query);
        if (pred == test.labels[i]) correct++;
    }

    float accuracy = 100.0f * correct / test.n_samples;
    cout << "Accuracy: " << accuracy << "% ("
        << correct << "/" << test.n_samples << ")\n";

    CUDA_CHECK(cudaFree(d_query));

    return 0;
}
