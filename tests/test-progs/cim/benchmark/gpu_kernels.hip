// HIP GPU Kernels for Row Operations
// Implements all 13 row operations for gem5 GPU simulation

#include <hip/hip_runtime.h>
#include <cstdint>

using dtype = int16_t;

// 1. Row AND operation
__global__ void gpu_rowand_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] & src2[idx];
    }
}

// 2. Row ADD operation
__global__ void gpu_rowadd_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] + src2[idx];
    }
}

// 3. Row SUB operation
__global__ void gpu_rowsub_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] - src2[idx];
    }
}

// 4. Row MULT operation
__global__ void gpu_rowmult_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] * src2[idx];
    }
}

// 5. Row DIV operation
__global__ void gpu_rowdiv_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src2[idx] != 0 ? src1[idx] / src2[idx] : 0;
    }
}

// 6. Row MIN operation
__global__ void gpu_rowmin_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] < src2[idx] ? src1[idx] : src2[idx];
    }
}

// 7. Row MAX operation
__global__ void gpu_rowmax_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = src1[idx] > src2[idx] ? src1[idx] : src2[idx];
    }
}

// 8. Row EQUAL operation
__global__ void gpu_rowequal_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = (src1[idx] == src2[idx]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

// 9. Row GREATER operation
__global__ void gpu_rowgreater_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = (src1[idx] > src2[idx]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

// 10. Row GREATER_EQUAL operation
__global__ void gpu_rowgreater_equal_kernel(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = (src1[idx] >= src2[idx]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

// 11. Row IF_ELSE operation (ternary select)
__global__ void gpu_rowif_else_kernel(dtype* dst, const dtype* src1, const dtype* src2, const dtype* mask, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dst[idx] = (mask[idx] != 0) ? src1[idx] : src2[idx];
    }
}

// 12. Row ABS operation (unary)
__global__ void gpu_rowabs_kernel(dtype* dst, const dtype* src, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        dtype val = src[idx];
        dst[idx] = val < 0 ? -val : val;
    }
}

// 13. Row BITCOUNT operation (population count)
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

// Host wrapper functions for kernel launches
extern "C" {

void gpu_rowand(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowand_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowadd(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowadd_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowsub(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowsub_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowmult(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowmult_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowdiv(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowdiv_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowmin(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowmin_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowmax(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowmax_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowequal(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowequal_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowgreater(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowgreater_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowgreater_equal(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowgreater_equal_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, size);
}

void gpu_rowif_else(dtype* dst, const dtype* src1, const dtype* src2, const dtype* mask, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowif_else_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src1, src2, mask, size);
}

void gpu_rowabs(dtype* dst, const dtype* src, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowabs_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src, size);
}

void gpu_rowbitcount(dtype* dst, const dtype* src, size_t size) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    hipLaunchKernelGGL(gpu_rowbitcount_kernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 
                       0, 0, dst, src, size);
}

} // extern "C"
