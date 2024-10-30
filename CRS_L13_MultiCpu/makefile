#-----------------------------------------------------------------------------
# this makefile relies on the Visual-Studio-2017 toolchain
# nb: this package provides the nmake.exe command

#~~~~ project-specific settings ~~~~
HEADER_FILES=platformAdapter.h \
             crsUtils.hpp \
             image.hpp
COMMON_OBJECT_FILES=crsUtils.obj \
                    image.obj
OBJECT_FILES=$(COMMON_OBJECT_FILES) \
             prog00_serial.obj \
             prog01_data_placement.obj \
             prog01_data_placement_solution.obj \
             prog02_task_parallelism.obj \
             prog02_task_parallelism_solution.obj \
             prog03_data_parallelism.obj \
             prog03_data_parallelism_solution.obj \
             prog04_lock_free.obj \
             prog04_lock_free_solution.obj
EXE_FILES=prog00_serial.exe \
          prog01_data_placement.exe \
          prog01_data_placement_solution.exe \
          prog02_task_parallelism.exe \
          prog02_task_parallelism_solution.exe \
          prog03_data_parallelism.exe \
          prog03_data_parallelism_solution.exe \
          prog04_lock_free.exe \
          prog04_lock_free_solution.exe

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
prog00_serial.exe : \
  prog00_serial.obj $(COMMON_OBJECT_FILES)

prog00_serial.obj : \
  prog00_serial.cpp $(HEADER_FILES)

prog01_data_placement.exe : \
  prog01_data_placement.obj $(COMMON_OBJECT_FILES)

prog01_data_placement.obj : \
  prog01_data_placement.cpp $(HEADER_FILES)

prog01_data_placement_solution.exe : \
  prog01_data_placement_solution.obj $(COMMON_OBJECT_FILES)

prog01_data_placement_solution.obj : \
  prog01_data_placement_solution.cpp $(HEADER_FILES)

prog02_task_parallelism.exe : \
  prog02_task_parallelism.obj $(COMMON_OBJECT_FILES)

prog02_task_parallelism.obj : \
  prog02_task_parallelism.cpp $(HEADER_FILES)

prog02_task_parallelism_solution.exe : \
  prog02_task_parallelism_solution.obj $(COMMON_OBJECT_FILES)

prog02_task_parallelism_solution.obj : \
  prog02_task_parallelism_solution.cpp $(HEADER_FILES)

prog03_data_parallelism.exe : \
  prog03_data_parallelism.obj $(COMMON_OBJECT_FILES)

prog03_data_parallelism.obj : \
  prog03_data_parallelism.cpp $(HEADER_FILES)

prog03_data_parallelism_solution.exe : \
  prog03_data_parallelism_solution.obj $(COMMON_OBJECT_FILES)

prog03_data_parallelism_solution.obj : \
  prog03_data_parallelism_solution.cpp $(HEADER_FILES)

prog04_lock_free.exe : \
  prog04_lock_free.obj $(COMMON_OBJECT_FILES)

prog04_lock_free.obj : \
  prog04_lock_free.cpp $(HEADER_FILES)

prog04_lock_free_solution.exe : \
  prog04_lock_free_solution.obj $(COMMON_OBJECT_FILES)

prog04_lock_free_solution.obj : \
  prog04_lock_free_solution.cpp $(HEADER_FILES)

crsUtils.obj : crsUtils.cpp $(HEADER_FILES)

image.obj : image.cpp $(HEADER_FILES)

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
