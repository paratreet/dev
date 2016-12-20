
PROGRAMS=barnes1d_cuda

all: $(PROGRAMS)

barnes1d_cuda: barnes1d_cuda.cu *.h
	nvcc $< -o $@ -O3 --gpu-architecture sm_30

clean:
	@rm -f $(PROGRAMS)

