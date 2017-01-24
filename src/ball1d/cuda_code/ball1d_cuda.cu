/**
  All-CUDA source code for 3D Ball-Hut
 */
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cuda.h>

#include "ball1d_cudatree.h"

#define CUDA_USE_RECURSION // FIXME: use recursion

/**
  Kernel for GPU traversal
 */
__global__ void tryTraverse(const BallNodeData *treeNodes, int firstLeaf, int nTreeNodes, float *acc) {
  int tid = (gridDim.x * blockIdx.y + blockIdx.x) * blockDim.x + threadIdx.x;
  int i = firstLeaf + tid;
  if (i < nTreeNodes) {
#ifdef CUDA_USE_RECURSION
    BallParaTree tree(treeNodes,firstLeaf,nTreeNodes);
#else
    BallParaTree untertree(treeNodes, firstLeaf, nTreeNodes);
    ManualStackTree<BallKey,typeof(untertree)> tree(untertree);
#endif
    BallKey treeRoot=1;
    BallConsumer<typeof(tree),BallKey> c(tree,treeNodes[i]);

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
BallNodeData *h_nodes;
BallKey firstLeaf;

inline bool isLeaf(int index) {
  return (index >= firstLeaf);
}

/**
  Recursively construct tree: not a member function 
  of BallParaTree, as deep copy would be necessary
  to pass a complete BallParaTree to the GPU.
  So BallParaTree is built instead on the GPU with
  the nodes data passed as an array from the CPU.
 */
void constructNodeArray(int index, float min, float max){
  // Interior node
  if (!isLeaf(index)) {
    float mid = (min+max)/2;
    h_nodes[index] = BallNodeData(20.0, mid, 0.0f, min, max);

    constructNodeArray(2*index, min, mid); // left child
    constructNodeArray(2*index+1, mid, max); // right child
  }
  // Leaf node
  else {
    float random = ((float) rand()) / (float) RAND_MAX;
    float pos = min + (max - min)*random;
    TRACE_BARNES(printf("[%d] Particle created : %6.2f\n",
          index, pos));

    h_nodes[index] = BallNodeData(20.0, pos, 25.0f, min, max);
  }
}

int main(int argc, char** argv) {
  // Parameters
  int depth = 3;
  if (argc >= 2) {
    depth = atoi(argv[1]);
  }

  // Record start time
  auto t1 = std::chrono::high_resolution_clock::now();

  // Calculate tree-related values
  int treeSize = (int)pow(2, depth+1);
  firstLeaf = (BallKey)pow(2, depth);
  int leafCount = (int)pow(2, depth-1);

  // Memory allocation on host
  h_nodes = (BallNodeData *)malloc(sizeof(BallNodeData) * treeSize);
  float *h_acc = (float*)malloc(sizeof(float) * treeSize);

  // Create nodes
  constructNodeArray(1, 0.0f, 100.0f);

  // Memory allocation on device
  float *d_acc;
  BallNodeData *d_nodes;
  check(cudaMalloc((void **)&d_acc, sizeof(float) * treeSize));
  check(cudaMalloc((void **)&d_nodes, sizeof(BallNodeData) * treeSize));

  // Copy nodes data to device
  check(cudaMemcpy(d_nodes, h_nodes, sizeof(BallNodeData) * treeSize, cudaMemcpyHostToDevice));

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
    TRACE_BARNES(printf("Accel node %d, pos=%6.2f: %f\n", i, h_nodes[i].pos, h_acc[i]));
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

  // Print time
  std::cout << "Execution time: " << t_diff << " ms" << std::endl;
}
