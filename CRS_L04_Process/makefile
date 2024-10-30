#-----------------------------------------------------------------------------
# this makefile relies on the Visual-Studio-2017 toolchain
# nb: this package provides the nmake.exe command

#~~~~ project-specific settings ~~~~
HEADER_FILES=platformAdapter.h \
             crsUtils.hpp
COMMON_OBJECT_FILES=crsUtils.obj
OBJECT_FILES=$(COMMON_OBJECT_FILES) \
             prog01_interaction.obj \
             prog01_interaction_solution.obj \
             prog02_copy.obj \
             prog02_copy_solution.obj \
             prog03_fork_waitpid.obj \
             prog03_fork_waitpid_solution.obj \
             prog04_address_space.obj \
             prog04_address_space_solution.obj \
             prog05_pipe.obj \
             prog05_pipe_solution.obj \
             prog06_redir_exec.obj \
             prog06_redir_exec_solution.obj
EXE_FILES=prog01_interaction.exe \
          prog01_interaction_solution.exe \
          prog02_copy.exe \
          prog02_copy_solution.exe \
          prog03_fork_waitpid.exe \
          prog03_fork_waitpid_solution.exe \
          prog04_address_space.exe \
          prog04_address_space_solution.exe \
          prog05_pipe.exe \
          prog05_pipe_solution.exe \
          prog06_redir_exec.exe \
          prog06_redir_exec_solution.exe

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
prog01_interaction.exe : \
  prog01_interaction.obj $(COMMON_OBJECT_FILES)

prog01_interaction.obj : \
  prog01_interaction.cpp $(HEADER_FILES)

prog01_interaction_solution.exe : \
  prog01_interaction_solution.obj $(COMMON_OBJECT_FILES)

prog01_interaction_solution.obj : \
  prog01_interaction_solution.cpp $(HEADER_FILES)

prog02_copy.exe : \
  prog02_copy.obj $(COMMON_OBJECT_FILES)

prog02_copy.obj : \
  prog02_copy.cpp $(HEADER_FILES)

prog02_copy_solution.exe : \
  prog02_copy_solution.obj $(COMMON_OBJECT_FILES)

prog02_copy_solution.obj : \
  prog02_copy_solution.cpp $(HEADER_FILES)

prog03_fork_waitpid.exe : \
  prog03_fork_waitpid.obj $(COMMON_OBJECT_FILES)

prog03_fork_waitpid.obj : \
  prog03_fork_waitpid.cpp $(HEADER_FILES)

prog03_fork_waitpid_solution.exe : \
  prog03_fork_waitpid_solution.obj $(COMMON_OBJECT_FILES)

prog03_fork_waitpid_solution.obj : \
  prog03_fork_waitpid_solution.cpp $(HEADER_FILES)

prog04_address_space.exe : \
  prog04_address_space.obj $(COMMON_OBJECT_FILES)

prog04_address_space.obj : \
  prog04_address_space.cpp $(HEADER_FILES)

prog04_address_space_solution.exe : \
  prog04_address_space_solution.obj $(COMMON_OBJECT_FILES)

prog04_address_space_solution.obj : \
  prog04_address_space_solution.cpp $(HEADER_FILES)

prog05_pipe.exe : \
  prog05_pipe.obj $(COMMON_OBJECT_FILES)

prog05_pipe.obj : \
  prog05_pipe.cpp $(HEADER_FILES)

prog05_pipe_solution.exe : \
  prog05_pipe_solution.obj $(COMMON_OBJECT_FILES)

prog05_pipe_solution.obj : \
  prog05_pipe_solution.cpp $(HEADER_FILES)

prog06_redir_exec.exe : \
  prog06_redir_exec.obj $(COMMON_OBJECT_FILES)

prog06_redir_exec.obj : \
  prog06_redir_exec.cpp $(HEADER_FILES)

prog06_redir_exec_solution.exe : \
  prog06_redir_exec_solution.obj $(COMMON_OBJECT_FILES)

prog06_redir_exec_solution.obj : \
  prog06_redir_exec_solution.cpp $(HEADER_FILES)

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
