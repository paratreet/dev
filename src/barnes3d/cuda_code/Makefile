OPTS=-O3 -g -std=c++11
INC=-I.. -I../..
PROGRAMS=barnes3d_cuda

all: $(PROGRAMS)

barnes3d_cuda: barnes3d_cuda.cu *.h
	nvcc $< -o $@ $(OPTS) $(INC) --gpu-architecture sm_30

clean:
	@rm -f $(PROGRAMS)

