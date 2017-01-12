/* 3D barnes-hut example
	 CPU Code
*/

#ifndef __PARATREET_BARNES3D_CPUTREES
#define __PARATREET_BARNES3D_CPUTREES

/// Set to print out DEBUG statements
#define DEBUG(x) x

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
using namespace std;
#include "barnes3d.h"

/*
Barnes Hut Tree : Stores nodes in a dense array
*/
class BarnesParaTree{
	public:
	int depth;
	int size;
	BarnesKey firstLeaf;
	BarnesNodeData *tree;
	BarnesParaTree(int depth): depth(depth){
		size = (BarnesKey)((int)pow(8, depth)/7 + 1);
		tree = (BarnesNodeData*)malloc(sizeof(BarnesNodeData)*size);
		firstLeaf = (int)pow(8, depth)/56 + 1;
  }

  bool isLeaf(int index) {
    return (index >= firstLeaf);
  }

	//Process node requests and send back nodes/leaves
	template <class Consumer>
	void requestKey(BarnesKey bk, Consumer &c){
		if(bk<1 || bk>= size) printf("BarnesParaTree: Requested INVALID tree node %d\n", (int)bk);
		else{
			if(bk>=firstLeaf)
				c.consumeLeaf(tree[bk],bk);
			else
				c.consumeNode(tree[bk],bk);
		}
	}

	template <class Consumer>
	void requestChildren(BarnesKey bk, Consumer &c){
    for (int i = 0; i < 8; i++) {
      requestKey(getChild(*this, bk, i), c);
    }
	}

  /// Recursively construct tree
	void constructNode(int index, vector3d min, vector3d max){
    // Interior node
    if (!isLeaf(index)) {
      vector3d mid = (min+max)/2;
      tree[index] = BarnesNodeData(20.0, mid, min, max);

      constructNode(getChild(index, 0), vector3d(min.x,min.y,min.z), vector3d(mid.x,mid.y,mid.z));
      constructNode(getChild(index, 1), vector3d(mid.x,min.y,min.z), vector3d(max.x,mid.y,mid.z));
      constructNode(getChild(index, 2), vector3d(min.x,mid.y,min.z), vector3d(mid.x,max.y,mid.z));
      constructNode(getChild(index, 3), vector3d(mid.x,mid.y,min.z), vector3d(max.x,max.y,mid.z));

      constructNode(getChild(index, 4), vector3d(min.x,min.y,mid.z), vector3d(mid.x,mid.y,max.z));
      constructNode(getChild(index, 5), vector3d(mid.x,min.y,mid.z), vector3d(max.x,mid.y,max.z));
      constructNode(getChild(index, 6), vector3d(min.x,mid.y,mid.z), vector3d(mid.x,max.y,max.z));
      constructNode(getChild(index, 7), vector3d(mid.x,mid.y,mid.z), vector3d(max.x,max.y,max.z));

    }
    // Leaf node
    else {
      float random = ((float) rand()) / (float) RAND_MAX;
      vector3d pos = min + (max - min)*random;
      DEBUG(printf("[%d] Particle created : (%6.2f, %6.2f, %6.2f)\n",
          index, pos.x, pos.y, pos.z);)

      tree[index] = BarnesNodeData(20.0, pos, min, max);
    }

	}

	void printSubTree(int index){
		if(2*index+1<size){
			BarnesNodeData thisNode = tree[index];
			//DEBUG(cout<<"Tree Node: Ind:"<<index<<" xMin:"<<thisNode.xMin<<" xMax:"<<thisNode.xMax<<endl;)
			printSubTree(2*index);
			printSubTree(2*index+1);
		}
		else{
			BarnesLeafData thisleaf =  tree[index];
			//DEBUG(cout<<"Tree Leaf: Ind:"<<index<<" xPos:"<<thisleaf.x<<endl;)
		}
	}

};

#endif
