#include "barnes1d.h"

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


	//Iterate over all leaves and compute their gravity and print accelerations
	for(int i=0;i<t.size;i++){
		if(t.node[i].isLeaf){
			BarnesConsumer<typeof(t)> c(t.node[i]);
			t.requestNode(treeRoot, c);
			cout<<"Particle "<<i<<" has an acceleration of "<<c.acc<<endl;
		}
	}
}
