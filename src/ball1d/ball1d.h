/**
 1D particle ball-search data structures and consumer example.

*/
#ifndef __PARATREET_BALL1D
#define __PARATREET_BALL1D

#include "paratreet.h"
#include <vector>

/**
Ball search key for tree nodes.
*/
typedef unsigned long BallKey;

/// Get key of left child of parent nodes
template <class ParaTree>
CUDA_BOTH BallKey leftChild(ParaTree &t,BallKey parent) { return parent*2+0; }
/// Get key of right child of parent node
template <class ParaTree>
CUDA_BOTH BallKey rightChild(ParaTree &t,BallKey parent) { return parent*2+1; }


/**
 A Ball-Search leaf: a particle (or list of particles).
*/
class BallLeafData {
public:
  float mass;
  float x;
  float searchRadius;

/// Packing-unpacking function needed for migrations in Charm++
#ifdef __CHARMC__
  void pup(PUP::er &p) {
    p|mass;
    p|x;
    p|searchRadius;
  }
#endif

  CUDA_BOTH BallLeafData() {}
	
  CUDA_BOTH BallLeafData(float mass,float x,float searchRadius) :mass(mass), x(x), searchRadius(searchRadius) {}
}; 


/**
 A Ball-Search tree interior node
*/
class BallNodeData
	: public BallLeafData // lumped mass and average position
{
public:
  float xMin, xMax; // range of size

/// Packing-unpacking function needed for migrations in Charm++
#ifdef __CHARMC__
  void pup(PUP::er &p) {
    BallLeafData::pup(p);
    p|xMin;
    p|xMax;
  }
#endif

  CUDA_BOTH BallNodeData() {}

  CUDA_BOTH BallNodeData(float mass,float x,float radius,float xMin,float xMax) :BallLeafData(mass,x,radius), xMin(xMin), xMax(xMax) {}
};

/**
 A Ball-Search tree data consumer: fixed-radius search for neighbours on nodes and leaves of the tree.
*/
template <class ParaTree,class BallKey>
struct BallConsumer {
public:
	ParaTree &tree;
	const BallLeafData &me;
    float searchRangeStart;
    float searchRangeEnd;
    std::vector<BallKey> neighbors;

	CUDA_BOTH BallConsumer(ParaTree &tree,const BallLeafData &me)
		:tree(tree), me(me)
	{
        searchRangeStart = me.x - me.searchRadius;
        searchRangeEnd = me.x + me.searchRadius;
    }

	/// Consume a tree node: recursively opens the node if nearby, or lumps it if distant.
	inline CUDA_BOTH void consumeNode(const BallNodeData &n,const BallKey &key) {

        if (searchRangeStart > n.xMax || searchRangeEnd < n.xMin) {
            // out-of-bounds
            return;
        }

        if (searchRangeEnd >= n.xMin && searchRangeEnd <= n.x) {
            // contained in left child range
			TRACE_BARNES(printf("Me = %.0f, opening node %d \n",me.x,key));
            tree.requestKey(leftChild(tree,key),*this);
        }
        else if (searchRangeStart >= n.x && searchRangeStart <= n.xMax) {
            // contained in right child range
			TRACE_BARNES(printf("Me = %.0f, opening node %d \n",me.x,key));
            tree.requestKey(rightChild(tree,key),*this);
        }
        else {
            // intersected range, open both
			TRACE_BARNES(printf("Me = %.0f, opening node %d \n",me.x,key));
            tree.requestKey(leftChild(tree,key),*this);
            tree.requestKey(rightChild(tree,key),*this);
        }

	}

	/// Consume a tree leaf: just computes gravity.
	inline CUDA_BOTH void consumeLeaf(const BallLeafData &l,const BallKey &key) {
		TRACE_BARNES(printf("Me = %.0f, leaf gravity from %.0f\n",me.x,l.x));

        if (l.x >= searchRangeStart && l.x <= searchRangeEnd) {
            neighbors.push_back(key);
        }
	}

};

#endif

