/* 1D barnes-hut example 

*/
#ifndef __PARATREET_BARNES1D
#define __PARATREET_BARNES1D

#include "paratreet.h"


/**
 A Barnes-Hut leaf: a particle (or list of particles).
*/
class BarnesLeafData {
public:
	float mass;
	float x;
	
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
	
	CUDA_BOTH BarnesNodeData(float mass,float x,float xMin,float xMax) :BarnesLeafData(mass,x), xMin(xMin), xMax(xMax) {}
};


typedef unsigned long BarnesKey;
template <class ParaTree>
CUDA_BOTH BarnesKey leftChild(const ParaTree &t,BarnesKey parent) { return parent*2+0; }
template <class ParaTree>
CUDA_BOTH BarnesKey rightChild(const ParaTree &t,BarnesKey parent) { return parent*2+1; }


/**
 A Barnes-Hut tree data consumer.
*/
template <class ParaTree>
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
	
	/// Consume a local tree node
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

	/// Consume a local tree leaf
	inline CUDA_BOTH void consumeLocalLeaf(const BarnesLeafData &l,const BarnesKey &key) { 
		TRACE_BARNES(printf("Me = %.0f, leaf gravity from %.0f\n",me.x,l.x));
		addGravity(l);
	}
	
};

/**
 Store tree nodes in a dense array.
 */
class BarnesParaTree {
public:
	const BarnesNodeData *n;
	BarnesKey firstLeaf, nTreeNodes;
	
	CUDA_BOTH BarnesParaTree(const BarnesNodeData *n,BarnesKey firstLeaf,BarnesKey nTreeNodes) 
		:n(n), firstLeaf(firstLeaf), nTreeNodes(nTreeNodes) 
	{}
	
	template <class Consumer>
	inline CUDA_BOTH void requestNode(const BarnesKey &bk,Consumer &c) {
#if SANITY_CHECKS
		// Sanity checks:
		if (bk<1 || bk>=nTreeNodes) printf("BarnesParaTree: Requested INVALID tree node %d\n",(int)bk);
		else 
#endif
		  if (bk>=firstLeaf) 
			c.consumeLocalLeaf(n[bk],bk); // HACK!  typecast node to leaf (could save space with dedicated leaf array)
		else 
			c.consumeLocalNode(n[bk],bk);
	}
};


/**
 Shim tree class: to avoid recursion, push requests to stack.
 Iteratively pull nodes off the stack to walk tree.
*/
template <class Key, class UnterTree>
class ManualStackTree {
public:
	UnterTree &untertree;
	
	Key stack[5];
	int stackTop;
	CUDA_BOTH bool stackEmpty(void) { return stackTop<0; }
	CUDA_BOTH void stackPush(const Key &b) {
		stackTop++;
		stack[stackTop]=b;
		TRACE_STACK(printf("		Stack pushing to depth %d: %ld new top\n",stackTop,stack[stackTop]));
	}
	CUDA_BOTH const Key &stackPop(void) {
		return stack[stackTop--];
	}

	CUDA_BOTH ManualStackTree(UnterTree &untertree) :untertree(untertree), stackTop(-1) {}
	
	// To request a node, just push to the stack
	template <class Consumer>
	CUDA_BOTH void requestNode(const Key &key, Consumer &consumer) {
		stackPush(key);
	}
	
	// To finish servicing a consumer, keep popping nodes
	template <class Consumer>
	CUDA_BOTH void iterateToConsumer(Consumer &consumer) {
		while (!stackEmpty()) {
			TRACE_STACK(printf("		Stack depth %d: %ld top\n",stackTop,stack[stackTop]));
			Key k=stackPop(); // dereference key (subtle: stack can change as we process this key)
			untertree.requestNode(k,consumer);
		}
	}
};



#endif

