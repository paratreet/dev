/**
 Main example header for ParaTreeT: the parallel tree toolkit.

*/
#ifndef __PARATREET_MAIN_HEADER
#define __PARATREET_MAIN_HEADER

#include <stdio.h>

/** CUDA_ONLY(keywords) makes keywords appear only if the code is compiled with CUDA. */
#ifdef __CUDACC__
#  define CUDA_ONLY(x) x __host__ __device__
#else
#  define CUDA_ONLY(x) /* empty */
#endif

/** CUDABOTH makes a method available on both CPU and GPU */
#define CUDA_BOTH CUDA_ONLY(__host__ __device__)

#define TRACE_STACK(print) /* print */
#define TRACE_BARNES(print) /* print */


/** Set this to 1 to enable runtime checking */
#define SANITY_CHECKS 1


namespace ParaTreeT {

/**
 The idiomatic definition of the read side of a ParaTree:
 basically just provides an interface for Consumers to request data.
*/
template <class Key,class NodeData,class LeafData>
class ExampleParaTree {
public:
	/// This consumer requests the node/leaf data at this key
	template <class Consumer>
	void requestKey(const Key &key, Consumer &consumer);

	/// This consumer requests the subkeys of this key
	template <class Consumer>
	void requestChildren(const Key &key, Consumer &consumer);
};

/**
 A Consumer receives parts of a tree and performs some computation on them.
 ParaTrees are templated on the consumer.

 Due to the magic of templates, probably nobody needs to actually use this class,
 but it's an example of what the Consumer interface provides.
*/
template <class Key, class NodeData, class LeafData>
class ExampleConsumer {
public:
	/// Consume a tree node.  We pass the key of this node, just in case you need it.
	inline void consumeNode(NodeData &n,const Key &key) { /* do node physics here */ }
	/// Consume a tree leaf
	inline void consumeLeaf(LeafData &l,const Key &key) { /* do leaf physics here */ }
};

};


#endif

