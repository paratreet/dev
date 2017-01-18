/** 
 1D FMM data structures and consumer example.
 
 The tree is a simple balanced binary tree as in barnes1d.
*/
#ifndef __PARATREET_FMM1D
#define __PARATREET_FMM1D

#include "paratreet.h"

/**
 key for tree nodes.
*/
typedef unsigned long FMMKey;

/// Get key of left child of parent nodes
template <class ParaTree>
CUDA_BOTH FMMKey leftChild(ParaTree &t,FMMKey parent) { return parent*2+0; }
/// Get key of right child of parent node
template <class ParaTree>
CUDA_BOTH FMMKey rightChild(ParaTree &t,FMMKey parent) { return parent*2+1; }


/**
 A FMM leaf: a particle (or list of particles) and local expansion.
*/
class FMMLeafData {
public:
  float mass;
  float x;
  /* local "multipole expansion"; just a constant force for this
     simple case */
  float local;

/// Packing-unpacking function needed for migrations in Charm++
#ifdef __CHARMC__
  void pup(PUP::er &p) {
    p|mass;
    p|x;
    p|local;
  }
#endif

  CUDA_BOTH FMMLeafData() {}
	
  CUDA_BOTH FMMLeafData(float mass,float x) :mass(mass), x(x), local(0.0) {}
}; 


/**
 An FMM tree interior node
*/
class FMMNodeData
	: public FMMLeafData // lumped mass and average position
{
public:
  float xMin, xMax; // range of size

/// Packing-unpacking function needed for migrations in Charm++
#ifdef __CHARMC__
  void pup(PUP::er &p) {
    FMMLeafData::pup(p);
    p|xMin;
    p|xMax;
  }
#endif

  CUDA_BOTH FMMNodeData() {}

  CUDA_BOTH FMMNodeData(float mass,float x,float xMin,float xMax) :FMMLeafData(mass,x), xMin(xMin), xMax(xMax) {}
};

/**
 An FMM tree data consumer.
*/
template <class ParaTree,class FMMKey>
struct FMMConsumer {
public:
	ParaTree &tree;
	const FMMNodeData &me;
	float acc;

	CUDA_BOTH FMMConsumer(ParaTree &tree,const FMMNodeData &me) 
		:tree(tree), me(me) 
	{
		acc=0.0f;
	}

	/// Add gravity from this object (node or leaf)
	inline CUDA_BOTH void addGravityFar(const FMMLeafData &l) {
		float G=1.0;
		float SOFTENING=0.00001; // force softening, to avoid divide by zero when evaluating self forces
		float r=l.x-me.x;
		float r3=(abs(r*r*r)+SOFTENING);
		float fm=G*l.mass*r/r3; // force divided by my mass
		TRACE_BARNES(printf("   gravity on %.0f from %.0f = %.3g (r3=%.2f)\n",me.x,l.x,fm,r3));
		acc+=fm;
	}

    /// Add Far expansion to Local expansion
    inline CUDA_BOTH void addFarToLocal(const FMMLeafData &l) {
        float SOFTENING=0.00001; // force softening, to avoid divide by zero when evaluating self forces
        float r=l.x-me.x;
        float r3=(abs(r*r*r)+SOFTENING);
        float fm=l.mass*r/r3; // force divided by my mass
        me.local += fm;
    }
    
    inline CUDA_BOTH void shiftLocal() {
    }
    
	
	/// Consume a tree node: recursively opens the node if nearby, or lumps it if distant.
	inline CUDA_BOTH void consumeNode(const FMMNodeData &n,const FMMKey &key) { 
            float openingThreshold=0.8;
            float radius=(n.xMax-n.xMin)/openingThreshold;
            float myRadius = 1.5*(me.xMax - me.xMin)/openingThreshold;
            float distance=fabs(me.x-n.x);
            if(distance > radius + myRadius) { // local expansion is
                                               // valid
                addFarToLocal(n);
                }
            else if(radius > myRadius) {  // open recursively
                tree.requestChildren(key,*this);
            }
            else if(tree.isLeaf(me)) {
                if(distance > radius) {
                    addGravityFar(n);
                }
                else {
                    tree.requestChildren(key,*this);
                }
            }
            else {
                // For each of my children:
                // Shift my local data to the child
                // Start walk with the child as sink
            }
	}

	/// Consume a tree leaf: just computes gravity.
	inline CUDA_BOTH void consumeLeaf(const FMMLeafData &l,const FMMKey &key) { 
            float openingThreshold=0.8;
            float myRadius = 1.5*(me.xMax - me.xMin)/openingThreshold;
            float distance=fabs(me.x-l.x);
            if(distance > myRadius) {
                addFarToLocal(l);
            }
            else if(tree.isLeaf(me)) {
                addGravityFar(l);
            }
            else {
                // For each of my children:
                // Shift my local data to the child
                // Start walk with the child as sink
            }
	}
	
};

#endif

