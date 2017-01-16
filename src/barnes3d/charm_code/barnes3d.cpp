#include "pup.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
using namespace std;
#include "barnes3d.h"
#include "barnes.decl.h"

/// Define DEBUG(x) to x if you need to print out a lot of statements
#define DEBUG(x) //x
#define MYDEBUG(x) //x

/* readonly */ CProxy_Main mainProxy;
/* readonly */ CProxy_BarnesTreePiece tpProxy;

/**
 * Trivial Barnes TreePiece.
 * Each TreePiece stores a single tree node (either internal node or leaf).
*/
class BarnesTreePiece : public CBase_BarnesTreePiece {
  public:
    /// Tree node (1 node for 1 tree piece)
    BarnesNodeData node;

    /// Consumer (only initialized for a leaf)
    BarnesConsumer<BarnesTreePiece, BarnesKey> *cons;

    /// Index of the first leaf
    BarnesKey firstLeaf;

    /// Size of the tree
    BarnesKey treeSize;

    /// Counter to keep a track of number of remote node requests sent
    int remoteCounter = 0;

    BarnesTreePiece(BarnesNodeData tpnode, BarnesKey firstLeaf, BarnesKey treeSize) : node(tpnode), firstLeaf(firstLeaf), treeSize(treeSize) {
      /// Create a constructor only if the node is a leaf
      if (thisIndex >= firstLeaf)
        cons = new BarnesConsumer<BarnesTreePiece, BarnesKey>(*this, node);
    }

    /// Check if all remote requests have completed
    void checkDone() {
      if (remoteCounter == 0) {
        DEBUG(CkPrintf("[%d]remoteCounter == 0\n", thisIndex);)
        if (thisIndex >= firstLeaf){
          MYDEBUG(CkPrintf("[%d] Acceleration of particle : %f\n", thisIndex, cons->acc);)
        }
        contribute(CkCallback(CkReductionTarget(Main, done), mainProxy));
      }
    }

    /// Begin computation
    void startWork() {
      DEBUG(CkPrintf("[%d]startWork()\n", thisIndex);)
      remoteCounter = 0;
      if (thisIndex >= firstLeaf)
        requestKey(1, *cons);
      checkDone();
    }

    BarnesTreePiece(CkMigrateMessage *m) {}

    /// Method called to request a node
    template <class Consumer>
    void requestKey(const BarnesKey &bk, Consumer &c) {
      if (bk < 1 || bk >= treeSize)
        CkPrintf("BarnesParaTree: Requested INVALID tree node %d\n", (int)bk);
      else if (bk == thisIndex) {   //Local node
        if (bk >= firstLeaf)
          c.consumeLeaf(node, bk);  //Call consumer's leaf method
        else
          c.consumeNode(node, bk);  //Call consumer's node method
      }
      else { //Remote node
        remoteCounter++;
        //Send a remote node request
        thisProxy[bk].requestRemoteNode(bk, thisIndex);
      }
    }

    template <class Consumer>
    void requestChildren(const BarnesKey &bk, Consumer &c) {
      for (int i = 0; i < 8; i++) {
        requestKey(getChild(*this, bk, i), c);
      }
    }

    /// Entry method called to request for a remote node
    void requestRemoteNode(BarnesKey bk, int consIndex) {
      if (bk >= firstLeaf)
        thisProxy[consIndex].consumeRemoteLeaf(node, bk);
      else
        thisProxy[consIndex].consumeRemoteNode(node, bk);
    }

    /// Entry method called to respond to a remote node request which is an internal node
    void consumeRemoteNode(const BarnesNodeData &n, const BarnesKey &key) {
      remoteCounter--;
      cons->consumeNode(n, key); //Call consumer's node method now that remote node is available
      checkDone();
    }

    /// Entry method called to respond to a remote node request which is a leaf node
    void consumeRemoteLeaf(const BarnesLeafData &n, const BarnesKey &key) {
      remoteCounter--;
      cons->consumeLeaf(n, key); //Call consumer's leaf method now that remote leaf is available
      checkDone();
    }
};

/*
 * Main
 */
class Main : public CBase_Main {
  public:
    BarnesNodeData *tree;
    BarnesKey treeSize; // total number of nodes
    BarnesKey treeRoot, firstLeaf;
    double startTime;

  Main(CkArgMsg *m) {
    int depth = 3; // depth of tree
    if (m->argc == 2) {
      depth = atoi(m->argv[1]);
    }

    treeSize = (BarnesKey)((int)pow(8, depth)/7 + 1);
    treeRoot = 1;
    firstLeaf = (int)pow(8, depth)/56 + 1;

    tree = new BarnesNodeData[treeSize];
    vector3d min = {0.0, 0.0, 0.0};
    vector3d max = {100.0, 100.0, 100.0};
    constructNode(treeRoot, min, max);

    mainProxy = thisProxy;
    tpProxy = CProxy_BarnesTreePiece::ckNew();

    // Dynamic insertion of treepieces
    for (int i = 1; i < treeSize; i++) {
      tpProxy[i].insert(tree[i], firstLeaf, treeSize);
    }
    // Finish insertion
    tpProxy.doneInserting();
    CkPrintf("[Main] Create tree-piece array\n");

    startTime = CkWallTimer();
    tpProxy.startWork();
  }

  Main(CkMigrateMessage *m){}

  /// Method called on reduction to indicate end of compuatations
  void done() {
    CkPrintf("[Main] Elapsed time: %lf\n", CkWallTimer() - startTime);
    CkPrintf("[Main] Done with 3D Barnes-Hut computations\n");
    CkExit();
  }

  bool isLeaf(int index) {
    return (index >= firstLeaf);
  }
  

  /// Construct the tree in an array form recursively
  void constructNode(int index, vector3d min, vector3d max) {
    // Interior node
    if (!isLeaf(index)) {
      vector3d mid = (min+max)/2;
      tree[index] = BarnesNodeData(20.0, mid, min, max);

      constructNode(getChild(index, 0), vector3d(min.x,min.y,min.z), vector3d(mid.x,mid.y,mid.z));
      constructNode(getChild(index, 1), vector3d(mid.x,min.y,min.z), vector3d(max.x,mid.y,mid.z));
      constructNode(getChild(index, 2), vector3d(min.x,mid.y,min.z), vector3d(mid.x,max.y,mid.z));
      constructNode(getChild(index, 3), vector3d(mid.x,mid.y,min.z), vector3d(max.x,max.y,mid.z));

      constructNode(getChild(index, 4), vector3d(min.x,min.y,mid.z), vector3d(mid.x,mid.y,max.z));
      constructNode(getChild(index, 5), vector3d(mid.x,min.y,mid.z), vector3d(max.x,mid.y,max.z));
      constructNode(getChild(index, 6), vector3d(min.x,mid.y,mid.z), vector3d(mid.x,max.y,max.z));
      constructNode(getChild(index, 7), vector3d(mid.x,mid.y,mid.z), vector3d(max.x,max.y,max.z));

    }
    // Leaf node
    else {
      float random = ((float) rand()) / (float) RAND_MAX;
      vector3d pos = min + (max - min)*random;
      DEBUG(printf("[%d] Particle created : (%6.2f, %6.2f, %6.2f)\n",
          index, pos.x, pos.y, pos.z);)

      tree[index] = BarnesNodeData(20.0, pos, min, max);
    }
  }
};

#include "barnes.def.h"
