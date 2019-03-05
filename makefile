#可执行文件的名字
TARGET=test

#TESTMODE=1

ifdef TESTMODE 
CC=gcc
CXX=g++
else
CC=arm-linux-androideabi-gcc
CXX=arm-linux-androideabi-g++
#CC=gcc
endif

CSOURCE= $(wildcard *.c)

COBJECT= $(patsubst %.c,%.o,$(CSOURCE))
#OBJECT+= $(patsubst %.cpp,%.o,$(SOURCE))
#编译选项
CFLAGS  = -DSTDC_HEADERS=1 -DHAVE_STRING_H=1 -DHAVE_ALLOCA_H=1 


CXXFLAGS=

CXXSOURCE= $(wildcard *.cpp)
CXXOBJECT= $(patsubst %.cpp,%.o,$(CXXSOURCE))

OBJECT= $(COBJECT) $(CXXOBJECT)
#链接选项

ifdef TESTMODE 
LDFLAGS = 
else
LDFLAGS = -pie -fPIE
#CC=gcc
endif


#LDFLAGS = -pie -fPIE
#LDFLAGS = -pie -fPIE
#引用的库,库文件路径 前面加上-L -l 等
LIBPATH = -L 
LIBA =
LIBSO = -l
#指定头文件目录

LINKCC= $(CXX) $(LDFLAGS)

COMPCC= $(CC) $(CFLAGS)
COMPCXX= $(CXX) $(CXXFLAGS)
all: $(TARGET)
$(TARGET): $(OBJECT)
	$(LINKCC) -o $@ $^ $(LIBPATH) $(LIBSO) 
..c.o:  
	$(COMPCC) -c $< 
..cpp.o:
	$(COMPCXX) -c $< 
	
clean:
		rm *.o test
#arm-linux-androideabi-gcc -DSTDC_HEADERS -DHAVE_STRING_H -pie -fPIE demo.c regex.c -o demo -I ~/Desktop/bluetooth_pipe
