all: barnes1d_cuda

barnes1d_cuda: barnes1d_cuda.cu *.h
	nvcc $< -o $@ -O3
