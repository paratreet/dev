/** 
 1D barnes-hut data structures and consumer example.
 
*/
#ifndef __PARATREET_BARNES1D
#define __PARATREET_BARNES1D

#include "paratreet.h"

/**
Barnes-Hut key for tree nodes.
*/
typedef unsigned long BarnesKey;

/// Get key of left child of parent nodes
template <class ParaTree>
BarnesKey leftChild(ParaTree &t,BarnesKey parent) { return parent*2+0; }
/// Get key of right child of parent node
template <class ParaTree>
BarnesKey rightChild(ParaTree &t,BarnesKey parent) { return parent*2+1; }


/**
 A Barnes-Hut leaf: a particle (or list of particles).
*/
class BarnesLeafData {
public:
	float mass;
	float x;

/// Packing-unpacking function needed for migrations in Charm++
#ifdef __CHARMC__
  void pup(PUP::er &p) {
    p|mass;
    p|x;
  }
#endif

  CUDA_BOTH BarnesLeafData() {}
	
	CUDA_BOTH BarnesLeafData(float mass,float x) :mass(mass), x(x) {}
}; 


/**
 A Barnes-Hut tree interior node
*/
class BarnesNodeData
	: public BarnesLeafData // lumped mass and average position
{
public:
	float xMin, xMax; // range of size

/// Packing-unpacking function needed for migrations in Charm++
#ifdef __CHARMC__
  void pup(PUP::er &p) {
    BarnesLeafData::pup(p);
    p|xMin;
    p|xMax;
  }
#endif

  CUDA_BOTH BarnesNodeData() {}
	
	CUDA_BOTH BarnesNodeData(float mass,float x,float xMin,float xMax) :BarnesLeafData(mass,x), xMin(xMin), xMax(xMax) {}
};

/**
 A Barnes-Hut tree data consumer: computes gravity on nodes and leaves of the tree.
*/
template <class ParaTree,class BarnesKey>
struct BarnesConsumer {
public:
	ParaTree &tree;
	const BarnesLeafData &me;
	float acc;

	CUDA_BOTH BarnesConsumer(ParaTree &tree,const BarnesLeafData &me) 
		:tree(tree), me(me) 
	{
		acc=0.0f;
	}

	/// Add gravity from this object (node or leaf)
	inline CUDA_BOTH void addGravity(const BarnesLeafData &l) {
		float G=1.0;
		float SOFTENING=0.00001; // force softening, to avoid divide by zero when evaluating self forces
		float r=l.x-me.x;
		float r3=(abs(r*r*r)+SOFTENING);
		float fm=G*l.mass*r/r3; // force divided by my mass
		TRACE_BARNES(printf("   gravity on %.0f from %.0f = %.3g (r3=%.2f)\n",me.x,l.x,fm,r3));
		acc+=fm;
	}
	
	/// Consume a local tree node: recursively opens the node if nearby, or lumps it if distant.
	inline CUDA_BOTH void consumeLocalNode(const BarnesNodeData &n,const BarnesKey &key) { 
		float radius=n.xMax-n.x;
		float distance=me.x-n.x;
		float angularSize=radius/abs(distance);
		float openingThreshold=0.8;
		
		if (angularSize>openingThreshold) { // open recursively
			TRACE_BARNES(printf("Me = %.0f, opening node %d (angular %.2f)\n",me.x,key,angularSize));
			tree.requestNode(leftChild(tree,key),*this);
			tree.requestNode(rightChild(tree,key),*this);
		} else { // compute acceleration to lumped centroid
			TRACE_BARNES(printf("Me = %.0f, lumping gravity from %.0f (angular %.2f)\n",me.x,n.x,angularSize));
			addGravity(n);
		}
	}

	/// Consume a local tree leaf: just computes gravity.
	inline CUDA_BOTH void consumeLocalLeaf(const BarnesLeafData &l,const BarnesKey &key) { 
		TRACE_BARNES(printf("Me = %.0f, leaf gravity from %.0f\n",me.x,l.x));
		addGravity(l);
	}
	
};

#endif

