/*
 CUDA-friendly tree implementation.
 
*/
#ifndef __PARATREET_BARNES1D_CUDATREES
#define __PARATREET_BARNES1D_CUDATREES
#include "barnes1d.h"


typedef unsigned long BarnesKey;
template <class ParaTree>
CUDA_BOTH BarnesKey leftChild(const ParaTree &t,BarnesKey parent) { return parent*2+0; }
template <class ParaTree>
CUDA_BOTH BarnesKey rightChild(const ParaTree &t,BarnesKey parent) { return parent*2+1; }

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


