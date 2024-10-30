#-----------------------------------------------------------------------------
# this makefile relies on the Visual-Studio-2017 toolchain
# nb: this package provides the nmake.exe command

#~~~~ project-specific settings ~~~~
HEADER_FILES=platformAdapter.h \
             crsUtils.hpp
COMMON_OBJECT_FILES=crsUtils.obj
OBJECT_FILES=$(COMMON_OBJECT_FILES) \
             prog01_http_perf.obj \
             prog01_http_perf_solution.obj \
             prog02_ws_perf.obj \
             prog02_ws_perf_solution.obj
EXE_FILES=prog01_http_perf.exe \
          prog01_http_perf_solution.exe \
          prog02_ws_perf.exe \
          prog02_ws_perf_solution.exe

#~~~~ compiler/linker settings ~~~~
CXX=cl.exe /nologo /std:c++17 /W3 /EHsc
CXXFLAGS=/MD
LDFLAGS=/link USER32.LIB WS2_32.LIB DBGHELP.LIB

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
prog01_http_perf.exe : \
  prog01_http_perf.obj $(COMMON_OBJECT_FILES)

prog01_http_perf.obj : \
  prog01_http_perf.cpp $(HEADER_FILES)

prog01_http_perf_solution.exe : \
  prog01_http_perf_solution.obj $(COMMON_OBJECT_FILES)

prog01_http_perf_solution.obj : \
  prog01_http_perf_solution.cpp $(HEADER_FILES)

prog02_ws_perf.exe : \
  prog02_ws_perf.obj $(COMMON_OBJECT_FILES)

prog02_ws_perf.obj : \
  prog02_ws_perf.cpp $(HEADER_FILES)

prog02_ws_perf_solution.exe : \
  prog02_ws_perf_solution.obj $(COMMON_OBJECT_FILES)

prog02_ws_perf_solution.obj : \
  prog02_ws_perf_solution.cpp $(HEADER_FILES)

crsUtils.obj : crsUtils.cpp $(HEADER_FILES)

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
