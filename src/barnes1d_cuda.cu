/**
  All-CUDA source code for 1D Barnes-Hut

*/
#include <iostream>
#include <vector>
#include <cuda.h>

#include "barnes1d_cudatree.h"

__global__ void tryTraverse(const BarnesNodeData *treeNodes,int firstLeaf,int nTreeNodes,float *acc) {
	int i=threadIdx.x+blockIdx.x*blockDim.x;
	if (i>=firstLeaf && i<nTreeNodes) {
#ifdef CUDA_USE_RECURSION
		BarnesParaTree tree(treeNodes,firstLeaf,nTreeNodes);
#else
		BarnesParaTree untertree(treeNodes,firstLeaf,nTreeNodes);
		ManualStackTree<BarnesKey,typeof(untertree)> tree(untertree);
#endif
		BarnesKey treeRoot=1;
		BarnesConsumer<typeof(tree),BarnesKey> c(tree,treeNodes[i]);
		
		// Expand the tree root into the consumer
		tree.requestNode(treeRoot,c);

#ifndef CUDA_USE_RECURSION	
		tree.iterateToConsumer(c);
#endif
		
		acc[i]=c.acc;
	}
}


#define check(cudacall) { int err=cudacall; if (err!=cudaSuccess) std::cout<<"CUDA ERROR "<<err<<" at line "<<__LINE__<<"'s "<<#cudacall<<"\n";}

int main() {
	// Build tree on CPU:
	std::vector<BarnesNodeData> n;
	n.push_back(BarnesNodeData(-666.0,-666.0,0.0,100.0)); // invalid entry 0
	
	n.push_back(BarnesNodeData(10.0,50.0,0.0,100.0)); // [1] root
	
	n.push_back(BarnesNodeData(3.0,25.0,0.0,50.0));	// first level
	n.push_back(BarnesNodeData(7.0,75.0,50.0,100.0));
	
	BarnesKey firstLeaf=n.size();
	
	n.push_back(BarnesNodeData(1.0,10.0,0.0,25));  // leaf level
	n.push_back(BarnesNodeData(2.0,40.0,25.0,50.0));
	n.push_back(BarnesNodeData(4.0,60.0,50.0,75));
	n.push_back(BarnesNodeData(3.0,85.0,75.0,100.0));
	
	
	// Copy tree to GPU
	BarnesNodeData *gn=0;
	check(cudaMalloc((void **)&gn, n.size()*sizeof(n[0])));
	check(cudaMemcpy(gn,&n[0],n.size()*sizeof(n[0]),cudaMemcpyHostToDevice));
	
	// Accelerations for each leaf:
	float *gacc=0;
	check(cudaMalloc((void **)&gacc, n.size()*sizeof(float)));
	
	// Walk tree on GPU
	tryTraverse<<<1,n.size()>>>(gn,firstLeaf,n.size(), gacc);
	
	// Copy accelerations back to CPU
	float *acc=new float[n.size()];
	check(cudaMemcpy(acc,gacc,n.size()*sizeof(float),cudaMemcpyDeviceToHost));
	
	// Print accelerations
	for (int i=firstLeaf;i<n.size();i++) {
		printf("Accel node %d, x=%.2f: %f\n", i, n[i].x, acc[i]);
	}
}


