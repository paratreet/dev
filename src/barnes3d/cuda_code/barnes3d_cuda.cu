/**
  All-CUDA source code for 3D Barnes-Hut
 */
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cuda.h>

#include "barnes3d_cudatree.h"

#define CUDA_USE_RECURSION // FIXME: use recursion

/**
  Kernel for GPU traversal
 */
__global__ void tryTraverse(const BarnesNodeData *treeNodes, int firstLeaf, int nTreeNodes, float *acc) {
  int tid = (gridDim.x * blockIdx.y + blockIdx.x) * blockDim.x + threadIdx.x;
  int i = firstLeaf + tid;
  if (i < nTreeNodes) {
#ifdef CUDA_USE_RECURSION
    BarnesParaTree tree(treeNodes,firstLeaf,nTreeNodes);
#else
    BarnesParaTree untertree(treeNodes, firstLeaf, nTreeNodes);
    ManualStackTree<BarnesKey,typeof(untertree)> tree(untertree);
#endif
    BarnesKey treeRoot=1;
    BarnesConsumer<typeof(tree),BarnesKey> c(tree,treeNodes[i]);

    // Expand the tree root into the consumer
    tree.requestKey(treeRoot,c);

#ifndef CUDA_USE_RECURSION	
    tree.iterateToConsumer(c);
#endif

    acc[i] = c.acc;
  }
}

#define check(cudacall) { int err=cudacall; if (err!=cudaSuccess) std::cout<<"CUDA ERROR "<<err<<" at line "<<__LINE__<<"'s "<<#cudacall<<"\n";}

/**
  Holder for nodes data
 */
BarnesNodeData *h_nodes;
BarnesKey firstLeaf;

inline bool isLeaf(int index) {
  return (index >= firstLeaf);
}

/**
  Recursively construct tree: not a member function 
  of BarnesParaTree, as deep copy would be necessary
  to pass a complete BarnesParaTree to the GPU.
  So BarnesParaTree is built instead on the GPU with
  the nodes data passed as an array from the CPU.
 */
void constructNodeArray(int index, vector3d min, vector3d max){
  // Interior node
  if (!isLeaf(index)) {
    vector3d mid = (min+max)/2;
    h_nodes[index] = BarnesNodeData(20.0, mid, min, max);

    constructNodeArray(getChild(index, 0), vector3d(min.x,min.y,min.z), vector3d(mid.x,mid.y,mid.z));
    constructNodeArray(getChild(index, 1), vector3d(mid.x,min.y,min.z), vector3d(max.x,mid.y,mid.z));
    constructNodeArray(getChild(index, 2), vector3d(min.x,mid.y,min.z), vector3d(mid.x,max.y,mid.z));
    constructNodeArray(getChild(index, 3), vector3d(mid.x,mid.y,min.z), vector3d(max.x,max.y,mid.z));

    constructNodeArray(getChild(index, 4), vector3d(min.x,min.y,mid.z), vector3d(mid.x,mid.y,max.z));
    constructNodeArray(getChild(index, 5), vector3d(mid.x,min.y,mid.z), vector3d(max.x,mid.y,max.z));
    constructNodeArray(getChild(index, 6), vector3d(min.x,mid.y,mid.z), vector3d(mid.x,max.y,max.z));
    constructNodeArray(getChild(index, 7), vector3d(mid.x,mid.y,mid.z), vector3d(max.x,max.y,max.z));

  }
  // Leaf node
  else {
    float random = ((float) rand()) / (float) RAND_MAX;
    vector3d pos = min + (max - min)*random;
    TRACE_BARNES(printf("[%d] Particle created : (%6.2f, %6.2f, %6.2f)\n",
          index, pos.x, pos.y, pos.z));

    h_nodes[index] = BarnesNodeData(20.0, pos, min, max);
  }
}

int main(int argc, char** argv) {
  // Parameters
  int depth = 3;
  if (argc >= 2) {
    depth = atoi(argv[1]);
  }
  int treeSize = (int)pow(8, depth)/7 + 1;
  firstLeaf = (BarnesKey)((int)pow(8, depth)/56 + 1);
  int leafCount = (int)pow(8, depth-1);

  // Memory allocation on host
  h_nodes = (BarnesNodeData *)malloc(sizeof(BarnesNodeData) * treeSize);
  float *h_acc = (float*)malloc(sizeof(float) * treeSize);

  // Create nodes
  constructNodeArray(1, vector3d(0.0f, 0.0f, 0.0f), vector3d(100.0f, 100.0f, 100.0f));

  // Record start time
  auto t1 = std::chrono::high_resolution_clock::now();

  // Memory allocation on device
  float *d_acc;
  BarnesNodeData *d_nodes;
  check(cudaMalloc((void **)&d_acc, sizeof(float) * treeSize));
  check(cudaMalloc((void **)&d_nodes, sizeof(BarnesNodeData) * treeSize));

  // Copy nodes data to device
  check(cudaMemcpy(d_nodes, h_nodes, sizeof(BarnesNodeData) * treeSize, cudaMemcpyHostToDevice));

  // Accelerations for each leaf
  // Each leaf does top-down traversal on device
  if ((int)ceil(sqrt(leafCount+255/256)) >= 65536) {
    std::cout << "Too many nodes, grid size overflow" << std::endl;
    return -1;
  }
  dim3 blocks((int)ceil(sqrt(leafCount+255/256)), (int)ceil(sqrt(leafCount+255/256)), 0);
  tryTraverse<<<blocks,min(256,leafCount)>>>(d_nodes, firstLeaf, treeSize, d_acc);

  // Copy accelerations back to host
  check(cudaMemcpy(h_acc, d_acc, sizeof(float) * treeSize, cudaMemcpyDeviceToHost));

  // Print accelerations
  for (int i = firstLeaf; i < treeSize; i++) {
    TRACE_BARNES(printf("Accel node %d, pos=(%6.2f, %6.2f, %6.2f): %f\n", i, h_nodes[i].pos.x, h_nodes[i].pos.y, h_nodes[i].pos.z, h_acc[i]));
  }

  // Free device memory
  check(cudaFree(d_acc));
  check(cudaFree(d_nodes));

  // Record end time
  auto t2 = std::chrono::high_resolution_clock::now();
  auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Free host memory
  free(h_nodes);
  free(h_acc);

  // End timing
  std::cout << "Execution time: " << t_diff << " ms" << std::endl;
}
