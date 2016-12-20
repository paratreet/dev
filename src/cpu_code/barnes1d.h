/* 1D barnes-hut example
	 CPU Code
*/

#ifndef __PARATREET_BARNES1D
#define __PARATREET_BARNES1D

/// Set to print out DEBUG statements
#define DEBUG(x) x

#include "../paratreet.h"
#include <stdio.h>
#include <cmath>
#include <iostream>
#include <cstdlib>

using namespace std;

/*
Barnes Hut Leaf
*/
class BarnesLeafData {
	public:
	float mass;
	float x;
	bool isLeaf;
	BarnesLeafData(){}
	BarnesLeafData(float pMass, float pX){
		isLeaf = true;
		mass = pMass;
		x = pX;
	}
	void print(){
		cout<<"Leaf"<<mass<<"\t"<<x<<endl;
	}
};


/**
Barnes Hut Tree Node
*/
class BarnesNodeData : public BarnesLeafData { //lumped mass and average position
	public:
	float xMin, xMax; // range of size
	BarnesNodeData(){}
	BarnesNodeData(float pMass, float pXMin, float pXMax) :
													BarnesLeafData(pMass, (pXMin + pXMax)/2){
		BarnesLeafData::isLeaf = false;
		xMin = pXMin;
		xMax = pXMax;
	}
	BarnesNodeData(const BarnesLeafData& data){
		this->mass = data.mass;
		this->x = data.x;
		this->isLeaf = data.isLeaf;
	}
	void print(){
		cout<<"Node:"<<mass<<"\t"<<xMin<<"\t"<<xMax<<endl;
	}
};

/**
Key to identifiy a node : index in the array
*/
typedef unsigned long BarnesKey;
template <class ParaTree>
BarnesKey leftChild(ParaTree &t,BarnesKey parent) { return parent*2+0; }
template <class ParaTree>
BarnesKey rightChild(ParaTree &t,BarnesKey parent) { return parent*2+1; }

/**
 Barnes Hut tree data consumer
*/
template <class ParaTree>
struct BarnesConsumer {
	public:
		const BarnesLeafData &me;
		float acc;
		BarnesConsumer(const BarnesLeafData &me):me(me){
			acc=0.0f;
		}

		/// Add gravity from this object (node or leaf)
	  template <class N>
		void addGravity(const N &n){
			float G=1.0;
			float SOFTENING=0.000001;
			float r= n.x - me.x;
			float r3 = (abs(r*r*r)+SOFTENING);
			float fm = (G* n.mass * r)/r3;
			acc+=fm;
		}

		/// Consume a local tree node
		void consumeLocalNode(BarnesNodeData &n, const BarnesKey &key, ParaTree &tree){
			float radius = n.xMax - n.x;
			float distance = me.x - n.x;
			float angularSize = radius/abs(distance);
			float openingThreshold = 0.8;
			// open recursively
			if (angularSize>openingThreshold) {
				printf("Me = %.0f, opening node %lu (angular %.2f)\n",me.x,key,angularSize);
				tree.requestNode(leftChild(tree,key),*this);
				tree.requestNode(rightChild(tree,key),*this);
			}
			// compute acceleration to lumped centroid
			else {
				printf("Me = %.0f, lumping gravity from %.0f (angular %.2f)\n",me.x,n.x,angularSize);
				addGravity(n);
			}
		}

		/// Consume a local tree leaf
		void consumeLocalLeaf(BarnesLeafData &l, BarnesKey &key, ParaTree &tree){
		printf("Me = %0.f, leaf gravity from %0.f\n", me.x, l.x);
		addGravity(l);
	}
};

/*
Barnes Hut Tree : Stores nodes in a dense array
*/
class BarnesParaTree{
	public:
	int depth;
	int size;

	BarnesNodeData *node;
	BarnesParaTree(int depth): depth(depth){
		size = pow(2, depth+1);
		node = new BarnesNodeData[size];
	}

	template <class Consumer>
	void requestNode(BarnesKey bk, Consumer &c){
		if(node[bk].isLeaf)
			c.consumeLocalLeaf(node[bk],bk, *this);
		else
			c.consumeLocalNode(node[bk],bk, *this);
	}

  /// Recursively construct tree
	void constructNode(int index, float xMin, float xMax){

		///interior node
		if(2*index<size){
			DEBUG(cout<<"Tree Node: Ind:"<<index<<" xMin:"<<xMin<<" xMax:"<<xMax<<endl;)
			node[index].print();


			float xMid = (xMin+xMax)/2;

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
			node[index] = BarnesLeafData(20.0, xPos);
			node[index].BarnesLeafData::print();
		}
	}

	void printSubTree(int index){
		if(2*index+1<size){
			BarnesNodeData thisNode = node[index];
			DEBUG(cout<<"Tree Node: Ind:"<<index<<" xMin:"<<thisNode.xMin<<" xMax:"<<thisNode.xMax<<endl;)
			printSubTree(2*index);
			printSubTree(2*index+1);
		}
		else{
			BarnesLeafData thisleaf =  node[index];
			DEBUG(cout<<"Tree Leaf: Ind:"<<index<<" xPos:"<<thisleaf.x<<endl;)
		}
	}

};

#endif
