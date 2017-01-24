/*
 CUDA-friendly tree implementation.
*/
#ifndef __PARATREET_BALL1D_CUDATREES
#define __PARATREET_BALL1D_CUDATREES
#include "ball1d.h"

/**
 Store tree nodes in a dense array.
 */
class BallParaTree {
public:
	const BallNodeData *n;
	BallKey firstLeaf, nTreeNodes;
	
	CUDA_BOTH BallParaTree(const BallNodeData *n,BallKey firstLeaf,BallKey nTreeNodes)
		:n(n), firstLeaf(firstLeaf), nTreeNodes(nTreeNodes)
	{}

	template <class Consumer>
	inline CUDA_BOTH void requestKey(const BallKey &bk,Consumer &c) {
#if SANITY_CHECKS
		// Sanity checks:
		if (bk<1 || bk>=nTreeNodes) printf("BallParaTree: Requested INVALID tree node %d\n",(int)bk);
		else
#endif
		  if (bk>=firstLeaf)
			c.consumeLeaf(n[bk],bk); // HACK!  typecast node to leaf (could save space with dedicated leaf array)
		else
			c.consumeNode(n[bk],bk);
	}

	template <class Consumer>
	inline CUDA_BOTH void requestChildren(const BallKey &bk,Consumer &c) {
    requestKey(leftChild(*this, bk), c);
    requestKey(rightChild(*this, bk), c);
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
  int maxDepth;
	
	Key stack[1000]; // FIXME: stack overflow? theoretical limit is equal to the number of leaves, but in practice it's a lot smaller because of the opening criteria
	int stackTop;
	CUDA_BOTH bool stackEmpty(void) { return stackTop < 0; }
	CUDA_BOTH void stackPush(const Key &b) {
		stackTop++;
    if (stackTop > maxDepth)
      maxDepth = stackTop;
		stack[stackTop] = b;
		TRACE_STACK(printf("[stack] pushing %ld to depth %d\n", stack[stackTop], stackTop));
	}
	CUDA_BOTH const Key &stackPop(void) {
		return stack[stackTop--];
	}

	CUDA_BOTH ManualStackTree(UnterTree &untertree) : untertree(untertree), stackTop(-1), maxDepth(-1) {}
	
	// To request a node, just push to the stack
	template <class Consumer>
	CUDA_BOTH void requestKey(const Key &key, Consumer &consumer) {
		stackPush(key);
	}

	// To request a node's children, just push them all to the stack
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
			TRACE_STACK(printf("[stack] popping %ld from depth %d\n", stack[stackTop], stackTop));
			Key k = stackPop(); // dereference key (subtle: stack can change as we process this key)
			untertree.requestKey(k,consumer);
		}
	}
};



#endif


