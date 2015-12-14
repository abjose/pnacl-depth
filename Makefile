#
# If NACL_SDK_ROOT is not set, then fail with a nice message.
#
THIS_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
#NACL_SDK_ROOT ?= $(abspath $(dir $(THIS_MAKEFILE))../..)

# Project Build flags
WARNINGS := -Wno-long-long -Wall -Wswitch-enum -pedantic -Werror
CXXFLAGS := -pthread -std=gnu++98 $(WARNINGS)

#
# Compute tool paths
#
GETOS := python $(NACL_SDK_ROOT)/tools/getos.py
OSHELPERS = python $(NACL_SDK_ROOT)/tools/oshelpers.py
OSNAME := $(shell $(GETOS))
RM := $(OSHELPERS) rm

PNACL_TC_PATH := $(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_pnacl)
PNACL_CXX := $(PNACL_TC_PATH)/bin/pnacl-clang++
PNACL_FINALIZE := $(PNACL_TC_PATH)/bin/pnacl-finalize
CXXFLAGS := -I$(NACL_SDK_ROOT)/include
LDFLAGS := -L$(NACL_SDK_ROOT)/lib/pnacl/Release -lppapi_cpp -lppapi

HDRS := stitching.h
SRCS := stitching.cc nacl_glue.cc

## Note that OPENCV should have been compiled and installed in the appropriate
## NaCl (pnacl, hopefully here) toolching pseudo root. So no need to paste any
#CXXFLAGS += -I../naclports/src/out/repository/opencv-2.4.7/include/
#CXXFLAGS += $(shell pkg-config --cflags opencv)
CXXFLAGS += -I../naclports/src/out/build/opencv/opencv-2.4.9/modules/core/include
CXXFLAGS += -I../naclports/src/out/build/opencv/opencv-2.4.9/modules/features2d/include
CXXFLAGS += -I../naclports/src/out/build/opencv/opencv-2.4.9/modules/flann/include
CXXFLAGS += -I../naclports/src/out/build/opencv/opencv-2.4.9/modules/imgproc/include
CXXFLAGS += -I../naclports/src/out/build/opencv/opencv-2.4.9/modules/calib3d/include
CXXFLAGS += -I../naclports/src/out/build/opencv/opencv-2.4.9/modules/highgui/include
CXXFLAGS += -I../naclports/src/out/build/opencv/opencv-2.4.9/include/

LDFLAGS  += -L../naclports/src/out/build/opencv/opencv-2.4.9/build/lib/
LDFLAGS  += -lopencv_features2d \
	    -lopencv_flann \
            -lopencv_nonfree \
            -lopencv_legacy \
            -lopencv_calib3d \
            -lopencv_imgproc \
            -lopencv_highgui \
            -lopencv_core \
            -lz

CVLDFLAGS = -L$(NACL_SDK_ROOT)/lib/pnacl/Release \
	-lopencv_features2d \
	-lopencv_flann \
	-lopencv_legacy \
	-lopencv_calib3d \
	-lopencv_imgproc \
	-lopencv_highgui \
	-lopencv_core \
	-lz \
        # -lopencv_nonfree

#LDFLAGS  += -Wl,--pnacl-driver-verbose #-Wl,--verbose
#LDFLAGS  += $(shell pkg-config --libs opencv)

# Declare the ALL target first, to make the 'all' target the default build
all: guard-NACL_SDK_ROOT nacl_glue.pexe

test: rectification_test.cpp
	g++ $(CXXFLAGS) -o $@ $< $(CVLDFLAGS)

clean:
	$(RM) *.pexe *.bc

nacl_glue.bc: $(SRCS) $(HDRS)
	$(PNACL_CXX) -O2 $(CXXFLAGS) $(SRCS) $(LDFLAGS)   -o $@

nacl_glue.pexe: nacl_glue.bc
	$(PNACL_FINALIZE) -o $@ $<


guard-NACL_SDK_ROOT:
	@if [ "${NACL_SDK_ROOT}" == "" ]; then \
	  tput setaf 1; \
	  echo "Environment variable $* not set, please define it pointing to\
 your nacl_sdk/pepper_XY folder. See \
 http://developers.google.com/native-client/dev/"; \
    tput sgr 0; \
	  exit 1; \
	fi

#
# Makefile target to run the SDK's simple HTTP server and serve this example.
#
HTTPD_PY := python ./tools/httpd.py

.PHONY: serve
serve: all
	python -m SimpleHTTPServer
	#$(HTTPD_PY) -C $(CURDIR)
