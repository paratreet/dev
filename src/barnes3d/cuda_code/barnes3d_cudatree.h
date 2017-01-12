/*
 CUDA-friendly tree implementation.
 
*/
#ifndef __PARATREET_BARNES3D_CUDATREES
#define __PARATREET_BARNES3D_CUDATREES
#include "barnes3d.h"

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
	inline CUDA_BOTH void requestKey(const BarnesKey &bk,Consumer &c) {
#if SANITY_CHECKS
		// Sanity checks:
		if (bk<1 || bk>=nTreeNodes) printf("BarnesParaTree: Requested INVALID tree node %d\n",(int)bk);
		else
#endif
		  if (bk>=firstLeaf)
			c.consumeLeaf(n[bk],bk); // HACK!  typecast node to leaf (could save space with dedicated leaf array)
		else
			c.consumeNode(n[bk],bk);
	}

	template <class Consumer>
	inline CUDA_BOTH void requestChildren(const BarnesKey &bk,Consumer &c) {
    for (int i = 0; i < 8; i++) {
      requestKey(getChild(*this, bk, i), c);
    }
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
	
	Key stack[10]; // FIXME: stack size is equal to the number of leaves, which can be large for deep trees
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
	CUDA_BOTH void requestKey(const Key &key, Consumer &consumer) {
		stackPush(key);
	}

	// To request a node's children, just push them both to the stack
	template <class Consumer>
	CUDA_BOTH void requestChildren(const Key &key, Consumer &consumer) {
    for (int i = 0; i < 8; i++) {
      stackPush(getChild(*this, key, i));
    }
	}
	
	// To finish servicing a consumer, keep popping nodes
	template <class Consumer>
	CUDA_BOTH void iterateToConsumer(Consumer &consumer) {
		while (!stackEmpty()) {
			TRACE_STACK(printf("		Stack depth %d: %ld top\n",stackTop,stack[stackTop]));
			Key k=stackPop(); // dereference key (subtle: stack can change as we process this key)
			untertree.requestKey(k,consumer);
		}
	}
};



#endif


