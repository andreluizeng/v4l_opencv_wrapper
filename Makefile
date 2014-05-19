#before buildint it don't forget to export ROOTF_DIR and CROSS_COMPILER


APPNAME 		= camera_test
TOOLCHAIN		= /opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/
CROSS_COMPILER		= $(TOOLCHAIN)/bin/arm-fsl-linux-gnueabi-

CC			= $(CROSS_COMPILER)gcc
DEL_FILE		= rm -rf
CP_FILE			= cp -rf      

TARGET_PATH_LIB 	= $(ROOTFS_DIR)/usr/lib
TARGET_PATH_INCLUDE 	= $(ROOTFS_DIR)/usr/include

CFLAGS			= -DLINUX -DUSE_SOC_MX6 -Wall -O2 -fsigned-char -march=armv7-a -mfpu=neon -mfloat-abi=softfp \
			  -DEGL_API_FB -DGPU_TYPE_VIV -DGL_GLEXT_PROTOTYPES -DENABLE_GPU_RENDER_20 \
			  -I../include -I$(ROOTFS_DIR)/usr/src/linux/include -I$(TARGET_PATH_INCLUDE) \

LFLAGS			= -Wl,--library-path=$(TARGET_PATH_LIB),-rpath-link=$(TARGET_PATH_LIB) -lm  -lpthread \
			   -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml
			  
OBJECTS			= $(APPNAME).o v4l_wrapper.o

first: all

all: $(APPNAME)

$(APPNAME): $(OBJECTS) 
	$(CC) $(LFLAGS) -o $(APPNAME) $(OBJECTS)  

$(APPNAME).o: $(APPNAME).c
	$(CC) $(CFLAGS) -c -o $(APPNAME).o $(APPNAME).c 

v4l_wrapper.o: v4l_wrapper.c
	$(CC) $(CFLAGS) -c -o v4l_wrapper.o v4l_wrapper.c 


clean:
	$(DEL_FILE) $(OBJECTS)
	$(DEL_FILE) *~ *.core
	$(DEL_FILE) $(APPNAME)

distclean: clean
	$(DEL_FILE) $(APPNAME)

install: all
	$(DEL_FILE) $(APPNAME)

