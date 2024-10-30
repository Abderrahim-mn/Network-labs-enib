#-----------------------------------------------------------------------------
# this makefile relies on the Visual-Studio-2017 toolchain
# nb: this package provides the nmake.exe command

#~~~~ project-specific settings ~~~~
HEADER_FILES=platformAdapter.h \
             crsUtils.hpp
COMMON_OBJECT_FILES=crsUtils.obj
OBJECT_FILES=$(COMMON_OBJECT_FILES) \
             prog_http_client.obj \
             prog_http_client_solution.obj
EXE_FILES=prog_http_client.exe \
          prog_http_client_solution.exe

#~~~~ compiler/linker settings ~~~~
CXX=cl.exe /nologo /std:c++17 /W3 /EHsc
CXXFLAGS=/MD
LDFLAGS=/link USER32.LIB WS2_32.LIB DBGHELP.LIB

#~~~~ debug/optimisation settings ~~~~
# (un)comment to {en|dis}able optimisation
# CXXFLAGS=$(CXXFLAGS) /O2
#
# (un)comment to {en|dis}able debug informations
CXXFLAGS=$(CXXFLAGS) /Z7
LDFLAGS=$(LDFLAGS) /debug

#~~~~ main target ~~~~
all : $(EXE_FILES)

rebuild : clean all

#~~~~ source file dependencies ~~~~
prog_http_client.exe : \
  prog_http_client.obj $(COMMON_OBJECT_FILES)

prog_http_client.obj : \
  prog_http_client.cpp $(HEADER_FILES)

prog_http_client_solution.exe : \
  prog_http_client_solution.obj $(COMMON_OBJECT_FILES)

prog_http_client_solution.obj : \
  prog_http_client_solution.cpp $(HEADER_FILES)

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
