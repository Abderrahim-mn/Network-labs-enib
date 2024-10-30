#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# this GNU-makefile relies on the GCC toolchain
# nb: on Windows, the Mingw-w64 package provides the mingw32-make.exe command
#     ( see https://web.enib.fr/~harrouet/dev_tools/#windows.mingw )

#~~~~ control global settings ~~~~
# make opt=2 --> enable optimisation, then disable debug, but keep symbols
# make opt=1 --> enable optimisation, then disable debug
# make opt=0 --> disable optimisation, then enable debug
opt=0
# make clang=1 --> use clang/clang++, not gcc/g++
# make clang=0 --> use gcc/g++, not clang/clang++
clang=0
# make wasm=1 --> target webassembly rather than the native platform
# make wasm=0 --> target the native platform rather than webassembly
wasm=0
# enable/disable OpenSSL usage
use_ssl=1
# enable/disable OpenGL usage
use_gl=0
# enable/disable Cuda usage
use_cuda=0

#~~~~ build library or programs ~~~~
# if LIB_TARGET is provided, this library will be built (with its
# platform-specific name), otherwise ${EXE_PREFIX}* programs will be built
LIB_TARGET=
EXE_PREFIX=prog

#~~~~ detect operating system ~~~~
ifneq (${OS},Windows_NT)
  ifneq (,${findstring Ubuntu,${shell lsb_release -i 2>/dev/null}})
    OS:=Ubuntu
  else ifneq (,${findstring Raspbian,${shell lsb_release -i 2>/dev/null}})
    # Standard distribution for Raspberry-Pi
    OS:=Raspbian
  else
    OS:=${strip ${shell uname -s}}
  endif
  ifneq (,${findstring Microsoft,${shell cat /proc/version 2>/dev/null}})
    # Windows-Subsystem-for-Linux
    OS:=${OS}_WSL
  endif
endif

#~~~~ adjust project-specific settings ~~~~
CPPFLAGS=
# CPPFLAGS+=-I header/path
LDFLAGS=
# LDFLAGS+=-L library/path -Wl,-rpath,library/path -l library_name
CFLAGS=
CXXFLAGS=
BINFLAGS=
ifneq (,${findstring Windows_NT,${OS}})
  LDFLAGS+=-lWS2_32 -lDbghelp
else
  BINFLAGS+=-pthread
  ifneq (,${strip ${LIB_TARGET}})
    BINFLAGS+=-fPIC
  endif
  ifneq (,${findstring Darwin,${OS}})
    # nothing special for now
  else
    LDFLAGS+=-ldl
    ifneq (,${findstring Ubuntu,${OS}})
      # sanitizer sometimes requires gold-linker on Ubuntu
      LDFLAGS+=-fuse-ld=gold
    else ifneq (,${findstring Raspbian,${OS}})
      # some warnings may appear when mixing g++-6 and g++-7 on Raspbian
      CXXFLAGS+=-Wno-psabi
    else
      # nothing special for now
    endif
  endif
endif

#~~~~ settings for OpenSSL usage ~~~~
ifeq (${use_ssl},1)
  ifneq (,${findstring Windows_NT,${OS}})
    # no easy support of OpenSSL on this system
    use_ssl:=0
  else
    ifneq (,${findstring Darwin,${OS}})
      # force Homebrew usage over default version
      export PKG_CONFIG_PATH=/usr/local/opt/openssl/lib/pkgconfig
    endif
    CPPFLAGS+=`pkg-config openssl --cflags` -DUSE_SSL=1
    LDFLAGS+=`pkg-config openssl --libs`
  endif
endif

#~~~~ settings for OpenGL usage ~~~~
ifeq (${use_gl},1)
  ifneq (,${findstring Windows_NT,${OS}})
    CPPFLAGS+=-DUSE_GL=1
    LDFLAGS+=-lUSER32 -lGDI32 -lOPENGL32
  else
    ifneq (,${findstring Darwin,${OS}})
      # no easy support of OpenGL on this system
      use_gl:=0
    else
      CPPFLAGS+=-DUSE_GL=1
      LDFLAGS+=-lX11 -lGL
    endif
  endif
endif

#~~~~ settings for Cuda usage ~~~~
ifeq (${use_cuda},1)
  CPPFLAGS+=-DUSE_CUDA=1
  ifneq (,${findstring Windows_NT,${OS}})
    ifneq (,${wildcard C:\\Program\ Files\\NVIDIA\ GPU\ Computing\ Toolkit\\CUDA\\v9.2})
      CPPFLAGS+=-I "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v9.2/include"
      LDFLAGS+=-L "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v9.2/lib/x64"
    endif
  else
    ifneq (,${findstring Darwin,${OS}})
      # FIXME: add settings
    else
      ifneq (,${wildcard /usr/local/cuda/targets/x86_64-linux})
        CPPFLAGS+=-I /usr/local/cuda/targets/x86_64-linux/include
        LDFLAGS+=-L /usr/local/cuda/targets/x86_64-linux/lib/stubs
        LDFLAGS+=-Wl,-rpath,/usr/local/cuda/targets/x86_64-linux/lib/stubs
      else
        ifneq (,${wildcard /opt/cuda})
          CPPFLAGS+=-I /opt/cuda/include
          LDFLAGS+=-L /opt/cuda/lib64/stubs
          LDFLAGS+=-Wl,-rpath,/opt/cuda/lib64/stubs
        endif
      endif
    endif
    LDFLAGS+=-lnvidia-ml
  endif
  LDFLAGS+=-lcuda -lnvrtc
endif

#~~~~ adjust platform-specific features (Posix/Windows/Emscripten...) ~~~~
ifneq (,${findstring Windows_NT,${OS}})
  LIB_PREFIX=
  LIB_SUFFIX=.dll
  EXE_SUFFIX=.exe
  SKIP_LINE=echo.
  REMOVE=del /q
else
  LIB_PREFIX=lib
  ifneq (,${findstring Darwin,${OS}})
    LIB_SUFFIX=.dylib
  else
    LIB_SUFFIX=.so
  endif
  EXE_SUFFIX=
  SKIP_LINE=echo
  REMOVE=rm -rf
endif
ifeq (${strip ${wasm}},1)
  LIB_PREFIX:=lib
  LIB_SUFFIX:=.bc
  EXE_SUFFIX:=.html
endif

#~~~~ deduce file names ~~~~
ifneq (,${strip ${LIB_TARGET}})
  LIB_TARGET:=${LIB_PREFIX}${strip ${LIB_TARGET}}${LIB_SUFFIX}
  MAIN_C_FILES=
  MAIN_CXX_FILES=
else
  LIB_TARGET:=
  MAIN_C_FILES=${wildcard ${strip ${EXE_PREFIX}}*.c}
  MAIN_CXX_FILES=${wildcard ${strip ${EXE_PREFIX}}*.cpp}
endif
COMMON_C_FILES=${filter-out ${MAIN_C_FILES},${wildcard *.c}}
COMMON_CXX_FILES=${filter-out ${MAIN_CXX_FILES},${wildcard *.cpp}}
#
MAIN_OBJECT_FILES=${sort ${patsubst %.c,%.o,${MAIN_C_FILES}} \
                         ${patsubst %.cpp,%.o,${MAIN_CXX_FILES}}}
COMMON_OBJECT_FILES=${sort ${patsubst %.c,%.o,${COMMON_C_FILES}} \
                           ${patsubst %.cpp,%.o,${COMMON_CXX_FILES}}}
OBJECT_FILES=${MAIN_OBJECT_FILES} ${COMMON_OBJECT_FILES}
DEPEND_FILES=${patsubst %.o,%.d,${OBJECT_FILES}}
EXE_FILES=${sort ${patsubst %.c,%${EXE_SUFFIX},${MAIN_C_FILES}} \
                 ${patsubst %.cpp,%${EXE_SUFFIX},${MAIN_CXX_FILES}}}
#
GENERATED_FILES=${DEPEND_FILES} ${OBJECT_FILES} ${LIB_TARGET} ${EXE_FILES}
ifneq (,${findstring Darwin,${OS}})
  GENERATED_FILES+=${wildcard *.dSYM}
endif
ifeq (${strip ${wasm}},1)
  GENERATED_FILES+=${patsubst %.html,%.js,${EXE_FILES}}
  GENERATED_FILES+=${patsubst %.html,%.html.mem,${EXE_FILES}}
  GENERATED_FILES+=${patsubst %.html,%.wasm,${EXE_FILES}}
  GENERATED_FILES+=${patsubst %.html,%.wast,${EXE_FILES}}
endif
GENERATED_FILES+=${wildcard output_* *~}

#~~~~ compiler/linker settings ~~~~
CPPFLAGS+=-MMD -pedantic -Wall -Wextra -Wconversion -Wno-sign-conversion
CPPFLAGS+=-Wno-unused -Wno-unused-parameter
CPPFLAGS+=-Werror -Wfatal-errors
CFLAGS+=-std=c99 -Wc++-compat -Wwrite-strings -Wold-style-definition -Wvla
CXXFLAGS+=-std=c++17 -Wno-missing-braces
LDFLAGS+=
BINFLAGS+=
ifeq (${strip ${wasm}},1)
  CC=emcc
  CXX=em++
  CPPFLAGS+=-Wno-dollar-in-identifier-extension
  LDFLAGS+=-s ALLOW_MEMORY_GROWTH=1
else ifeq (${strip ${clang}},1)
  CC=clang
  CXX=clang++
else
  CC=gcc
  CXX=g++
endif
#
ifneq (,${strip ${MAIN_CXX_FILES} ${COMMON_CXX_FILES}})
  # force c++ link if there is at least one c++ source file
  LD:=${CXX}
else
  LD:=${CC}
endif

#~~~~ debug/optimisation settings ~~~~
ifneq (${strip ${opt}},0)
  CPPFLAGS+=-DNDEBUG
  BINFLAGS+=-O3 -ffast-math
  # BINFLAGS+=-fopt-info-vec-optimized
  ifneq (${strip ${wasm}},1)
    BINFLAGS+=-march=native
  endif
  ifeq (${strip ${opt}},2)
    # optimise but keep symbols for profiling
    BINFLAGS+=-g -fno-omit-frame-pointer
  else
    BINFLAGS+=-fomit-frame-pointer
  endif
else
  CPPFLAGS+=-UNDEBUG
  BINFLAGS+=-g -O0
  ifeq (${strip ${wasm}},1)
    # sanitizer is not available yet with Emscripten
  else ifneq (,${findstring Windows_NT,${OS}})
    # sanitizer is not available yet on Windows
  else
    BINFLAGS+=-fsanitize=address,undefined
    ifneq (,${findstring Raspbian,${OS}})
      # dynamic sanitizer libraries may not be found on Raspbian
      BINFLAGS+=-static-libasan -static-libubsan
    endif
  endif
endif

#~~~~ main target ~~~~
all : ${EXE_FILES} ${LIB_TARGET}

rebuild : clean all

.SUFFIXES:
.SECONDARY:
.PHONY: all clean rebuild

#~~~~ linker command to produce the library (if any) ~~~~
${LIB_TARGET} : ${COMMON_OBJECT_FILES}
	@echo ==== linking [opt=${opt}] $@ ====
	${LD} -shared -o $@ $^ ${BINFLAGS} ${LDFLAGS}
	@${SKIP_LINE}

#~~~~ linker command to produce the executable files (if any) ~~~~
%${EXE_SUFFIX} : %.o ${COMMON_OBJECT_FILES}
	@echo ==== linking [opt=${opt}] $@ ====
	${LD} -o $@ $^ ${BINFLAGS} ${LDFLAGS}
	@${SKIP_LINE}

#~~~~ compiler command for every source file ~~~~
%.o : %.c
	@echo ==== compiling [opt=${opt}] $< ====
	${CC} -o $@ $< -c ${BINFLAGS} ${CPPFLAGS} ${CFLAGS}
	@${SKIP_LINE}

%.o : %.cpp
	@echo ==== compiling [opt=${opt}] $< ====
	${CXX} -o $@ $< -c ${BINFLAGS} ${CPPFLAGS} ${CXXFLAGS}
	@${SKIP_LINE}

-include ${DEPEND_FILES}

#~~~~ remove generated files ~~~~
clean :
	@echo ==== cleaning ====
	${REMOVE} ${GENERATED_FILES}
	@${SKIP_LINE}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
