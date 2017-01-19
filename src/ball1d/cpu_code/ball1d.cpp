#include "ball1d_cputree.h"

int main(int argc, char *argv[]){

	//depth of the binary tree
	int depth = 3;
	BallKey treeRoot=1;

	//tree of the depth d
	BallParaTree t(depth);

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
			BallConsumer<typeof(t),BallKey> c(t, t.node[i]);
			t.requestKey(treeRoot, c);
			cout<<"Particle "<<i<<" has an neighbors : ";
            for (int i=0; i < c.neighbors.size(); i++)
                cout<<" "<<c.neighbors[i]<<" ";
            cout<<endl;
		}
	}
}
