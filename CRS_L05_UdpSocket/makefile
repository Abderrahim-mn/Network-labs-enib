#-----------------------------------------------------------------------------
# this makefile relies on the Visual-Studio-2017 toolchain
# nb: this package provides the nmake.exe command

#~~~~ project-specific settings ~~~~
HEADER_FILES=platformAdapter.h \
             crsUtils.hpp
COMMON_OBJECT_FILES=crsUtils.obj
OBJECT_FILES=$(COMMON_OBJECT_FILES) \
             prog01_ip_address.obj \
             prog01_ip_address_solution.obj \
             prog02_txt_udp_server.obj \
             prog02_txt_udp_server_solution.obj \
             prog03_txt_udp_client.obj \
             prog03_txt_udp_client_solution.obj \
             prog04_bin_udp_server.obj \
             prog04_bin_udp_server_solution.obj \
             prog05_bin_udp_client.obj \
             prog05_bin_udp_client_solution.obj \
             prog06_txt_bin_udp_server.obj \
             prog06_txt_bin_udp_server_solution.obj
EXE_FILES=prog01_ip_address.exe \
          prog01_ip_address_solution.exe \
          prog02_txt_udp_server.exe \
          prog02_txt_udp_server_solution.exe \
          prog03_txt_udp_client.exe \
          prog03_txt_udp_client_solution.exe \
          prog04_bin_udp_server.exe \
          prog04_bin_udp_server_solution.exe \
          prog05_bin_udp_client.exe \
          prog05_bin_udp_client_solution.exe \
          prog06_txt_bin_udp_server.exe \
          prog06_txt_bin_udp_server_solution.exe

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
prog01_ip_address.exe : \
  prog01_ip_address.obj $(COMMON_OBJECT_FILES)

prog01_ip_address.obj : \
  prog01_ip_address.cpp $(HEADER_FILES)

prog01_ip_address_solution.exe : \
  prog01_ip_address_solution.obj $(COMMON_OBJECT_FILES)

prog01_ip_address_solution.obj : \
  prog01_ip_address_solution.cpp $(HEADER_FILES)

prog02_txt_udp_server.exe : \
  prog02_txt_udp_server.obj $(COMMON_OBJECT_FILES)

prog02_txt_udp_server.obj : \
  prog02_txt_udp_server.cpp $(HEADER_FILES)

prog02_txt_udp_server_solution.exe : \
  prog02_txt_udp_server_solution.obj $(COMMON_OBJECT_FILES)

prog02_txt_udp_server_solution.obj : \
  prog02_txt_udp_server_solution.cpp $(HEADER_FILES)

prog03_txt_udp_client.exe : \
  prog03_txt_udp_client.obj $(COMMON_OBJECT_FILES)

prog03_txt_udp_client.obj : \
  prog03_txt_udp_client.cpp $(HEADER_FILES)

prog03_txt_udp_client_solution.exe : \
  prog03_txt_udp_client_solution.obj $(COMMON_OBJECT_FILES)

prog03_txt_udp_client_solution.obj : \
  prog03_txt_udp_client_solution.cpp $(HEADER_FILES)

prog04_bin_udp_server.exe : \
  prog04_bin_udp_server.obj $(COMMON_OBJECT_FILES)

prog04_bin_udp_server.obj : \
  prog04_bin_udp_server.cpp $(HEADER_FILES)

prog04_bin_udp_server_solution.exe : \
  prog04_bin_udp_server_solution.obj $(COMMON_OBJECT_FILES)

prog04_bin_udp_server_solution.obj : \
  prog04_bin_udp_server_solution.cpp $(HEADER_FILES)

prog05_bin_udp_client.exe : \
  prog05_bin_udp_client.obj $(COMMON_OBJECT_FILES)

prog05_bin_udp_client.obj : \
  prog05_bin_udp_client.cpp $(HEADER_FILES)

prog05_bin_udp_client_solution.exe : \
  prog05_bin_udp_client_solution.obj $(COMMON_OBJECT_FILES)

prog05_bin_udp_client_solution.obj : \
  prog05_bin_udp_client_solution.cpp $(HEADER_FILES)

prog06_txt_bin_udp_server.exe : \
  prog06_txt_bin_udp_server.obj $(COMMON_OBJECT_FILES)

prog06_txt_bin_udp_server.obj : \
  prog06_txt_bin_udp_server.cpp $(HEADER_FILES)

prog06_txt_bin_udp_server_solution.exe : \
  prog06_txt_bin_udp_server_solution.obj $(COMMON_OBJECT_FILES)

prog06_txt_bin_udp_server_solution.obj : \
  prog06_txt_bin_udp_server_solution.cpp $(HEADER_FILES)

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
