#-----------------------------------------------------------------------------
# this makefile relies on the Visual-Studio-2017 toolchain
# nb: this package provides the nmake.exe command

#~~~~ project-specific settings ~~~~
HEADER_FILES=platformAdapter.h \
             crsUtils.hpp
COMMON_OBJECT_FILES=crsUtils.obj
OBJECT_FILES=$(COMMON_OBJECT_FILES) \
             prog01_create_join.obj \
             prog01_create_join_solution.obj \
             prog02_multiple.obj \
             prog02_multiple_solution.obj \
             prog03_synchronisation.obj \
             prog03_synchronisation_solution.obj \
             prog04_producer_consumer.obj \
             prog04_producer_consumer_solution.obj
EXE_FILES=prog01_create_join.exe \
          prog01_create_join_solution.exe \
          prog02_multiple.exe \
          prog02_multiple_solution.exe \
          prog03_synchronisation.exe \
          prog03_synchronisation_solution.exe \
          prog04_producer_consumer.exe \
          prog04_producer_consumer_solution.exe

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
prog01_create_join.exe : \
  prog01_create_join.obj $(COMMON_OBJECT_FILES)

prog01_create_join.obj : \
  prog01_create_join.cpp $(HEADER_FILES)

prog01_create_join_solution.exe : \
  prog01_create_join_solution.obj $(COMMON_OBJECT_FILES)

prog01_create_join_solution.obj : \
  prog01_create_join_solution.cpp $(HEADER_FILES)

prog02_multiple.exe : \
  prog02_multiple.obj $(COMMON_OBJECT_FILES)

prog02_multiple.obj : \
  prog02_multiple.cpp $(HEADER_FILES)

prog02_multiple_solution.exe : \
  prog02_multiple_solution.obj $(COMMON_OBJECT_FILES)

prog02_multiple_solution.obj : \
  prog02_multiple_solution.cpp $(HEADER_FILES)

prog03_synchronisation.exe : \
  prog03_synchronisation.obj $(COMMON_OBJECT_FILES)

prog03_synchronisation.obj : \
  prog03_synchronisation.cpp $(HEADER_FILES)

prog03_synchronisation_solution.exe : \
  prog03_synchronisation_solution.obj $(COMMON_OBJECT_FILES)

prog03_synchronisation_solution.obj : \
  prog03_synchronisation_solution.cpp $(HEADER_FILES)

prog04_producer_consumer.exe : \
  prog04_producer_consumer.obj $(COMMON_OBJECT_FILES)

prog04_producer_consumer.obj : \
  prog04_producer_consumer.cpp $(HEADER_FILES)

prog04_producer_consumer_solution.exe : \
  prog04_producer_consumer_solution.obj $(COMMON_OBJECT_FILES)

prog04_producer_consumer_solution.obj : \
  prog04_producer_consumer_solution.cpp $(HEADER_FILES)

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
