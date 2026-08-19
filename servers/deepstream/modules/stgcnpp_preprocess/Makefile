CUDA_VER?=
ifeq ($(CUDA_VER),)
 $(error "CUDA_VER is not set")
endif

CXX:= g++
SRCS:= nvdspreprocess_lib.cpp
LIB:=libnvds_stgcnpp_preprocess.so

CFLAGS+= -fPIC -std=c++17 -Wall -Werror \
	-I /usr/local/cuda-$(CUDA_VER)/include \
	-I /opt/nvidia/deepstream/deepstream/sources/gst-plugins/include \
	-I /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include \
	-I /opt/nvidia/deepstream/deepstream/sources/includes

LIB_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream/lib/

LIBS := -shared -Wl,-no-undefined \
	-L/usr/local/cuda-$(CUDA_VER)/lib64/ -lcudart -ldl \
	-L$(LIB_INSTALL_DIR) -lnvdsgst_meta -lnvds_meta -lnvbufsurface -lnvbufsurftransform \
	-lcuda -Wl,-rpath,$(LIB_INSTALL_DIR)

OBJS:= $(SRCS:.cpp=.o)
PKGS:= gstreamer-1.0

CFLAGS+=$(shell pkg-config --cflags $(PKGS))
LIBS+=$(shell pkg-config --libs $(PKGS))

all: $(LIB)

%.o: %.cpp nvdspreprocess_lib.h Makefile
	$(CXX) -c -o $@ $(CFLAGS) $<

$(LIB): $(OBJS) Makefile
	$(CXX) -o $@ $(OBJS) $(LIBS)

install: $(LIB)
	cp -v $(LIB) $(LIB_INSTALL_DIR)

clean:
	rm -rf $(OBJS) $(LIB)
