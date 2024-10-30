//----------------------------------------------------------------------------

#include "crsCuda.hpp"

namespace crs {

#define THROW_CUDA_FAILURE(function, errorCode)                     \
        do                                                          \
        {                                                           \
          std::string msg{CudaPlatform::errorMessage((errorCode))}; \
          throw std::runtime_error{txt(                             \
            "%:%:%() %() failure --- %\n%",                         \
            __FILE__, __LINE__, __func__,                           \
            function, msg, computeStackTrace())};                   \
        } while(0)

#define CUDA_CALL(function, args)                 \
        do                                        \
        {                                         \
          CUresult cuErr=function args;           \
          if(cuErr)                               \
          {                                       \
            THROW_CUDA_FAILURE(#function, cuErr); \
          }                                       \
        } while(0)

CudaPlatform::CudaPlatform()
: devices_{}
, deviceCount_{}
{
#if !defined NDEBUG
  auto varName="ASAN_OPTIONS";
  auto varValue="protect_shadow_gap=0:replace_intrin=0:detect_leaks=0";
  if(getenv(varName)!=varValue)
  {
    err("warning: CUDA initialisation may fail (due to the sanitizer)\n");
    err("  before launching your program, try using this command line:\n\n");
    err("    export %=%\n\n", varName, varValue);
  }
#endif
#if !defined _WIN32
  const bool useNvml=nvmlInit()==NVML_SUCCESS;
#endif
  CUDA_CALL(cuInit, (0));
  CUDA_CALL(cuDeviceGetCount, (&deviceCount_));
  // private ctor/dtor --> std::make_unique() unusable
  devices_=new CudaDevice[deviceCount()];
  for(int i=0; i<deviceCount(); ++i)
  {
    auto &dev=devices_[i];
    CUDA_CALL(cuDeviceGet, (&dev.id_, i));
    CUDA_CALL(cuCtxCreate, (&dev.context_,
                            CU_CTX_SCHED_SPIN|CU_CTX_MAP_HOST, dev.id()));
    dev.makeCurrent_();
    if(dev.id()>=int(8*sizeof(dev.peerMask_)))
    {
      throw std::runtime_error{txt(
        "%:%:%() failure --- insufficient width for CudaDevice.peerMask_\n%",
        __FILE__, __LINE__, __func__,
        computeStackTrace())};
    }
    char name[0x80]="";
    CUDA_CALL(cuDeviceGetName, (name, sizeof(name), dev.id()));
    dev.name_=name;
    auto &prop=dev.properties_;
    size_t totalMemory;
    CUDA_CALL(cuDeviceTotalMem, (&totalMemory, dev.id()));
    prop.total_memory=int64_t(totalMemory);
#define DEV_ATTR(value, attrib) \
        CUDA_CALL(cuDeviceGetAttribute, (&prop.value, attrib, dev.id()))
    DEV_ATTR(max_threads_per_block,
             CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK);
    DEV_ATTR(max_block_dim_x,
             CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_X);
    DEV_ATTR(max_block_dim_y,
             CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Y);
    DEV_ATTR(max_block_dim_z,
             CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Z);
    DEV_ATTR(max_grid_dim_x,
             CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_X);
    DEV_ATTR(max_grid_dim_y,
             CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Y);
    DEV_ATTR(max_grid_dim_z,
             CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Z);
    DEV_ATTR(max_shared_memory_per_block,
             CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK);
    DEV_ATTR(total_constant_memory,
             CU_DEVICE_ATTRIBUTE_TOTAL_CONSTANT_MEMORY);
    DEV_ATTR(warp_size,
             CU_DEVICE_ATTRIBUTE_WARP_SIZE);
    DEV_ATTR(max_pitch,
             CU_DEVICE_ATTRIBUTE_MAX_PITCH);
    DEV_ATTR(max_registers_per_block,
             CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_BLOCK);
    DEV_ATTR(clock_rate_kHz,
             CU_DEVICE_ATTRIBUTE_CLOCK_RATE);
    DEV_ATTR(texture_alignment,
             CU_DEVICE_ATTRIBUTE_TEXTURE_ALIGNMENT);
    DEV_ATTR(gpu_overlap,
             CU_DEVICE_ATTRIBUTE_GPU_OVERLAP);
    DEV_ATTR(multiprocessor_count,
             CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT);
    DEV_ATTR(kernel_exec_timeout,
             CU_DEVICE_ATTRIBUTE_KERNEL_EXEC_TIMEOUT);
    DEV_ATTR(integrated,
             CU_DEVICE_ATTRIBUTE_INTEGRATED);
    DEV_ATTR(can_map_host_memory,
             CU_DEVICE_ATTRIBUTE_CAN_MAP_HOST_MEMORY);
    DEV_ATTR(compute_mode,
             CU_DEVICE_ATTRIBUTE_COMPUTE_MODE);
    DEV_ATTR(maximum_texture1d_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE1D_WIDTH);
    DEV_ATTR(maximum_texture2d_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_WIDTH);
    DEV_ATTR(maximum_texture2d_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_HEIGHT);
    DEV_ATTR(maximum_texture3d_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_WIDTH);
    DEV_ATTR(maximum_texture3d_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_HEIGHT);
    DEV_ATTR(maximum_texture3d_depth,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_DEPTH);
    DEV_ATTR(maximum_texture2d_layered_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_LAYERED_WIDTH);
    DEV_ATTR(maximum_texture2d_layered_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_LAYERED_HEIGHT);
    DEV_ATTR(maximum_texture2d_layered_layers,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_LAYERED_LAYERS);
    DEV_ATTR(surface_alignment,
             CU_DEVICE_ATTRIBUTE_SURFACE_ALIGNMENT);
    DEV_ATTR(concurrent_kernels,
             CU_DEVICE_ATTRIBUTE_CONCURRENT_KERNELS);
    DEV_ATTR(ecc_enabled,
             CU_DEVICE_ATTRIBUTE_ECC_ENABLED);
    DEV_ATTR(pci_bus_id,
             CU_DEVICE_ATTRIBUTE_PCI_BUS_ID);
    DEV_ATTR(pci_device_id,
             CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID);
    DEV_ATTR(tcc_driver,
             CU_DEVICE_ATTRIBUTE_TCC_DRIVER);
    DEV_ATTR(memory_clock_rate_kHz,
             CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE);
    DEV_ATTR(global_memory_bus_width,
             CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH);
    DEV_ATTR(l2_cache_size,
             CU_DEVICE_ATTRIBUTE_L2_CACHE_SIZE);
    DEV_ATTR(max_threads_per_multiprocessor,
             CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR);
    DEV_ATTR(async_engine_count,
             CU_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT);
    DEV_ATTR(unified_addressing,
             CU_DEVICE_ATTRIBUTE_UNIFIED_ADDRESSING);
    DEV_ATTR(maximum_texture1d_layered_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE1D_LAYERED_WIDTH);
    DEV_ATTR(maximum_texture1d_layered_layers,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE1D_LAYERED_LAYERS);
    DEV_ATTR(maximum_texture2d_gather_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_GATHER_WIDTH);
    DEV_ATTR(maximum_texture2d_gather_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_GATHER_HEIGHT);
    DEV_ATTR(maximum_texture3d_width_alternate,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_WIDTH_ALTERNATE);
    DEV_ATTR(maximum_texture3d_height_alternate,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_HEIGHT_ALTERNATE);
    DEV_ATTR(maximum_texture3d_depth_alternate,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_DEPTH_ALTERNATE);
    DEV_ATTR(pci_domain_id,
             CU_DEVICE_ATTRIBUTE_PCI_DOMAIN_ID);
    DEV_ATTR(texture_pitch_alignment,
             CU_DEVICE_ATTRIBUTE_TEXTURE_PITCH_ALIGNMENT);
    DEV_ATTR(maximum_texturecubemap_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURECUBEMAP_WIDTH);
    DEV_ATTR(maximum_texturecubemap_layered_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURECUBEMAP_LAYERED_WIDTH);
    DEV_ATTR(maximum_texturecubemap_layered_layers,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURECUBEMAP_LAYERED_LAYERS);
    DEV_ATTR(maximum_surface1d_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE1D_WIDTH);
    DEV_ATTR(maximum_surface2d_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE2D_WIDTH);
    DEV_ATTR(maximum_surface2d_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE2D_HEIGHT);
    DEV_ATTR(maximum_surface3d_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE3D_WIDTH);
    DEV_ATTR(maximum_surface3d_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE3D_HEIGHT);
    DEV_ATTR(maximum_surface3d_depth,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE3D_DEPTH);
    DEV_ATTR(maximum_surface1d_layered_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE1D_LAYERED_WIDTH);
    DEV_ATTR(maximum_surface1d_layered_layers,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE1D_LAYERED_LAYERS);
    DEV_ATTR(maximum_surface2d_layered_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE2D_LAYERED_WIDTH);
    DEV_ATTR(maximum_surface2d_layered_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE2D_LAYERED_HEIGHT);
    DEV_ATTR(maximum_surface2d_layered_layers,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACE2D_LAYERED_LAYERS);
    DEV_ATTR(maximum_surfacecubemap_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACECUBEMAP_WIDTH);
    DEV_ATTR(maximum_surfacecubemap_layered_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACECUBEMAP_LAYERED_WIDTH);
    DEV_ATTR(maximum_surfacecubemap_layered_layers,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_SURFACECUBEMAP_LAYERED_LAYERS);
    DEV_ATTR(maximum_texture1d_linear_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE1D_LINEAR_WIDTH);
    DEV_ATTR(maximum_texture2d_linear_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_LINEAR_WIDTH);
    DEV_ATTR(maximum_texture2d_linear_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_LINEAR_HEIGHT);
    DEV_ATTR(maximum_texture2d_linear_pitch,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_LINEAR_PITCH);
    DEV_ATTR(maximum_texture2d_mipmapped_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_MIPMAPPED_WIDTH);
    DEV_ATTR(maximum_texture2d_mipmapped_height,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_MIPMAPPED_HEIGHT);
    DEV_ATTR(compute_capability_major,
             CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR);
    DEV_ATTR(compute_capability_minor,
             CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR);
    DEV_ATTR(maximum_texture1d_mipmapped_width,
             CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE1D_MIPMAPPED_WIDTH);
    DEV_ATTR(stream_priorities_supported,
             CU_DEVICE_ATTRIBUTE_STREAM_PRIORITIES_SUPPORTED);
    DEV_ATTR(global_l1_cache_supported,
             CU_DEVICE_ATTRIBUTE_GLOBAL_L1_CACHE_SUPPORTED);
    DEV_ATTR(local_l1_cache_supported,
             CU_DEVICE_ATTRIBUTE_LOCAL_L1_CACHE_SUPPORTED);
    DEV_ATTR(max_shared_memory_per_multiprocessor,
             CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_MULTIPROCESSOR);
    DEV_ATTR(max_registers_per_multiprocessor,
             CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_MULTIPROCESSOR);
    DEV_ATTR(managed_memory,
             CU_DEVICE_ATTRIBUTE_MANAGED_MEMORY);
    DEV_ATTR(multi_gpu_board,
             CU_DEVICE_ATTRIBUTE_MULTI_GPU_BOARD);
    DEV_ATTR(multi_gpu_board_group_id,
             CU_DEVICE_ATTRIBUTE_MULTI_GPU_BOARD_GROUP_ID);
    DEV_ATTR(host_native_atomic_supported,
             CU_DEVICE_ATTRIBUTE_HOST_NATIVE_ATOMIC_SUPPORTED);
    DEV_ATTR(single_to_double_precision_perf_ratio,
             CU_DEVICE_ATTRIBUTE_SINGLE_TO_DOUBLE_PRECISION_PERF_RATIO);
    DEV_ATTR(pageable_memory_access,
             CU_DEVICE_ATTRIBUTE_PAGEABLE_MEMORY_ACCESS);
    DEV_ATTR(concurrent_managed_access,
             CU_DEVICE_ATTRIBUTE_CONCURRENT_MANAGED_ACCESS);
    DEV_ATTR(compute_preemption_supported,
             CU_DEVICE_ATTRIBUTE_COMPUTE_PREEMPTION_SUPPORTED);
    DEV_ATTR(can_use_host_pointer_for_registered_mem,
             CU_DEVICE_ATTRIBUTE_CAN_USE_HOST_POINTER_FOR_REGISTERED_MEM);
#if CUDA_VERSION < 12000
    DEV_ATTR(can_use_stream_mem_ops,
             CU_DEVICE_ATTRIBUTE_CAN_USE_STREAM_MEM_OPS);
#else
    DEV_ATTR(can_use_stream_mem_ops,
             CU_DEVICE_ATTRIBUTE_CAN_USE_STREAM_MEM_OPS_V1);
#endif
    DEV_ATTR(can_use_64_bit_stream_mem_ops,
             CU_DEVICE_ATTRIBUTE_CAN_USE_64_BIT_STREAM_MEM_OPS);
    DEV_ATTR(can_use_stream_wait_value_nor,
             CU_DEVICE_ATTRIBUTE_CAN_USE_STREAM_WAIT_VALUE_NOR);
    DEV_ATTR(cooperative_launch,
             CU_DEVICE_ATTRIBUTE_COOPERATIVE_LAUNCH);
    DEV_ATTR(cooperative_multi_device_launch,
             CU_DEVICE_ATTRIBUTE_COOPERATIVE_MULTI_DEVICE_LAUNCH);
    DEV_ATTR(max_shared_memory_per_block_optin,
             CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK_OPTIN);
    DEV_ATTR(can_flush_remote_writes,
             CU_DEVICE_ATTRIBUTE_CAN_FLUSH_REMOTE_WRITES);
    DEV_ATTR(host_register_supported,
             CU_DEVICE_ATTRIBUTE_HOST_REGISTER_SUPPORTED);
    DEV_ATTR(pageable_memory_access_uses_host_page_tables,
             CU_DEVICE_ATTRIBUTE_PAGEABLE_MEMORY_ACCESS_USES_HOST_PAGE_TABLES);
    DEV_ATTR(direct_managed_mem_access_from_host,
             CU_DEVICE_ATTRIBUTE_DIRECT_MANAGED_MEM_ACCESS_FROM_HOST);
    // https://en.wikipedia.org/wiki/CUDA#Version_features_and_specifications
    const int coreCounts[][3]={{1, 0,   8}, // Tesla
                               {1, 1,   8},
                               {1, 2,   8},
                               {1, 3,   8},
                               {2, 0,  32}, // Fermi
                               {2, 1,  48},
                               {3, 0, 192}, // Kepler
                               {3, 2, 192},
                               {3, 5, 192},
                               {3, 7, 192},
                               {5, 0, 128}, // Maxwell
                               {5, 2, 128},
                               {5, 3, 128},
                               {6, 0,  64}, // Pascal
                               {6, 1, 128},
                               {6, 2, 128},
                               {7, 0,  64}, // Volta
                               {7, 1,  64},
                               {7, 2,  64},
                               {7, 5,  64}, // Turing
                               {8, 0,  64}, // Ampere
                               {8, 6,  64},
                               {0, 0,   0}};
    int lastIndex=0;
    for(int c=0; coreCounts[c][0]; ++c)
    {
      if((coreCounts[c][0]>prop.compute_capability_major)||
         ((coreCounts[c][0]==prop.compute_capability_major)&&
          (coreCounts[c][1]>prop.compute_capability_minor)))
      {
        break;
      }
      lastIndex=c;
    }
    if((coreCounts[lastIndex][0]!=prop.compute_capability_major)||
       (coreCounts[lastIndex][1]!=prop.compute_capability_minor))
    {
      err("warning: unknown compute capability"
          " %.% for GPU device %, assuming %.%\n",
          prop.compute_capability_major,
          prop.compute_capability_minor,
          dev.name(),
          coreCounts[lastIndex][0],
          coreCounts[lastIndex][1]);
    }
    prop.cores_per_multiprocessor=coreCounts[lastIndex][2];
    prop.core_count=prop.multiprocessor_count*
                    prop.cores_per_multiprocessor;
#if !defined _WIN32
    if(useNvml)
    {
      char pciStr[0x20]="";
      std::snprintf(pciStr, sizeof(pciStr)-1, "%.8x:%.2x:%.2x.0",
                    prop.pci_domain_id,
                    prop.pci_bus_id,
                    prop.pci_device_id);
      nvmlDeviceGetHandleByPciBusId(pciStr, &dev.nvmlDev_);
    }
#endif
  }
  for(int i=0; i<deviceCount(); ++i) // sort devices to ease peer access
  {
    CudaDevice *devA=&devices_[i];
    CudaDevice *bestDev=devA;
    for(int j=i+1; j<deviceCount(); ++j)
    {
      CudaDevice *devB=&devices_[j];
#define COMPARE_FIELD(f)                               \
        if(devB->properties_.f>bestDev->properties_.f) \
        {                                              \
          bestDev=devB;                                \
          continue;                                    \
        }                                              \
        if(devB->properties_.f<bestDev->properties_.f) \
        {                                              \
          continue;                                    \
        }
      COMPARE_FIELD(compute_capability_major)
      COMPARE_FIELD(compute_capability_minor)
      COMPARE_FIELD(multiprocessor_count)
      COMPARE_FIELD(clock_rate_kHz)
      COMPARE_FIELD(total_memory)
      COMPARE_FIELD(memory_clock_rate_kHz)
    }
    if(bestDev!=devA)
    {
      // private move-operations --> std::swap() unusable
      CudaDevice tmp{std::move(*devA)};
      *devA=std::move(*bestDev);
      *bestDev=std::move(tmp);
    }
  }
  for(int i=0; i<deviceCount(); ++i)
  {
    CudaDevice &devA=devices_[i];
    for(int j=i+1; j<deviceCount(); ++j)
    {
      CudaDevice &devB=devices_[j];
      int canAccess;
      CUDA_CALL(cuDeviceCanAccessPeer, (&canAccess, devA.id(), devB.id()));
      if(canAccess)
      {
        devA.peerMask_|=(uint64_t(1))<<devB.id();
        devA.makeCurrent_();
        CUDA_CALL(cuCtxEnablePeerAccess, (devB.context_, 0));
      }
      CUDA_CALL(cuDeviceCanAccessPeer, (&canAccess, devB.id(), devA.id()));
      if(canAccess)
      {
        devB.peerMask_|=(uint64_t(1))<<devA.id();
        devB.makeCurrent_();
        CUDA_CALL(cuCtxEnablePeerAccess, (devA.context_, 0));
      }
    }
  }
}

CudaPlatform::CudaPlatform(CudaPlatform &&rhs) noexcept
: devices_{std::move(rhs.devices_)}
, deviceCount_{std::move(rhs.deviceCount_)}
{
  rhs.devices_={}; // prevent destruction
}

CudaPlatform &
CudaPlatform::operator=(CudaPlatform &&rhs) noexcept
{
  if(this!=&rhs)
  {
    devices_=std::move(rhs.devices_);
    deviceCount_=std::move(rhs.deviceCount_);
    rhs.devices_={}; // prevent destruction
  }
  return *this;
}

CudaPlatform::~CudaPlatform()
{
  if(devices_)
  {
    delete []devices_;
#if !defined _WIN32
    nvmlShutdown();
#endif
  }
}

const CudaDevice & // index'th cuda device
CudaPlatform::device(int index)
{
  if(index<0||index>=deviceCount_)
  {
    throw std::runtime_error{txt(
              "%:%:%() failure --- no device at index %d\n%",
              __FILE__, __LINE__, __func__,
              index, computeStackTrace())};
  }
  return devices_[index];
}

std::string
CudaPlatform::errorMessage(int errorCode)
{
  const char *errorMsg;
  switch(errorCode)
  {
#define ERR_CODE_MSG(code) \
        case code:         \
        {                  \
          errorMsg=#code;  \
          break;           \
        }
    ERR_CODE_MSG(CUDA_SUCCESS)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_VALUE)
    ERR_CODE_MSG(CUDA_ERROR_OUT_OF_MEMORY)
    ERR_CODE_MSG(CUDA_ERROR_NOT_INITIALIZED)
    ERR_CODE_MSG(CUDA_ERROR_DEINITIALIZED)
    ERR_CODE_MSG(CUDA_ERROR_PROFILER_DISABLED)
    ERR_CODE_MSG(CUDA_ERROR_PROFILER_NOT_INITIALIZED)
    ERR_CODE_MSG(CUDA_ERROR_PROFILER_ALREADY_STARTED)
    ERR_CODE_MSG(CUDA_ERROR_PROFILER_ALREADY_STOPPED)
    ERR_CODE_MSG(CUDA_ERROR_NO_DEVICE)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_DEVICE)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_IMAGE)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_CONTEXT)
    ERR_CODE_MSG(CUDA_ERROR_CONTEXT_ALREADY_CURRENT)
    ERR_CODE_MSG(CUDA_ERROR_MAP_FAILED)
    ERR_CODE_MSG(CUDA_ERROR_UNMAP_FAILED)
    ERR_CODE_MSG(CUDA_ERROR_ARRAY_IS_MAPPED)
    ERR_CODE_MSG(CUDA_ERROR_ALREADY_MAPPED)
    ERR_CODE_MSG(CUDA_ERROR_NO_BINARY_FOR_GPU)
    ERR_CODE_MSG(CUDA_ERROR_ALREADY_ACQUIRED)
    ERR_CODE_MSG(CUDA_ERROR_NOT_MAPPED)
    ERR_CODE_MSG(CUDA_ERROR_NOT_MAPPED_AS_ARRAY)
    ERR_CODE_MSG(CUDA_ERROR_NOT_MAPPED_AS_POINTER)
    ERR_CODE_MSG(CUDA_ERROR_ECC_UNCORRECTABLE)
    ERR_CODE_MSG(CUDA_ERROR_UNSUPPORTED_LIMIT)
    ERR_CODE_MSG(CUDA_ERROR_CONTEXT_ALREADY_IN_USE)
    ERR_CODE_MSG(CUDA_ERROR_PEER_ACCESS_UNSUPPORTED)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_PTX)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_GRAPHICS_CONTEXT)
    ERR_CODE_MSG(CUDA_ERROR_NVLINK_UNCORRECTABLE)
    ERR_CODE_MSG(CUDA_ERROR_JIT_COMPILER_NOT_FOUND)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_SOURCE)
    ERR_CODE_MSG(CUDA_ERROR_FILE_NOT_FOUND)
    ERR_CODE_MSG(CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND)
    ERR_CODE_MSG(CUDA_ERROR_SHARED_OBJECT_INIT_FAILED)
    ERR_CODE_MSG(CUDA_ERROR_OPERATING_SYSTEM)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_HANDLE)
    ERR_CODE_MSG(CUDA_ERROR_NOT_FOUND)
    ERR_CODE_MSG(CUDA_ERROR_NOT_READY)
    ERR_CODE_MSG(CUDA_ERROR_ILLEGAL_ADDRESS)
    ERR_CODE_MSG(CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES)
    ERR_CODE_MSG(CUDA_ERROR_LAUNCH_TIMEOUT)
    ERR_CODE_MSG(CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING)
    ERR_CODE_MSG(CUDA_ERROR_PEER_ACCESS_ALREADY_ENABLED)
    ERR_CODE_MSG(CUDA_ERROR_PEER_ACCESS_NOT_ENABLED)
    ERR_CODE_MSG(CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE)
    ERR_CODE_MSG(CUDA_ERROR_CONTEXT_IS_DESTROYED)
    ERR_CODE_MSG(CUDA_ERROR_ASSERT)
    ERR_CODE_MSG(CUDA_ERROR_TOO_MANY_PEERS)
    ERR_CODE_MSG(CUDA_ERROR_HOST_MEMORY_ALREADY_REGISTERED)
    ERR_CODE_MSG(CUDA_ERROR_HOST_MEMORY_NOT_REGISTERED)
    ERR_CODE_MSG(CUDA_ERROR_HARDWARE_STACK_ERROR)
    ERR_CODE_MSG(CUDA_ERROR_ILLEGAL_INSTRUCTION)
    ERR_CODE_MSG(CUDA_ERROR_MISALIGNED_ADDRESS)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_ADDRESS_SPACE)
    ERR_CODE_MSG(CUDA_ERROR_INVALID_PC)
    ERR_CODE_MSG(CUDA_ERROR_LAUNCH_FAILED)
    ERR_CODE_MSG(CUDA_ERROR_COOPERATIVE_LAUNCH_TOO_LARGE)
    ERR_CODE_MSG(CUDA_ERROR_NOT_PERMITTED)
    ERR_CODE_MSG(CUDA_ERROR_NOT_SUPPORTED)
    ERR_CODE_MSG(CUDA_ERROR_UNKNOWN)
    default:
    {
      errorMsg="unknown Cuda error";
      break;
    }
  }
  return std::string(errorMsg)+" ("+std::to_string(errorCode)+')';
}

std::tuple<void *,      // host pointer
           CUdeviceptr> // device pointer
CudaPlatform::allocLockedMem_(bool writeOnly,
                              intptr_t size)
{
  // no need to select a specific context with CU_MEMHOSTALLOC_PORTABLE 
  unsigned int flags=CU_MEMHOSTALLOC_PORTABLE|CU_MEMHOSTALLOC_DEVICEMAP;
  if(writeOnly)
  {
    flags|=CU_MEMHOSTALLOC_WRITECOMBINED;
  }
  void *hostPtr;
  CUDA_CALL(cuMemHostAlloc, (&hostPtr, size, flags));
  // FIXME: is this really not specific to any device?
  CUdeviceptr devPtr;
  CUDA_CALL(cuMemHostGetDevicePointer,(&devPtr, hostPtr, 0));
  return {hostPtr, devPtr};
}

void
CudaPlatform::freeLockedMem_(void *hostPtr) noexcept
{
  cuMemFreeHost(hostPtr);
}

//----------------------------------------------------------------------------

const CudaDevice * CudaDevice::current_;

int64_t
CudaDevice::freeMemory() const
{
  makeCurrent_();
  size_t freeMem, totalMem;
  CUDA_CALL(cuMemGetInfo, (&freeMem, &totalMem));
  return int64_t(freeMem);
}

double // electric power consumed by GPU device, in Watts
CudaDevice::power() const
{
  unsigned int milliWatts=0;
#if !defined _WIN32
  if(nvmlDev_)
  {
    nvmlDeviceGetPowerUsage(nvmlDev_, &milliWatts);
  }
#endif
  return double(milliWatts)*1e-3;
}

CudaDevice::~CudaDevice()
{
  if(current_==this)
  {
    current_=nullptr;
    cuCtxSetCurrent(nullptr);
  }
  if(context_)
  {
    cuCtxDestroy(context_);
  }
}

void
CudaDevice::makeCurrent_() const
{
  if(current_!=this)
  {
    current_=this;
    CUDA_CALL(cuCtxSetCurrent, (context_));
  }
}

void
CudaDevice::makeCurrent_unchecked_() const noexcept
{
  if(current_!=this)
  {
    current_=this;
    cuCtxSetCurrent(context_);
  }
}

CUdeviceptr
CudaDevice::allocBuffer_(intptr_t size) const
{
  makeCurrent_();
  CUdeviceptr devPtr;
  CUDA_CALL(cuMemAlloc, (&devPtr, size));
  return devPtr;
}

void
CudaDevice::freeBuffer_(CUdeviceptr devPtr) const noexcept
{
  if(current_!=this)
  {
    current_=this;
    cuCtxSetCurrent(context_);
  }
  cuMemFree(devPtr);
}

void
CudaDevice::hostToDevice_(CUstream stream,
                          CUdeviceptr devDst,
                          const void *hostSrc,
                          intptr_t size,
                          intptr_t dstOffset,
                          intptr_t srcOffset) const
{
  makeCurrent_();
  CUDA_CALL(cuMemcpyHtoDAsync, (dstOffset+devDst,
                                srcOffset+(const uint8_t *)hostSrc,
                                size, stream));
}

void
CudaDevice::deviceToHost_(CUstream stream,
                          void *hostDst,
                          CUdeviceptr devSrc,
                          intptr_t size,
                          intptr_t dstOffset,
                          intptr_t srcOffset) const
{
  makeCurrent_();
  CUDA_CALL(cuMemcpyDtoHAsync, (dstOffset+(uint8_t *)hostDst,
                                srcOffset+devSrc,
                                size, stream));
}

void
CudaDevice::deviceToDevice_(CUstream stream,
                            CUcontext dstCtx,
                            CUcontext srcCtx,
                            CUdeviceptr devDst,
                            CUdeviceptr devSrc,
                            intptr_t size,
                            intptr_t dstOffset,
                            intptr_t srcOffset) const
{
  makeCurrent_();
  const auto dst=dstOffset+devDst;
  const auto src=srcOffset+devSrc;
  if(dstCtx==srcCtx)
  {
    CUDA_CALL(cuMemcpyDtoDAsync, (dst, src, size, stream));
  }
  else
  {
    CUDA_CALL(cuMemcpyPeerAsync, (dst, dstCtx, src, srcCtx, size, stream));
  }
}

int // maximal size supported by a 1D block
maxBlockSize(const CudaDevice &device)
{
  const auto &prop=device.properties();
  return std::min(prop.max_threads_per_block,
                  prop.max_block_dim_x);
}

int // maximal power-of-two size supported by a 1D block
maxPowerOfTwoBlockSize(const CudaDevice &device)
{
  const int maxSz=maxBlockSize(device);
  int sz=device.properties().warp_size;
  while((sz<<1)<=maxSz)
  {
    sz<<=1;
  }
  return sz;
}

int // a generaly suitable block size
chooseBlockSize(const CudaDevice &device,
                bool powerOfTwo)
{
  (void)powerOfTwo; // avoid ``unused parameter'' warning
  // A power-of-two block size is not mandatory but seems to be faster
  // FIXME: this hardcoded setting gives good average performances for some
  //        experiments on the actual device used during development.
  //        The performances may vary for other devices and/or algorithms that
  //        would require specific settings.
  const int sz=maxPowerOfTwoBlockSize(device)/4;
  const int warpSize=device.properties().warp_size;
  return std::max(sz, warpSize);
}

int // a generaly suitable block count
chooseBlockCount(const CudaDevice &device)
{
  // FIXME: this hardcoded setting gives good average performances for some
  //        experiments on the actual device used during development.
  //        The performances may vary for other devices and/or algorithms that
  //        would require specific settings.
  return 8*device.properties().multiprocessor_count;
}

std::string
to_string(const CudaDevice &device)
{
  const auto &prop=device.properties();
  std::string result;
  result+=txt("CUDA device %: %\n", device.id(), device.name());
#define SHOW_PROPERTY(pname) \
        result+=txt("  "#pname": %\n", prop.pname)
  result+=txt("  compute_capability: %.%\n",
              prop.compute_capability_major,
              prop.compute_capability_minor);
  result+=txt("  total_memory: %\n",
              prop.total_memory);
  result+=txt("  free_memory: %\n",
              device.freeMemory());
  SHOW_PROPERTY(max_threads_per_block);
  result+=txt("  max_block_dim: % % %\n",
              prop.max_block_dim_x,
              prop.max_block_dim_y,
              prop.max_block_dim_z);
  result+=txt("  max_grid_dim: % % %\n",
              prop.max_grid_dim_x,
              prop.max_grid_dim_y,
              prop.max_grid_dim_z);
  SHOW_PROPERTY(max_shared_memory_per_block);
  SHOW_PROPERTY(total_constant_memory);
  SHOW_PROPERTY(warp_size);
  SHOW_PROPERTY(max_registers_per_block);
  SHOW_PROPERTY(clock_rate_kHz);
  SHOW_PROPERTY(gpu_overlap);
  SHOW_PROPERTY(multiprocessor_count);
  SHOW_PROPERTY(cores_per_multiprocessor);
  SHOW_PROPERTY(core_count);
  SHOW_PROPERTY(kernel_exec_timeout);
  SHOW_PROPERTY(integrated);
  SHOW_PROPERTY(can_map_host_memory);
  SHOW_PROPERTY(compute_mode);
  SHOW_PROPERTY(concurrent_kernels);
  SHOW_PROPERTY(ecc_enabled);
  SHOW_PROPERTY(pci_bus_id);
  SHOW_PROPERTY(pci_device_id);
  SHOW_PROPERTY(tcc_driver);
  SHOW_PROPERTY(memory_clock_rate_kHz);
  SHOW_PROPERTY(global_memory_bus_width);
  SHOW_PROPERTY(l2_cache_size);
  SHOW_PROPERTY(max_threads_per_multiprocessor);
  SHOW_PROPERTY(async_engine_count);
  SHOW_PROPERTY(unified_addressing);
  SHOW_PROPERTY(pci_domain_id);
  SHOW_PROPERTY(stream_priorities_supported);
  SHOW_PROPERTY(global_l1_cache_supported);
  SHOW_PROPERTY(local_l1_cache_supported);
  SHOW_PROPERTY(max_shared_memory_per_multiprocessor);
  SHOW_PROPERTY(max_registers_per_multiprocessor);
  SHOW_PROPERTY(managed_memory);
  SHOW_PROPERTY(multi_gpu_board);
  SHOW_PROPERTY(multi_gpu_board_group_id);
  SHOW_PROPERTY(host_native_atomic_supported);
  SHOW_PROPERTY(single_to_double_precision_perf_ratio);
  SHOW_PROPERTY(pageable_memory_access);
  SHOW_PROPERTY(concurrent_managed_access);
  SHOW_PROPERTY(compute_preemption_supported);
  SHOW_PROPERTY(can_use_host_pointer_for_registered_mem);
  SHOW_PROPERTY(can_use_stream_mem_ops);
  SHOW_PROPERTY(can_use_64_bit_stream_mem_ops);
  SHOW_PROPERTY(can_use_stream_wait_value_nor);
  SHOW_PROPERTY(cooperative_launch);
  SHOW_PROPERTY(cooperative_multi_device_launch);
  SHOW_PROPERTY(max_shared_memory_per_block_optin);
  SHOW_PROPERTY(can_flush_remote_writes);
  SHOW_PROPERTY(host_register_supported);
  SHOW_PROPERTY(pageable_memory_access_uses_host_page_tables);
  SHOW_PROPERTY(direct_managed_mem_access_from_host);
  SHOW_PROPERTY(cores_per_multiprocessor);
  SHOW_PROPERTY(core_count);
  return result;
}

//----------------------------------------------------------------------------

CudaStream::CudaStream(const CudaDevice &device)
: device_{&device}
, stream_{}
{
  device_->makeCurrent_();
  CUDA_CALL(cuStreamCreate, (&stream_, CU_STREAM_NON_BLOCKING));
}

CudaStream::CudaStream(CudaStream &&rhs) noexcept
: device_{std::move(rhs.device_)}
, stream_{std::move(rhs.stream_)}
{
  rhs.stream_={}; // prevent destruction
}


CudaStream &
CudaStream::operator=(CudaStream &&rhs) noexcept
{
  if(this!=&rhs)
  {
    device_=std::move(rhs.device_);
    stream_=std::move(rhs.stream_);
    rhs.stream_={}; // prevent destruction
  }
  return *this;
}

CudaStream::~CudaStream()
{
  if(stream_)
  {
    device_->makeCurrent_unchecked_();
    cuStreamDestroy(stream_);
  }
}

void
CudaStream::hostSync()
{
  device_->makeCurrent_();
  CUDA_CALL(cuStreamSynchronize, (stream_));
}

//----------------------------------------------------------------------------

CudaMarker::CudaMarker(const CudaDevice &device)
: device_{&device}
, event_{}
{
  device_->makeCurrent_();
  CUDA_CALL(cuEventCreate, (&event_, CU_EVENT_DEFAULT));
}

CudaMarker::CudaMarker(CudaMarker &&rhs) noexcept
: device_{std::move(rhs.device_)}
, event_{std::move(rhs.event_)}
{
  rhs.event_={}; // prevent destruction
}

CudaMarker &
CudaMarker::operator=(CudaMarker &&rhs) noexcept
{
  if(this!=&rhs)
  {
    device_=std::move(rhs.device_);
    event_=std::move(rhs.event_);
    rhs.event_={}; // prevent destruction
  }
  return *this;
}

CudaMarker::~CudaMarker()
{
  if(event_)
  {
    device_->makeCurrent_unchecked_();
    cuEventDestroy(event_);
  }
}

void
CudaMarker::set(CudaStream &stream)
{
  device_->makeCurrent_();
  CUDA_CALL(cuEventRecord, (event_, stream.stream_));
}

void
CudaMarker::deviceSync(CudaStream &stream)
{
  device_->makeCurrent_(); // FIXME: really necessary?
  CUDA_CALL(cuStreamWaitEvent, (stream.stream_, event_, 0));
}

void
CudaMarker::hostSync()
{
  // device_->makeCurrent_(); // not necessary
  CUDA_CALL(cuEventSynchronize, (event_));
}

bool // previous work is done
CudaMarker::test() const
{
  // device_->makeCurrent_(); // not necessary
  CUresult cuErr=cuEventQuery(event_);
  switch(cuErr)
  {
    case CUDA_SUCCESS:
    {
      return true;
    }
    case CUDA_ERROR_NOT_READY:
    {
      return false;
    }
    default:
    {
      THROW_CUDA_FAILURE("cuEventQuery", cuErr);
      return false;
    }
  }
}

int64_t // microseconds
CudaMarker::duration(const CudaMarker &previous) const
{
  // device_->makeCurrent_(); // not necessary
  float milliseconds;
  CUDA_CALL(cuEventElapsedTime, (&milliseconds, previous.event_, event_));
  return int64_t(1.0e3*real64_t(milliseconds));
}

//----------------------------------------------------------------------------

CudaProgram::CudaProgram(const CudaDevice &device,
                         std::string name,
                         std::string sourceCode,
                         std::string options,
                         bool prefersCacheToShared)
: CudaProgram{device, std::move(name),
              std::move(sourceCode), std::move(options), {},
              prefersCacheToShared}
{
  // nothing more to be done
}

CudaProgram::CudaProgram(const CudaDevice &device,
                         std::string name,
                         std::vector<uint8_t> binaryCode,
                         bool prefersCacheToShared)
: CudaProgram{device, std::move(name),
              {}, {}, std::move(binaryCode),
              prefersCacheToShared}
{
  // nothing more to be done
}

CudaProgram::CudaProgram(const CudaDevice &device,
                         std::string name,
                         std::string sourceCode,
                         std::string options,
                         std::vector<uint8_t> binaryCode,
                         bool prefersCacheToShared)
: device_{&device}
, name_{std::move(name)}
, sourceCode_{std::move(sourceCode)}
, options_{std::move(options)}
, binaryCode_{std::move(binaryCode)}
, prefersCacheToShared_{prefersCacheToShared}
, buildLog_{}
, module_{}
, kernel_{}
, properties_{}
{
  device_->makeCurrent_();
  CUresult cuErr;
  if(!empty(sourceCode_))
  {
    nvrtcResult res;
    nvrtcProgram prog;
    res=nvrtcCreateProgram(&prog, data(sourceCode_) , data(name_),
                           0, nullptr, nullptr);
    if(res!=NVRTC_SUCCESS)
    {
      buildLog_+=txt("nvrtcCreateProgram(%) failure: %\n",
                     name_, nvrtcGetErrorString(res));
    }
    else
    {
      auto optionWords=split(options_);
      const auto &prop=device_->properties();
      optionWords.emplace_back(txt("-arch=compute_%%",
                                   prop.compute_capability_major,
                                   prop.compute_capability_minor));
      optionWords.emplace_back("-default-device");
#if defined NDEBUG
      optionWords.emplace_back("-use_fast_math");
      optionWords.emplace_back("-restrict");
#endif
      options_=join(optionWords, ' ');
      std::vector<const char *> rawOptions;
      rawOptions.reserve(size(optionWords));
      std::transform(
        cbegin(optionWords), cend(optionWords), back_inserter(rawOptions),
        [](const auto &elem)
        {
          return data(elem);
        });
      nvrtcResult compRes=nvrtcCompileProgram(prog, len(rawOptions),
                                              data(rawOptions));
      size_t logSize=0;
      res=nvrtcGetProgramLogSize(prog, &logSize);
      if(res!=NVRTC_SUCCESS)
      {
        buildLog_+=txt("nvrtcGetProgramLogSize(%) failure: %\n",
                       name_, nvrtcGetErrorString(res));
      }
      if(logSize>0)
      {
        std::string log;
        uninitialised_resize(log, logSize);
        res=nvrtcGetProgramLog(prog, data(log));
        if(res!=NVRTC_SUCCESS)
        {
          buildLog_+=txt("nvrtcGetProgramLog(%) failure: %\n",
                         name_, nvrtcGetErrorString(res));
        }
        else
        {
          while(!empty(log)&&
                ((log.back()=='\0')||std::isspace(log.back())))
          {
            log.pop_back();
          }
          if(!empty(log))
          {
            log+='\n';
            buildLog_+=log;
          }
        }
      }
      if(compRes!=NVRTC_SUCCESS)
      {
        buildLog_+=txt("nvrtcCompileProgram(%) failure: %\n",
                       name_, nvrtcGetErrorString(compRes));
      }
      else
      {
        size_t ptxSize;
        res=nvrtcGetPTXSize(prog, &ptxSize);
        if(res!=NVRTC_SUCCESS)
        {
          buildLog_+=txt("nvrtcGetPTXSize(%) failure: %\n",
                         name_, nvrtcGetErrorString(res));
        }
        else
        {
          std::vector<uint8_t> ptxCode;
          uninitialised_resize(ptxCode, ptxSize);
          res=nvrtcGetPTX(prog, (char *)data(ptxCode));
          if(res!=NVRTC_SUCCESS)
          {
            buildLog_+=txt("nvrtcGetPTX(%) failure: %\n",
                           name_, nvrtcGetErrorString(res));
          }
          else
          {
            binaryCode_=std::move(ptxCode);
          }
        }
      }
      res=nvrtcDestroyProgram(&prog);
      if(res!=NVRTC_SUCCESS)
      {
        buildLog_+=txt("nvrtcDestroyProgram(%) failure: %\n",
                       name_, nvrtcGetErrorString(res));
      }
    }
  }
  if(!empty(binaryCode_))
  {
    cuErr=cuModuleLoadData(&module_, data(binaryCode_));
    if(cuErr)
    {
      buildLog_+=txt("cuModuleLoadData() failure: %\n",
                     CudaPlatform::errorMessage(cuErr));
    }
  }
  if(module_)
  {
    cuErr=cuModuleGetFunction(&kernel_, module_, data(name_));
    if(cuErr)
    {
      buildLog_+=txt("cuModuleGetFunction() failure: %\n",
                     CudaPlatform::errorMessage(cuErr));
    }
  }
  if(kernel_)
  {
    CUDA_CALL(cuFuncSetCacheConfig, (kernel_,
                                     prefersCacheToShared_
                                     ? CU_FUNC_CACHE_PREFER_L1
                                     : CU_FUNC_CACHE_PREFER_SHARED));
    Properties &prop=properties_;
#define FUNC_ATTR(value, attrib) \
        CUDA_CALL(cuFuncGetAttribute, (&prop.value, attrib , kernel_))
    FUNC_ATTR(max_threads_per_block,
              CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK);
    FUNC_ATTR(shared_size_bytes,
              CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES);
    FUNC_ATTR(const_size_bytes,
              CU_FUNC_ATTRIBUTE_CONST_SIZE_BYTES);
    FUNC_ATTR(local_size_bytes,
              CU_FUNC_ATTRIBUTE_LOCAL_SIZE_BYTES);
    FUNC_ATTR(num_regs,
              CU_FUNC_ATTRIBUTE_NUM_REGS);
    FUNC_ATTR(ptx_version,
              CU_FUNC_ATTRIBUTE_PTX_VERSION);
    FUNC_ATTR(binary_version,
              CU_FUNC_ATTRIBUTE_BINARY_VERSION);
    FUNC_ATTR(cache_mode_ca,
              CU_FUNC_ATTRIBUTE_CACHE_MODE_CA);
    FUNC_ATTR(max_dynamic_shared_size_bytes,
              CU_FUNC_ATTRIBUTE_MAX_DYNAMIC_SHARED_SIZE_BYTES);
    FUNC_ATTR(preferred_shared_memory_carveout,
              CU_FUNC_ATTRIBUTE_PREFERRED_SHARED_MEMORY_CARVEOUT);
  }
}

CudaProgram::CudaProgram(CudaProgram &&rhs) noexcept
: device_{std::move(rhs.device_)}
, sourceCode_{std::move(rhs.sourceCode_)}
, options_{std::move(rhs.options_)}
, binaryCode_{std::move(rhs.binaryCode_)}
, prefersCacheToShared_{std::move(rhs.prefersCacheToShared_)}
, buildLog_{std::move(rhs.buildLog_)}
, module_{std::move(rhs.module_)}
, kernel_{std::move(rhs.kernel_)}
, properties_{std::move(rhs.properties_)}
{
    rhs.module_={}; // prevent destruction
}

CudaProgram &
CudaProgram::operator=(CudaProgram &&rhs) noexcept
{
  if(this!=&rhs)
  {
    device_=std::move(rhs.device_);
    sourceCode_=std::move(rhs.sourceCode_);
    options_=std::move(rhs.options_);
    binaryCode_=std::move(rhs.binaryCode_);
    prefersCacheToShared_=std::move(rhs.prefersCacheToShared_);
    buildLog_=std::move(rhs.buildLog_);
    module_=std::move(rhs.module_);
    kernel_=std::move(rhs.kernel_);
    properties_=std::move(rhs.properties_);
    rhs.module_={}; // prevent destruction
  }
  return *this;
}

CudaProgram::~CudaProgram()
{
  if(module_)
  {
    device_->makeCurrent_unchecked_();
    cuModuleUnload(module_);
  }
}

void
CudaProgram::launch(CudaStream &stream,
                    int xBlockCount,
                    int yBlockCount,
                    int zBlockCount,
                    int xBlockSize,
                    int yBlockSize,
                    int zBlockSize,
                    int sharedMemorySize,
                    const void * const *args) const
{
  device_->makeCurrent_();
  CUDA_CALL(cuLaunchKernel, (kernel_,
                             xBlockCount, yBlockCount, zBlockCount,
                             xBlockSize, yBlockSize, zBlockSize,
                             sharedMemorySize,
                             stream.stream_,
                             (void **)args, nullptr));
}

void
assertSuccess(const CudaProgram &program)
{
  if(!empty(program.buildLog()))
  {
    err("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    auto lines=split(program.sourceCode(), "\n", true);
    auto lineCount=len(lines);
    auto maxTagWidth=size(std::to_string(lineCount));
    for(int i=0; i<lineCount; ++i)
    {
      const auto tag=std::to_string(i+1);
      err("%%:%\n", std::string(maxTagWidth-size(tag), ' '), tag, lines[i]);
    }
    err("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n%"
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n",
        program.buildLog());
  }
  if(program.buildFailure())
  {
    throw std::runtime_error{txt("%:%:%() CUDA program build failure\n%",
                                 __FILE__, __LINE__, __func__,
                                 computeStackTrace())};
  }
}

std::string
to_string(const CudaProgram &program)
{
  const auto &prop=program.properties();
  std::string result;
  result+=txt("CUDA program: %\n", program.name());
  result+=txt("  options: %\n", program.options());
  SHOW_PROPERTY(max_threads_per_block);
  SHOW_PROPERTY(shared_size_bytes);
  SHOW_PROPERTY(const_size_bytes);
  SHOW_PROPERTY(local_size_bytes);
  SHOW_PROPERTY(num_regs);
  SHOW_PROPERTY(ptx_version);
  SHOW_PROPERTY(binary_version);
  SHOW_PROPERTY(cache_mode_ca);
  SHOW_PROPERTY(max_dynamic_shared_size_bytes);
  SHOW_PROPERTY(preferred_shared_memory_carveout);
  return result;
}

} // namespace crs

//----------------------------------------------------------------------------
