#-----------------------------------------------------------------------------
# this makefile relies on the Visual-Studio-2017 toolchain
# nb: this package provides the nmake.exe command

#~~~~ project-specific settings ~~~~
HEADER_FILES=platformAdapter.h \
             crsUtils.hpp \
             crsCuda.hpp
COMMON_OBJECT_FILES=crsUtils.obj \
                    crsCuda.obj
OBJECT_FILES=$(COMMON_OBJECT_FILES) \
             prog00_hello.obj \
             prog01_compute.obj \
             prog01_compute_solution.obj \
             prog02_transfer.obj \
             prog02_transfer_solution.obj \
             prog03_AOS.obj \
             prog03_AOS_solution.obj \
             prog04_SOA.obj \
             prog04_SOA_solution.obj
EXE_FILES=prog00_hello.exe \
          prog01_compute.exe \
          prog01_compute_solution.exe \
          prog02_transfer.exe \
          prog02_transfer_solution.exe \
          prog03_AOS.exe \
          prog03_AOS_solution.exe \
          prog04_SOA.exe \
          prog04_SOA_solution.exe

#~~~~ compiler/linker settings ~~~~
CXX=cl.exe /nologo /std:c++17 /W3 /EHsc
CXXFLAGS=/MD /DUSE_CUDA=1
CXXFLAGS=$(CXXFLAGS) /I"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v9.2/include"
LDFLAGS=/link USER32.LIB WS2_32.LIB DBGHELP.LIB
LDFLAGS=$(LDFLAGS) /libpath:"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v9.2/lib/x64"
LDFLAGS=$(LDFLAGS) CUDA.LIB NVRTC.LIB

#~~~~ debug/optimisation settings ~~~~
# (un)comment to {en|dis}able optimisation
CXXFLAGS=$(CXXFLAGS) /O2
#
# (un)comment to {en|dis}able debug informations
# CXXFLAGS=$(CXXFLAGS) /Z7
# LDFLAGS=$(LDFLAGS) /debug

#~~~~ main target ~~~~
all : $(EXE_FILES)

rebuild : clean all

#~~~~ source file dependencies ~~~~
prog00_hello.exe : \
  prog00_hello.obj $(COMMON_OBJECT_FILES)

prog00_hello.obj : \
  prog00_hello.cpp $(HEADER_FILES)

prog01_compute.exe : \
  prog01_compute.obj $(COMMON_OBJECT_FILES)

prog01_compute.obj : \
  prog01_compute.cpp $(HEADER_FILES)

prog01_compute_solution.exe : \
  prog01_compute_solution.obj $(COMMON_OBJECT_FILES)

prog01_compute_solution.obj : \
  prog01_compute_solution.cpp $(HEADER_FILES)

prog02_transfer.exe : \
  prog02_transfer.obj $(COMMON_OBJECT_FILES)

prog02_transfer.obj : \
  prog02_transfer.cpp $(HEADER_FILES)

prog02_transfer_solution.exe : \
  prog02_transfer_solution.obj $(COMMON_OBJECT_FILES)

prog02_transfer_solution.obj : \
  prog02_transfer_solution.cpp $(HEADER_FILES)

prog03_AOS.exe : \
  prog03_AOS.obj $(COMMON_OBJECT_FILES)

prog03_AOS.obj : \
  prog03_AOS.cpp $(HEADER_FILES)

prog03_AOS_solution.exe : \
  prog03_AOS_solution.obj $(COMMON_OBJECT_FILES)

prog03_AOS_solution.obj : \
  prog03_AOS_solution.cpp $(HEADER_FILES)

prog04_SOA.exe : \
  prog04_SOA.obj $(COMMON_OBJECT_FILES)

prog04_SOA.obj : \
  prog04_SOA.cpp $(HEADER_FILES)

prog04_SOA_solution.exe : \
  prog04_SOA_solution.obj $(COMMON_OBJECT_FILES)

prog04_SOA_solution.obj : \
  prog04_SOA_solution.cpp $(HEADER_FILES)

crsUtils.obj : crsUtils.cpp $(HEADER_FILES)

crsCuda.obj : crsCuda.cpp $(HEADER_FILES)

#~~~~ linker command to produce the executable files ~~~~
.obj.exe :
	@echo ==== linking $@ ====
	$(CXX) $** $(LDFLAGS) /out:$@
	@echo.

#~~~~ compiler command for every source file ~~~~
.cpp.obj :
	@echo ==== compiling $< ====
	$(CXX) /Fo$@ /c $<  $(CXXFLAGS)
	@echo.

#~~~~ remove generated files ~~~~
clean :
	@echo ==== cleaning ====
	del /q $(OBJECT_FILES) $(EXE_FILES) *.pdb *.ilk output_*
	@echo.

#-----------------------------------------------------------------------------
