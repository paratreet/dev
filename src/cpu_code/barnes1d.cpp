#include "barnes1d_cputree.h"

int main(int argc, char *argv[]){

	//depth of the binary tree
	int depth = 3;
	BarnesKey treeRoot=1;

	//tree of the depth d
	BarnesParaTree t(depth);

	//recursively construct tree starting from the root
	cout<<"*********BUILDING TREE*********\n";
	t.constructNode(treeRoot, 0.0, 100.0);

	//traverse the tree starting from the root
	DEBUG(cout<<"*********TRAVERSING TREE*********\n";)
	DEBUG(t.printSubTree(treeRoot);)

	DEBUG(cout<<"*********COMPUTING GRAVITY*********\n";)
	//Iterate over all leaves and compute their gravity and print accelerations
	for(int i=0;i<t.size;i++){
		//check if the node is a leaf
		if(i>=pow(2,depth)){
			BarnesConsumer<typeof(t),BarnesKey> c(t, t.node[i]);
			t.requestNode(treeRoot, c);
			cout<<"Particle "<<i<<" has an acceleration of "<<c.acc<<endl;
		}
	}
}
