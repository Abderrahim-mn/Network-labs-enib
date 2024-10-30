//----------------------------------------------------------------------------

#include "crsCuda.hpp"

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};


  //---- detect and enumerate GPU devices ----
  crs::CudaPlatform platform;
  for(int i=0; i<platform.deviceCount(); ++i)
  {
    //---- display GPU properties ----
    const auto &device=platform.device(i);
    crs::out("%", to_string(device));

    //---- prepare host data ----
    auto pattern={72, 100, 106, 105, 107, 27, 65, 73, 77,
                  23, 109, 100, 102, 95, 86, 18, 16, 15};
    const int patternSize=crs::len(pattern);
    const int dataSize=50*patternSize;
    std::vector<int32_t> hostData(dataSize);
    for(int i=0; i<dataSize; i+=patternSize)
    {
      std::copy(cbegin(pattern), cend(pattern), begin(hostData)+i);
    }

    //---- choose GPU-program layout ----
    const auto [blockSize, blockCount]=chooseLayout(device);

    //---- generate and build GPU-program ----
    crs::CudaProgram program{device, "hello",
R"RAW_CUDA_CODE(
  extern "C" __global__
  void
  hello(int *data,
        int count,
        int modulo)
  {
    const int gridSize=blockDim.x*gridDim.x;
    const int globalId=blockDim.x*blockIdx.x+threadIdx.x;
    for(int id=globalId; id<count; id+=gridSize)
    {
      data[id]+=(id%modulo);
    }
  }
)RAW_CUDA_CODE"};
    assertSuccess(program);

    //---- allocate suitable GPU-buffer ----
    crs::CudaBuffer<int> gpuData(device, dataSize);

    //---- create GPU-command stream ----
    crs::CudaStream stream{device};

    //---- transfer data from host to GPU ----
    gpuData.fromHost(stream, data(hostData));

    //---- launch GPU-program with its prepared parameters ----
    const void *gpuArgs[]={gpuData.programArg(),
                           &dataSize,
                           &patternSize};
    program.launch(stream, blockCount, blockSize, 0, gpuArgs);

    //---- transfer data from GPU to host ----
    gpuData.toHost(stream, data(hostData));

    //---- wait for GPU-commands to finish ----
    stream.hostSync();

    //---- make use of computed data ----
    std::string msg;
    for(const auto &i: hostData)
    {
      msg+=char(i);
    }
    crs::out("%\n", msg);
  }

  return 0;
}

//----------------------------------------------------------------------------
