/* 1D ball-search example
	 CPU Code
*/

#ifndef __PARATREET_BALL1D_CPUTREES
#define __PARATREET_BALL1D_CPUTREES

/// Set to print out DEBUG statements
#define DEBUG(x) x

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
using namespace std;
#include "ball1d.h"

/*
Ball-Search Tree : Stores nodes in a dense array
*/
class BallParaTree{
	public:
	int depth;
	int size;
	BallKey firstLeaf;
	BallNodeData *node;
	BallParaTree(int depth): depth(depth){
		size = pow(2, depth+1);
		node = (BallNodeData*)malloc(sizeof(BallNodeData)*size);
		firstLeaf = pow(2, depth);
	}

	//Process node requests and send back nodes/leaves
	template <class Consumer>
	void requestKey(BallKey bk, Consumer &c){
		if(bk<1 || bk>= size) printf("BallParaTree: Requested INVALID tree node %d\n", (int)bk);
		else{
			if(bk>=firstLeaf)
				c.consumeLeaf(node[bk],bk);
			else
				c.consumeNode(node[bk],bk);
		}
	}

	template <class Consumer>
	void requestChildren(BallKey bk, Consumer &c){
		requestKey(leftChild(*this,bk),c);
		requestKey(rightChild(*this,bk),c);
	}

  /// Recursively construct tree
	void constructNode(int index, float xMin, float xMax){

		///interior node
		if(2*index<size){
			DEBUG(cout<<"Tree Node: Ind:"<<index<<" xMin:"<<xMin<<" xMax:"<<xMax<<endl;)

			float xMid = (xMin+xMax)/2;
			node[index] = BallNodeData(20.0, xMid, 0.0f, xMin, xMax);

			//construct left child node
			constructNode(2*index, xMin, xMid);

			//construct right child node
			constructNode(2*index+1, xMid, xMax);
		}
		///leaf node
		else{
			float random = ((float) rand()) / (float) RAND_MAX;
			float xPos= xMin + random*(xMax - xMin);

			cout<<"Tree Leaf: Ind:"<<index<<" xPos:"<<xPos<<endl;
			node[index] = BallNodeData(20.0, xPos, 25.0f, xMin, xMax);
		}
	}

	void printSubTree(int index){
		if(2*index+1<size){
			BallNodeData thisNode = node[index];
			DEBUG(cout<<"Tree Node: Ind:"<<index<<" xMin:"<<thisNode.xMin<<" xMax:"<<thisNode.xMax<<endl;)
			printSubTree(2*index);
			printSubTree(2*index+1);
		}
		else{
			BallLeafData thisleaf =  node[index];
			DEBUG(cout<<"Tree Leaf: Ind:"<<index<<" xPos:"<<thisleaf.x<<endl;)
		}
	}

};

#endif
