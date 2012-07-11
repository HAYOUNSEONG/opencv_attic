/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or bpied warranties, including, but not limited to, the bpied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef __OPENCV_GPU_DEVICE_LBP_HPP_
#define __OPENCV_GPU_DEVICE_LBP_HPP_

#include "internal_shared.hpp"

namespace cv { namespace gpu { namespace device {

namespace lbp{

    #define TAG_MASK ( (1U << ( (sizeof(unsigned int) << 3) - 5U)) - 1U )
template<typename T>
__device__ __forceinline__ T __atomicInc(T* address, T val)
{
    T count;
    unsigned int tag = threadIdx.x << ( (sizeof(unsigned int) << 3) - 5U);
    do
    {
        count = *address & TAG_MASK;
        count = tag | (count + 1);
        *address = count;
    } while (*address != count);

    return (count & TAG_MASK) - 1;
}

template<typename T>
__device__ __forceinline__ void __atomicAdd(T* address, T val)
{
    T count;
    unsigned int tag = threadIdx.x << ( (sizeof(unsigned int) << 3) - 5U);
    do
    {
        count = *address & TAG_MASK;
        count = tag | (count + val);
        *address = count;
    } while (*address != count);
}

template<typename T>
__device__ __forceinline__ T __atomicMin(T* address, T val)
{
    T count = min(*address, val);
    do
    {
        *address = count;
    } while (*address > count);

    return count;
}

    struct Stage
    {
        int    first;
        int    ntrees;
        float  threshold;
    };

    struct ClNode
    {
        int   left;
        int   right;
        int   featureIdx;
    };

    struct InSameComponint
    {
    public:
        __device__ __forceinline__ InSameComponint(float _eps) : eps(_eps) {}
        __device__ __forceinline__ InSameComponint(const InSameComponint& other) : eps(other.eps) {}

        __device__ __forceinline__ bool operator()(const int4& r1, const int4& r2) const
        {
            float delta = eps * (min(r1.z, r2.z) + min(r1.w, r2.w)) * 0.5;

            return abs(r1.x - r2.x) <= delta && abs(r1.y - r2.y) <= delta
                && abs(r1.x + r1.z - r2.x - r2.z) <= delta && abs(r1.y + r1.w - r2.y - r2.w) <= delta;
        }
        float eps;
    };

    template<typename Pr>
    __device__ __forceinline__ void partition(int4* vec, unsigned int n, int* labels, Pr predicate)
    {
        unsigned tid = threadIdx.x;
        labels[tid] = tid;
        __syncthreads();

        for (unsigned int id = 0; id < n; id++)
        {
            if (tid != id && predicate(vec[tid], vec[id]))
            {
                int p = labels[tid];
                int q = labels[id];

                if (p < q)
                {
#if defined (__CUDA_ARCH__) && (__CUDA_ARCH__ < 120)
                    __atomicMin(labels + id, p);
#else
                    atomicMin(labels + id, p);
#endif
                }
                else if (p > q)
                {
#if defined (__CUDA_ARCH__) && (__CUDA_ARCH__ < 120)
                    __atomicMin(labels + tid, q);
#else
                    atomicMin(labels + tid, q);
#endif
                }
            }
        }
        __syncthreads();
    }
} // lbp

} } }// namespaces

#endif