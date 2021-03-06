/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "paddle/platform/device_context.h"
#include "paddle/memory/memory.h"

namespace paddle {
namespace platform {

template <>
Eigen::DefaultDevice* DeviceContext::get_eigen_device<Eigen::DefaultDevice>()
    const {
  return reinterpret_cast<const CPUDeviceContext*>(this)->eigen_device();
}

CPUDeviceContext::CPUDeviceContext() {
  eigen_device_.reset(new Eigen::DefaultDevice());
}

CPUDeviceContext::CPUDeviceContext(CPUPlace place) {
  eigen_device_.reset(new Eigen::DefaultDevice());
}

Eigen::DefaultDevice* CPUDeviceContext::eigen_device() const {
  return eigen_device_.get();
}

Place CPUDeviceContext::GetPlace() const { return CPUPlace(); }

#ifndef PADDLE_ONLY_CPU

class EigenCudaStreamDevice : public Eigen::StreamInterface {
 public:
  EigenCudaStreamDevice() : scratch_(nullptr), semaphore_(nullptr) {
    Eigen::initializeDeviceProp();
  }
  ~EigenCudaStreamDevice() override {}

  void Reinitialize(const cudaStream_t* cuda_stream, GPUPlace place) {
    stream_ = cuda_stream;
    place_ = place;
    device_prop_ = &Eigen::m_deviceProperties[place.device];
  }

  const cudaStream_t& stream() const override { return *stream_; }

  const cudaDeviceProp& deviceProperties() const override {
    return *device_prop_;
  }

  void* allocate(size_t num_bytes) const override {
    return paddle::memory::Alloc(place_, num_bytes);
  }

  void deallocate(void* buffer) const override {
    paddle::memory::Free(place_, buffer);
  }

  void* scratchpad() const override {
    if (scratch_ == NULL) {
      scratch_ = allocate(Eigen::kCudaScratchSize + sizeof(unsigned int));
    }
    return scratch_;
  }

  unsigned int* semaphore() const override {
    if (semaphore_ == NULL) {
      char* scratch =
          static_cast<char*>(scratchpad()) + Eigen::kCudaScratchSize;
      semaphore_ = reinterpret_cast<unsigned int*>(scratch);
      PADDLE_ENFORCE(
          cudaMemsetAsync(semaphore_, 0, sizeof(unsigned int), *stream_));
    }
    return semaphore_;
  }

 private:
  GPUPlace place_;
  const cudaStream_t* stream_;         // not owned;
  const cudaDeviceProp* device_prop_;  // not owned;
  mutable void* scratch_;
  mutable unsigned int* semaphore_;
};

template <>
Eigen::GpuDevice* DeviceContext::get_eigen_device<Eigen::GpuDevice>() const {
  return reinterpret_cast<const CUDADeviceContext*>(this)->eigen_device();
}

CUDADeviceContext::CUDADeviceContext(GPUPlace place) : place_(place) {
  SetDeviceId(place_.device);
  PADDLE_ENFORCE(cudaStreamCreate(&stream_));
  eigen_stream_.reset(new EigenCudaStreamDevice());
  eigen_stream_->Reinitialize(&stream_, place);
  eigen_device_.reset(new Eigen::GpuDevice(eigen_stream_.get()));
}

CUDADeviceContext::~CUDADeviceContext() {
  SetDeviceId(place_.device);
  Wait();
  if (cublas_handle_) {
    PADDLE_ENFORCE(dynload::cublasDestroy(cublas_handle_));
  }

  if (cudnn_handle_) {
    PADDLE_ENFORCE(dynload::cudnnDestroy(cudnn_handle_));
  }

  eigen_stream_.reset();
  eigen_device_.reset();
  PADDLE_ENFORCE(cudaStreamDestroy(stream_));
}

Place CUDADeviceContext::GetPlace() const { return place_; }

void CUDADeviceContext::Wait() const {
  PADDLE_ENFORCE(cudaStreamSynchronize(stream_));
}

Eigen::GpuDevice* CUDADeviceContext::eigen_device() const {
  return eigen_device_.get();
}

cublasHandle_t CUDADeviceContext::cublas_handle() {
  if (!cublas_handle_) {
    SetDeviceId(place_.device);
    PADDLE_ENFORCE(dynload::cublasCreate(&cublas_handle_));
    PADDLE_ENFORCE(dynload::cublasSetStream(cublas_handle_, stream_));
  }
  return cublas_handle_;
}

cudnnHandle_t CUDADeviceContext::cudnn_handle() {
  if (!cudnn_handle_) {
    SetDeviceId(place_.device);
    PADDLE_ENFORCE(dynload::cudnnCreate(&cudnn_handle_));
    PADDLE_ENFORCE(dynload::cudnnSetStream(cudnn_handle_, stream_));
  }
  return cudnn_handle_;
}

cudaStream_t CUDADeviceContext::stream() { return stream_; }

#endif  // PADDLE_ONLY_CPU

}  // namespace platform
}  // namespace paddle
