OPTS=-O3 -g -std=c++11
INC=-I.. -I../..
PROGRAMS=ball1d_cuda

all: $(PROGRAMS)

ball1d_cuda: ball1d_cuda.cu *.h
	nvcc $< -o $@ $(OPTS) $(INC) --gpu-architecture sm_30

clean:
	@rm -f $(PROGRAMS)

