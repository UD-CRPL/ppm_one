EXE=ppm_stable

CC=pgcc
CXX=pgc++

CFLAGS=   -g -O0 -ta=tesla:cc70 -Minfo=accel
CXXFLAGS= -g -O0 -std=c++11 -ta=tesla:cc70 -Minfo=accel

OUT_DIR=build

INC_COMMON= -I./openacc/ -I./common/ -I./common/common/ -I./common/munkres/
INC_LMMIN= -I./openacc/ -I./lmmin/
INC_PPM= -I./openacc/ -I./ppm/ -I./ppm/ppm/

COMMONLIB=common
MUNKRESLIB=munkres
LMMINLIB=lmmin

LIB_COMMON= -L./$(OUT_DIR)/ -l$(COMMONLIB) -l$(MUNKRESLIB)
LIB_LMMIN= -L./$(OUT_DIR)/ -l$(LMMINLIB)

DEPS=common/config.h \
     common/munkres/matrix.h \
     common/munkres/munkres.h \
     common/common/aa.h \
     common/common/bmrb.h \
     common/common/pdb.h \
     common/common/rdc.h \
     common/common/supply.h \
     common/common/traj.h \
     lmmin/ann.h \
     lmmin/config.h \
     lmmin/lmcurve.h \
     lmmin/lmmin.h \
     lmmin/lmstruct.h \
     ppm/config.h \
     ppm/ppm/data.h \
     ppm/ppm/mainbody.h \
     openacc/debug.h

all: builddir munkres common lmmin ppm

builddir:
	mkdir -p $(OUT_DIR)

clean:
	rm -f common/common/*.o common/munkres/*.o lmmin/*.o ppm/ppm/*.o $(OUT_DIR)/*.a $(OUT_DIR)/$(EXE)

munkres: common/munkres/matrix.o common/munkres/munkres.o
	ar rcs $(OUT_DIR)/lib$(MUNKRESLIB).a $^
	rm -f common/munkres/*.o

common: common/common/aa.o common/common/bmrb.o common/common/pdb.o common/common/rdc.o common/common/supply.o common/common/traj.o
	ar rcs $(OUT_DIR)/lib$(COMMONLIB).a $^
	rm -f common/common/*.o

lmmin: lmmin/ann.o lmmin/lmcurve.o lmmin/lmmin.o
	ar rcs $(OUT_DIR)/lib$(LMMINLIB).a $^
	rm -f lmmin/*.o

ppm: ppm/ppm/mainbody.o ppm/ppm/main.o
	$(CXX) $(CXXFLAGS) -o $(OUT_DIR)/$(EXE) $^ $(INC_COMMON) $(INC_LMMIN) $(INC_PPM) $(LIB_COMMON) $(LIB_LMMIN)
	rm -f ppm/ppm/*.o

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(INC_COMMON) $(INC_LMMIN) $(INC_PPM) -c -o $@ $<

%.o: %.c $(DEPS)
	$(CC) $(CCFLAGS) $(INC_COMMON) $(INC_LMMIN) $(INC_PPM) -c -o $@ $<

