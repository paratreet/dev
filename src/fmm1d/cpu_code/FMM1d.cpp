#include "FMM1d_cputree.h"

int main(int argc, char *argv[]){

	//depth of the binary tree
	int depth = 3;
	FMMKey treeRoot=1;

	//tree of the depth d
	FMMParaTree t(depth);

	//recursively construct tree starting from the root
	cout<<"*********BUILDING TREE*********\n";
	t.constructNode(treeRoot, 0.0, 100.0);

	//traverse the tree starting from the root
	DEBUG(cout<<"*********TRAVERSING TREE*********\n";)
	DEBUG(t.printSubTree(treeRoot);)

	DEBUG(cout<<"*********COMPUTING GRAVITY*********\n";)
        // Interact tree with itself
        FMMConsumer<typeof(t),FMMKey> c(t, t.node[treeRoot]);
        t.requestKey(treeRoot, c);
        
	//Iterate over all leaves and finalize their gravity and print accelerations
	for(int i=0;i<t.size;i++){
		//check if the node is a leaf
		if(i>=pow(2,depth)){
                        c.acc += c.me.local;
			cout<<"Particle "<<i<<" has an acceleration of "<<c.acc<<endl;
		}
	}
}
