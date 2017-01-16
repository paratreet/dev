#include "barnes3d_cputree.h"
#include <chrono>

int main(int argc, char *argv[]){

	//depth of the binary tree
	int depth = 3;
  if (argc == 2) {
    depth = atoi(argv[1]);
  }
	BarnesKey treeRoot=1;

	//tree of the depth d
	BarnesParaTree t(depth);

  // Record start time
  auto t1 = std::chrono::high_resolution_clock::now();

	//recursively construct tree starting from the root
	DEBUG(cout<<"*********BUILDING TREE*********\n";)
	t.constructNode(treeRoot, vector3d(0.0, 0.0, 0.0), vector3d(100.0, 100.0, 100.0));

	//traverse the tree starting from the root
	DEBUG(cout<<"*********TRAVERSING TREE*********\n";)
	DEBUG(t.printSubTree(treeRoot);)

	DEBUG(cout<<"*********COMPUTING GRAVITY*********\n";)
	//Iterate over all leaves and compute their gravity and print accelerations
	for(int i=0;i<t.size;i++){
		//check if the node is a leaf
		if(t.isLeaf(i)){
			BarnesConsumer<__typeof__(t),BarnesKey> c(t, t.tree[i]);
			t.requestKey(treeRoot, c);
			DEBUG(cout<<"Particle "<<i<<" has an acceleration of "<<c.acc<<endl;)
		}
	}

  // Record end time
  auto t2 = std::chrono::high_resolution_clock::now();
  auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Print time
  std::cout << "Execution time: " << t_diff << " ms" << std::endl;
}
