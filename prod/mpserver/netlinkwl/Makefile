export LD_LIBRARY_PATH=/projects/hnd/tools/bcm/crosstools-arm-gcc-5.5-linux-4.1-glibc-2.26-binutils-2.28.1/lib
CC=/projects/hnd/tools/bcm/crosstools-arm-gcc-5.5-linux-4.1-glibc-2.26-binutils-2.28.1/usr/bin/arm-buildroot-linux-gnueabi-gcc


NETLINKWL_OBJ = netlinkwl
CFLAGS += -Werror -Wall

all:$(NETLINKWL_OBJ)


$(NETLINKWL_OBJ): netlinkwl.c
	$(CC) netlinkwl.c -o $@

clean: 
	-rm $(NETLINKWL_OBJ)
