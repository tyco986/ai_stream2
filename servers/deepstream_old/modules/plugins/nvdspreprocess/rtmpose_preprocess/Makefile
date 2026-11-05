CUDA_VER?=
ifeq ($(CUDA_VER),)
 $(error "CUDA_VER is not set")
endif

CXX:= g++
NVCC:= /usr/local/cuda-$(CUDA_VER)/bin/nvcc
SRCS:= nvdspreprocess_lib.cpp
CUSRC:= rgba_to_nchw.cu
LIB:=libnvds_rtmpose_preprocess.so

CFLAGS+= -fPIC -std=c++17 -Wall -Werror \
	-I /usr/local/cuda-$(CUDA_VER)/include \
	-I /opt/nvidia/deepstream/deepstream/sources/gst-plugins/include \
	-I /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include \
	-I /opt/nvidia/deepstream/deepstream/sources/includes \
	-I .

LIB_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream/lib/

LIBS := -shared -Wl,-no-undefined \
	-L/usr/local/cuda-$(CUDA_VER)/lib64/ -lcudart -ldl \
	-L$(LIB_INSTALL_DIR) -lnvdsgst_meta -lnvds_meta -lnvbufsurface -lnvbufsurftransform \
	-lcuda -Wl,-rpath,$(LIB_INSTALL_DIR)

OBJS:= nvdspreprocess_lib.o rgba_to_nchw.o
PKGS:= gstreamer-1.0

CFLAGS+=$(shell pkg-config --cflags $(PKGS))
LIBS+=$(shell pkg-config --libs $(PKGS))

all: $(LIB)

nvdspreprocess_lib.o: nvdspreprocess_lib.cpp nvdspreprocess_lib.h rect_expand.hpp Makefile
	$(CXX) -c -o $@ $(CFLAGS) $<

rgba_to_nchw.o: rgba_to_nchw.cu rgba_to_nchw.h Makefile
	$(NVCC) -c -o $@ -Xcompiler -fPIC -std=c++17 \
		-I /usr/local/cuda-$(CUDA_VER)/include $<

$(LIB): $(OBJS) Makefile
	$(CXX) -o $@ $(OBJS) $(LIBS)

install: $(LIB)
	cp -v $(LIB) $(LIB_INSTALL_DIR)

clean:
	rm -rf $(OBJS) $(LIB)
