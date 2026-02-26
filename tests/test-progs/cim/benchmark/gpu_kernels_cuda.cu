// CUDA GPU Kernels for Row Operations
// Implements all 12 row operations for NVIDIA GPUs

#include <cuda_runtime.h>
#include <cstdint>

using dtype = int16_t;

__global__ void gpu_rowand_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] & src2[idx];
    }
}

__global__ void gpu_rowadd_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] + src2[idx];
    }
}

__global__ void gpu_rowsub_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] - src2[idx];
    }
}

__global__ void gpu_rowmult_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] * src2[idx];
    }
}

__global__ void gpu_rowmin_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] < src2[idx] ? src1[idx] : src2[idx];
    }
}

__global__ void gpu_rowmax_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] > src2[idx] ? src1[idx] : src2[idx];
    }
}

__global__ void gpu_rowequal_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = (src1[idx] == src2[idx]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

__global__ void gpu_rowgreater_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = (src1[idx] > src2[idx]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

__global__ void gpu_rowgreater_equal_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = (src1[idx] >= src2[idx]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

__global__ void gpu_rowif_else_kernel(dtype* dst, const dtype* src1, const dtype* src2, const dtype* mask, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = (mask[idx] != 0) ? src1[idx] : src2[idx];
    }
}

__global__ void gpu_rowabs_kernel(dtype* dst, const dtype* src, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dtype val = src[idx];
        dst[idx] = val < 0 ? -val : val;
    }
}

__global__ void gpu_rowbitcount_kernel(dtype* dst, const dtype* src, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        unsigned int count = 0;
        unsigned int val = static_cast<unsigned int>(src[idx]);
        while (val) {
            count += val & 1;
            val >>= 1;
        }
        dst[idx] = static_cast<dtype>(count);
    }
}

extern "C" {

void gpu_rowand(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowand_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowadd(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowadd_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowsub(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowsub_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowmult(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowmult_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowmin(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowmin_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowmax(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowmax_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowequal(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowequal_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowgreater(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowgreater_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowgreater_equal(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowgreater_equal_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, size);
}

void gpu_rowif_else(dtype* dst, const dtype* src1, const dtype* src2, const dtype* mask, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowif_else_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src1, src2, mask, size);
}

void gpu_rowabs(dtype* dst, const dtype* src, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowabs_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src, size);
}

void gpu_rowbitcount(dtype* dst, const dtype* src, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    gpu_rowbitcount_kernel<<<blocksPerGrid, threadsPerBlock>>>(dst, src, size);
}

}
