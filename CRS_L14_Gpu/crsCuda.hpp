//----------------------------------------------------------------------------

#ifndef CRS_CUDA_HPP
#define CRS_CUDA_HPP 1

#include "crsUtils.hpp"

#include <cuda.h>
#include <nvrtc.h>

#if !defined _WIN32
# include <nvml.h>
#endif

namespace crs {

class CudaDevice;

class CudaPlatform
{
public:

  CudaPlatform();

  CudaPlatform(CudaPlatform &&rhs) noexcept;

  CudaPlatform &
  operator=(CudaPlatform &&rhs) noexcept;

  ~CudaPlatform();

  int // number of available cuda devices
  deviceCount()
  {
    return deviceCount_;
  }

  const CudaDevice & // index'th cuda device
  device(int index);

  static
  std::string
  errorMessage(int errorCode);

private:
  CudaPlatform(const CudaPlatform &) =delete;
  CudaPlatform & operator=(const CudaPlatform &) =delete;

  static
  std::tuple<void *,      // host pointer
             CUdeviceptr> // device pointer
  allocLockedMem_(bool writeOnly,
                  intptr_t size);

  static
  void
  freeLockedMem_(void *hostPtr) noexcept;

  friend class CudaDevice;
  template<typename T> friend class CudaLockedMem;

  // private ctor/dtor --> std::make_unique() unusable
  CudaDevice *devices_;
  int deviceCount_;
};

//----------------------------------------------------------------------------

class CudaDevice
{
public:

  int
  id() const
  {
    return id_;
  }

  const std::string &
  name() const
  {
    return name_;
  }

  struct Properties
  {
    int64_t total_memory;
    int max_threads_per_block;
    int max_block_dim_x;
    int max_block_dim_y;
    int max_block_dim_z;
    int max_grid_dim_x;
    int max_grid_dim_y;
    int max_grid_dim_z;
    int max_shared_memory_per_block;
    int total_constant_memory;
    int warp_size;
    int max_pitch;
    int max_registers_per_block;
    int clock_rate_kHz;
    int texture_alignment;
    int gpu_overlap;
    int multiprocessor_count;
    int kernel_exec_timeout;
    int integrated;
    int can_map_host_memory;
    int compute_mode;
    int maximum_texture1d_width;
    int maximum_texture2d_width;
    int maximum_texture2d_height;
    int maximum_texture3d_width;
    int maximum_texture3d_height;
    int maximum_texture3d_depth;
    int maximum_texture2d_layered_width;
    int maximum_texture2d_layered_height;
    int maximum_texture2d_layered_layers;
    int surface_alignment;
    int concurrent_kernels;
    int ecc_enabled;
    int pci_bus_id;
    int pci_device_id;
    int tcc_driver;
    int memory_clock_rate_kHz;
    int global_memory_bus_width;
    int l2_cache_size;
    int max_threads_per_multiprocessor;
    int async_engine_count;
    int unified_addressing;
    int maximum_texture1d_layered_width;
    int maximum_texture1d_layered_layers;
    int maximum_texture2d_gather_width;
    int maximum_texture2d_gather_height;
    int maximum_texture3d_width_alternate;
    int maximum_texture3d_height_alternate;
    int maximum_texture3d_depth_alternate;
    int pci_domain_id;
    int texture_pitch_alignment;
    int maximum_texturecubemap_width;
    int maximum_texturecubemap_layered_width;
    int maximum_texturecubemap_layered_layers;
    int maximum_surface1d_width;
    int maximum_surface2d_width;
    int maximum_surface2d_height;
    int maximum_surface3d_width;
    int maximum_surface3d_height;
    int maximum_surface3d_depth;
    int maximum_surface1d_layered_width;
    int maximum_surface1d_layered_layers;
    int maximum_surface2d_layered_width;
    int maximum_surface2d_layered_height;
    int maximum_surface2d_layered_layers;
    int maximum_surfacecubemap_width;
    int maximum_surfacecubemap_layered_width;
    int maximum_surfacecubemap_layered_layers;
    int maximum_texture1d_linear_width;
    int maximum_texture2d_linear_width;
    int maximum_texture2d_linear_height;
    int maximum_texture2d_linear_pitch;
    int maximum_texture2d_mipmapped_width;
    int maximum_texture2d_mipmapped_height;
    int compute_capability_major;
    int compute_capability_minor;
    int maximum_texture1d_mipmapped_width;
    int stream_priorities_supported;
    int global_l1_cache_supported;
    int local_l1_cache_supported;
    int max_shared_memory_per_multiprocessor;
    int max_registers_per_multiprocessor;
    int managed_memory;
    int multi_gpu_board;
    int multi_gpu_board_group_id;
    int host_native_atomic_supported;
    int single_to_double_precision_perf_ratio;
    int pageable_memory_access;
    int concurrent_managed_access;
    int compute_preemption_supported;
    int can_use_host_pointer_for_registered_mem;
    int can_use_stream_mem_ops;
    int can_use_64_bit_stream_mem_ops;
    int can_use_stream_wait_value_nor;
    int cooperative_launch;
    int cooperative_multi_device_launch;
    int max_shared_memory_per_block_optin;
    int can_flush_remote_writes;
    int host_register_supported;
    int pageable_memory_access_uses_host_page_tables;
    int direct_managed_mem_access_from_host;
    int cores_per_multiprocessor;
    int core_count;
  };

  const Properties &
  properties() const
  {
    return properties_;
  }

  int64_t
  freeMemory() const;

  double // electric power consumed by GPU device, in Watts
  power() const;

private:
  CudaDevice() =default;
  CudaDevice(const CudaDevice &) =delete;
  CudaDevice & operator=(const CudaDevice &) =delete;
  CudaDevice(CudaDevice &&) =default;
  CudaDevice & operator=(CudaDevice &&) =default;
  ~CudaDevice();

  void
  makeCurrent_() const;

  void
  makeCurrent_unchecked_() const noexcept;

  CUdeviceptr
  allocBuffer_(intptr_t size) const;

  void
  freeBuffer_(CUdeviceptr buffer) const noexcept;

  void
  hostToDevice_(CUstream stream,
                CUdeviceptr devDst,
                const void *hostSrc,
                intptr_t size,
                intptr_t dstOffset,
                intptr_t srcOffset) const;

  void
  deviceToHost_(CUstream stream,
                void *hostDst,
                CUdeviceptr devSrc,
                intptr_t size,
                intptr_t dstOffset,
                intptr_t srcOffset) const;

  void
  deviceToDevice_(CUstream stream,
                  CUcontext dstCtx,
                  CUcontext srcCtx,
                  CUdeviceptr devDst,
                  CUdeviceptr devSrc,
                  intptr_t size,
                  intptr_t dstOffset,
                  intptr_t srcOffset) const;

  friend class CudaPlatform;
  friend class CudaStream;
  friend class CudaMarker;
  friend class CudaProgram;
  template<typename T> friend class CudaBuffer;

  static const CudaDevice *current_;

  int id_{-1};
  CUcontext context_{};
  uint64_t peerMask_{};
  std::string name_{};
  Properties properties_{};
#if !defined _WIN32
  nvmlDevice_t nvmlDev_{};
#endif
};

int // maximal size supported by a 1D block
maxBlockSize(const CudaDevice &device);

int // maximal power-of-two size supported by a 1D block
maxPowerOfTwoBlockSize(const CudaDevice &device);

int // a generaly suitable block size
chooseBlockSize(const CudaDevice &device,
                bool powerOfTwo=true);

int // a generaly suitable block count
chooseBlockCount(const CudaDevice &device);

inline
std::tuple<int, // a generaly suitable block size
           int> // a generaly suitable block count
chooseLayout(const CudaDevice& device,
             bool powerOfTwoBlockSize=false)
{
  return {chooseBlockSize(device, powerOfTwoBlockSize),
          chooseBlockCount(device)};
}

std::string
to_string(const CudaDevice &device);

//----------------------------------------------------------------------------

class CudaStream
{
public:

  CudaStream(const CudaDevice &device);

  CudaStream(CudaStream &&rhs) noexcept;

  CudaStream &
  operator=(CudaStream &&rhs) noexcept;

  ~CudaStream();

  const CudaDevice &
  device() const
  {
    return *device_;
  }

  void
  hostSync();

private:
  CudaStream(const CudaStream &) =delete;
  CudaStream & operator=(const CudaStream &) =delete;

  friend class CudaMarker;
  friend class CudaProgram;
  template<typename T> friend class CudaBuffer;

  const CudaDevice *device_;
  CUstream stream_;
};

//----------------------------------------------------------------------------

class CudaMarker
{
public:

  CudaMarker(const CudaDevice &device);

  CudaMarker(CudaMarker &&rhs) noexcept;

  CudaMarker &
  operator=(CudaMarker &&rhs) noexcept;

  ~CudaMarker();

  const CudaDevice &
  device() const
  {
    return *device_;
  }

  void
  set(CudaStream &stream);

  void
  deviceSync(CudaStream &stream);

  void
  hostSync();

  bool // previous work is done
  test() const;

  int64_t // microseconds
  duration(const CudaMarker &previous) const;

private:
  CudaMarker(const CudaMarker &) =delete;
  CudaMarker & operator=(const CudaMarker &) =delete;

  const CudaDevice *device_;
  CUevent event_;
};

//----------------------------------------------------------------------------

class CudaProgram
{
public:
  CudaProgram(const CudaDevice &device,
              std::string name,
              std::string sourceCode,
              std::string options={},
              bool prefersCacheToShared=true);

  CudaProgram(const CudaDevice &device,
              std::string name,
              std::vector<uint8_t> binaryCode,
              bool prefersCacheToShared=true);

  CudaProgram(CudaProgram &&rhs) noexcept;

  CudaProgram &
  operator=(CudaProgram &&rhs) noexcept;

  ~CudaProgram();

  struct Properties
  {
    int max_threads_per_block;
    int shared_size_bytes;
    int const_size_bytes;
    int local_size_bytes;
    int num_regs;
    int ptx_version;
    int binary_version;
    int cache_mode_ca;
    int max_dynamic_shared_size_bytes;
    int preferred_shared_memory_carveout;
  };

  const Properties &
  properties() const
  {
    return properties_;
  }

  const CudaDevice &
  device() const
  {
    return *device_;
  }

  const std::string &
  name() const
  {
    return name_;
  }

  const std::string &
  options() const
  {
    return options_;
  }

  const std::string &
  sourceCode() const
  {
    return sourceCode_;
  }

  const std::vector<uint8_t> &
  binaryCode() const
  {
    return binaryCode_;
  }

  bool
  prefersCacheToShared() const
  {
    return prefersCacheToShared_;
  }

  bool
  buildFailure() const
  {
    return !kernel_;
  }

  const std::string &
  buildLog() const
  {
    return buildLog_;
  }

  void
  launch(CudaStream &stream,
         int xBlockCount,
         int yBlockCount,
         int zBlockCount,
         int xBlockSize,
         int yBlockSize,
         int zBlockSize,
         int sharedMemorySize,
         const void * const *args) const;

  void
  launch(CudaStream &stream,
         int xBlockCount,
         int yBlockCount,
         int xBlockSize,
         int yBlockSize,
         int sharedMemorySize,
         const void * const *args) const
  {
    launch(stream,
           xBlockCount, yBlockCount, 1,
           xBlockSize, yBlockSize, 1,
           sharedMemorySize, args);
  }

  void
  launch(CudaStream &stream,
         int blockCount,
         int blockSize,
         int sharedMemorySize,
         const void * const *args) const
  {
    launch(stream,
           blockCount, 1, 1,
           blockSize, 1, 1,
           sharedMemorySize, args);
  }

private:
  CudaProgram(const CudaProgram &) =delete;
  CudaProgram & operator=(const CudaProgram &) =delete;

  CudaProgram(const CudaDevice &device,
              std::string name,
              std::string sourceCode,
              std::string options,
              std::vector<uint8_t> binaryCode,
              bool prefersCacheToShared);

  const CudaDevice *device_;
  std::string name_;
  std::string sourceCode_;
  std::string options_;
  std::vector<uint8_t> binaryCode_;
  bool prefersCacheToShared_;
  std::string buildLog_;
  CUmodule module_;
  CUfunction kernel_;
  Properties properties_;
};

void
assertSuccess(const CudaProgram &program);

std::string
to_string(const CudaProgram &program);

//----------------------------------------------------------------------------

template<typename T>
class CudaBuffer
{
public:

  static_assert(std::is_trivially_copyable_v<T>,
                "only trivially copyable types allowed");

  CudaBuffer(const CudaDevice &device,
             intptr_t size)
  : device_{&device}
  , devPtr_{}
  , size_{size}
  {
    devPtr_=device_->allocBuffer_(size_*sizeof(T));
  }

  CudaBuffer(CudaBuffer &&rhs) noexcept
  : device_{std::move(rhs.device_)}
  , devPtr_{std::move(rhs.devPtr_)}
  , size_{std::move(rhs.size_)}
  {
    rhs.devPtr_={}; // prevent destruction
  }

  CudaBuffer &
  operator=(CudaBuffer &&rhs) noexcept
  {
    if(this!=&rhs)
    {
      device_=std::move(rhs.device_);
      devPtr_=std::move(rhs.devPtr_);
      size_=std::move(rhs.size_);
      rhs.devPtr_={}; // prevent destruction
    }
    return *this;
  }

  ~CudaBuffer()
  {
    if(devPtr_)
    {
      device_->freeBuffer_(devPtr_);
    }
  }

  const CudaDevice &
  device() const
  {
    return *device_;
  }

  const void * // buffer as program argument
  programArg() const
  {
    return static_cast<const void *>(&devPtr_);
  }

  static
  const void * // null buffer as program argument
  nullProgramArg()
  {
    static const CUdeviceptr nullArg{};
    return static_cast<const void *>(&nullArg);
  }

  intptr_t
  size() const
  {
    return size_;
  }

  void
  fromHost(CudaStream &stream,
           const T *hostSrc,
           intptr_t size=0,
           intptr_t dstOffset=0,
           intptr_t srcOffset=0)
  {
    device_->hostToDevice_(stream.stream_,
                           devPtr_,
                           hostSrc,
                           (size ? size : size_-dstOffset)*sizeof(T),
                           dstOffset*sizeof(T),
                           srcOffset*sizeof(T));
  }

  void
  toHost(CudaStream &stream,
         T *hostDst,
         intptr_t size=0,
         intptr_t dstOffset=0,
         intptr_t srcOffset=0) const
  {
    device_->deviceToHost_(stream.stream_,
                           hostDst,
                           devPtr_,
                           (size ? size : size_-srcOffset)*sizeof(T),
                           dstOffset*sizeof(T),
                           srcOffset*sizeof(T));
  }

  bool // copy to dstBuffer will not involve host
  directCopyAvailable(const CudaBuffer &dstBuffer) const
  {
    return (dstBuffer.device_==device_)||
           (dstBuffer.device_->peerMask_&((uint64_t(1))<<device_->id_));
  }

  void
  toBuffer(CudaStream &stream,
           CudaBuffer &dstBuffer,
           intptr_t size=0,
           intptr_t dstOffset=0,
           intptr_t srcOffset=0) const
  {
    if(!size)
    {
      size=std::min(dstBuffer.size_-dstOffset, size_-srcOffset);
    }
    device_->deviceToDevice_(stream.stream_,
                             dstBuffer.device_->context_,
                             device_->context_,
                             dstBuffer.devPtr_,
                             devPtr_,
                             size*sizeof(T),
                             dstOffset*sizeof(T),
                             srcOffset*sizeof(T));
  }

private:
  CudaBuffer(const CudaBuffer &) =delete;
  CudaBuffer & operator=(const CudaBuffer &) =delete;

  const CudaDevice *device_;
  CUdeviceptr devPtr_;
  intptr_t size_;
};

//----------------------------------------------------------------------------

template<typename T>
class CudaLockedMem
{
public:

  static_assert(std::is_trivially_copyable_v<T>,
                "only trivially copyable types allowed");

  CudaLockedMem(const CudaPlatform &platform,
                bool writeOnly,
                intptr_t size)
  : hostPtr_{}
  , devPtr_{}
  , size_{size}
  , writeOnly_{writeOnly}
  {
    auto [hostPtr, devPtr]=platform.allocLockedMem_(writeOnly_,
                                                    size_*sizeof(T));
    hostPtr_=static_cast<T *>(hostPtr);
    devPtr_=devPtr;
  }

  CudaLockedMem(CudaLockedMem &&rhs) noexcept
  : hostPtr_{std::move(rhs.hostPtr_)}
  , devPtr_{std::move(rhs.devPtr_)}
  , size_{std::move(rhs.size_)}
  , writeOnly_{std::move(rhs.writeOnly_)}
  {
    rhs.hostPtr_={}; // prevent destruction
  }

  CudaLockedMem &
  operator=(CudaLockedMem &&rhs) noexcept
  {
    if(this!=&rhs)
    {
      hostPtr_=std::move(rhs.hostPtr_);
      devPtr_=std::move(rhs.devPtr_);
      size_=std::move(rhs.size_);
      writeOnly_=std::move(rhs.writeOnly_);
      rhs.hostPtr_={}; // prevent destruction
    }
    return *this;
  }

  ~CudaLockedMem()
  {
    if(hostPtr_)
    {
      CudaPlatform::freeLockedMem_(hostPtr_);
    }
  }

  // standard array-like member functions
  auto data()         { return hostPtr_;      }
  auto data()   const { return hostPtr_;      }
  auto size()   const { return size_;         }
  auto empty()  const { return !size();       }
  auto begin()        { return data();        }
  auto begin()  const { return data();        }
  auto cbegin() const { return data();        }
  auto end()          { return data()+size(); }
  auto end()    const { return data()+size(); }
  auto cend()   const { return data()+size(); }

  bool
  writeOnly() const
  {
    return writeOnly_;
  }

  const void * // host memory as zero-copy buffer program argument
  programArg() const
  {
    return static_cast<const void *>(&devPtr_);
  }

private:
  CudaLockedMem(const CudaLockedMem &) =delete;
  CudaLockedMem & operator=(const CudaLockedMem &) =delete;

  T *hostPtr_;
  CUdeviceptr devPtr_;
  intptr_t size_;
  bool writeOnly_;
};

} // namespace crs

#endif // CRS_CUDA_HPP

//----------------------------------------------------------------------------
