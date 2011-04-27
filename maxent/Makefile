# path of liblbfgs
LIBLBFGS=$(HOME)/Dev/773proj/tools/liblbfgs-build/lib
# must use g++ >= 4.4
CXX=g++-4.6

# Usually you don't need to touch this
CC=$(CXX)
CXXFLAGS=-std=c++0x -Wall -I../tools/liblbfgs-build/include -mtune=native -O2
LDFLAGS=-L$(LIBLBFGS) -llbfgs -Wl,-rpath,$(LIBLBFGS)
TARGET=maxent_predict maxent_train latent_log_linear_train latent_log_linear_predict

all:$(TARGET)

clean_data:data.o

maxent_predict:maxent.o data.o
maxent_train:maxent.o data.o

latent_log_linear_train:latent_log_linear.o data.o
latent_log_linear_predict:latent_log_linear.o data.o
repeat_lll:latent_log_linear.o data.o

maxent.o:data.o

%.o:%.hh

.PHONY:clean
clean:
	rm -f *.o $(TARGET)
