/** 
 * 3D barnes-hut data structures and consumer example.
 */
#ifndef __PARATREET_BARNES3D
#define __PARATREET_BARNES3D

#include "paratreet.h"
#include <cmath>
#include <cstdlib>

/**
 * Barnes-Hut key for tree nodes.
 */
typedef unsigned long BarnesKey;

/// Get child of given index (0-7)
CUDA_BOTH inline BarnesKey getChild(BarnesKey parent, int index) {
  return (parent * 8 - 6 + index);
}
template <class ParaTree>
CUDA_BOTH inline BarnesKey getChild(ParaTree &t, BarnesKey parent, int index) {
  return getChild(parent, index);
}

/**
 * Struct for using 3d vectors.
 */
struct vector3d {
  float x;
  float y;
  float z;

  vector3d() {}
  
  vector3d(float x_, float y_, float z_) {
    x = x_; y = y_; z = z_;
  }

  vector3d operator+(const vector3d& rhs) {
    vector3d v;
    v.x = this->x + rhs.x;
    v.y = this->y + rhs.y;
    v.z = this->z + rhs.z;
    return v;
  }

  vector3d operator-(const vector3d& rhs) {
    vector3d v;
    v.x = this->x - rhs.x;
    v.y = this->y - rhs.y;
    v.z = this->z - rhs.z;
    return v;
  }

  vector3d operator/(const int& div) {
    vector3d v;
    v.x = this->x / div;
    v.y = this->y / div;
    v.z = this->z / div;
    return v;
  }

  vector3d operator*(const float& mul) {
    vector3d v;
    v.x = this->x * mul;
    v.y = this->y * mul;
    v.z = this->z * mul;
    return v;
  }

#ifdef __CHARMC__
  void pup(PUP::er &p) {
    p|x; p|y; p|z;
  }
#endif
};

/**
 * A Barnes-Hut leaf: a particle (or list of particles).
 */
class BarnesLeafData {
public:
  float mass;
  vector3d pos;

/// Packing-unpacking function needed for migrations in Charm++
#ifdef __CHARMC__
  void pup(PUP::er &p) {
    p|mass;
    p|pos;
  }
#endif

  CUDA_BOTH BarnesLeafData() {}
	
  CUDA_BOTH BarnesLeafData(float mass, vector3d pos) :mass(mass), pos(pos) {}
}; 


/**
 * A Barnes-Hut tree interior node
 */
class BarnesNodeData
	: public BarnesLeafData // lumped mass and average position
{
public:
  vector3d min, max; // range of particle positions

/// Packing-unpacking function needed for migrations in Charm++
#ifdef __CHARMC__
  void pup(PUP::er &p) {
    BarnesLeafData::pup(p);
    p|min;
    p|max;
  }
#endif

  CUDA_BOTH BarnesNodeData() {}

  CUDA_BOTH BarnesNodeData(float mass, vector3d pos, vector3d min, vector3d max) :
    BarnesLeafData(mass, pos), min(min), max(max) {}
};

/**
 * A Barnes-Hut tree data consumer: computes gravity on nodes and leaves of the tree.
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
		float G = 1.0;
		float SOFTENING = 0.00001; // force softening, to avoid divide by zero when evaluating self forces
		float r = sqrt(pow(l.pos.x - me.pos.x,2) + pow(l.pos.y - me.pos.y,2) + pow(l.pos.z - me.pos.z,2));
		float r3 = (abs(r*r*r)+SOFTENING);
		float fm = G*l.mass*r/r3; // force divided by my mass
		TRACE_BARNES(printf("   gravity on (%6.2f, %6.2f, %6.2f) from (%6.2f, %6.2f, %6.2f) = %.3g (r3=%.2f)\n",
          me.pos.x, me.pos.y, me.pos.z, l.pos.x, l.pos.y, l.pos.z, fm, r3));
		acc += fm;
	}
	
	/// Consume a tree node: recursively opens the node if nearby, or lumps it if distant.
	inline CUDA_BOTH void consumeNode(const BarnesNodeData &n, const BarnesKey &key) { 
		float radius = sqrt(pow(n.max.x - n.pos.x,2) + pow(n.max.y - n.pos.y,2) + pow(n.max.z - n.pos.z,2));
		float distance = sqrt(pow(me.pos.x - n.pos.x,2) + pow(me.pos.y - n.pos.y,2) + pow(me.pos.z - n.pos.z,2));
		float angularSize = radius/abs(distance);
		float openingThreshold = 0.8;
		
		if (angularSize > openingThreshold) { // open recursively
			TRACE_BARNES(printf("Me = (%6.2f, %6.2f, %6.2f), opening node %d (angular %.2f)\n",
            me.pos.x, me.pos.y, me.pos.z, key, angularSize));
			tree.requestChildren(key, *this);
		} else { // compute acceleration to lumped centroid
			TRACE_BARNES(printf("Me = (%6.2f, %6.2f, %6.2f), lumping gravity from (%6.2f, %6.2f, %6.2f) (angular %.2f)\n",
            me.pos.x, me.pos.y, me.pos.z, n.pos.x, n.pos.y, n.pos.z, angularSize));
			addGravity(n);
		}
	}

	/// Consume a tree leaf: just computes gravity.
	inline CUDA_BOTH void consumeLeaf(const BarnesLeafData &l, const BarnesKey &key) { 
		TRACE_BARNES(printf("Me = (%6.2f, %6.2f, %6.2f), leaf gravity from (%6.2f, %6.2f, %6.2f)\n",
          me.pos.x, me.pos.y, me.pos.z, l.pos.x, l.pos.y, l.pos.z));
		addGravity(l);
	}
	
};

#endif
