#include "pup.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
using namespace std;
#include "barnes1d.h"
#include "barnes.decl.h"

/// Define DEBUG(x) to x if you need to print out a lot of statements
#define DEBUG(x)

/* readonly */ CProxy_Main mainProxy;
/* readonly */ CProxy_BarnesTreePiece tpProxy;

/**
Trivial Barnes Tree piece
Each Treepiece stores a single tree node (either internal node or leaf)
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
          CkPrintf("[%d] Acceleration of particle : %f\n", thisIndex, cons->acc);
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
      requestKey(leftChild(*this,bk),c);
      requestKey(rightChild(*this,bk),c);
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

class Main : public CBase_Main {
  public:
    BarnesNodeData *tree;
    BarnesKey treeSize, treeRoot, firstLeaf;

  Main(CkArgMsg *m) {
    int depth = 10; // Number of levels in Barnes-Hut  tree
    treeSize = (BarnesKey)pow(2, depth);
    treeRoot = 1;
    firstLeaf = pow(2, depth-1);

    tree = new BarnesNodeData[treeSize];
    constructNode(treeRoot, 0.0, 100.0);

    mainProxy = thisProxy;
    tpProxy = CProxy_BarnesTreePiece::ckNew();

    // Dynamic insertion of treepieces
    for (int i = 1; i < treeSize; i++) {
      tpProxy[i].insert(tree[i], firstLeaf, treeSize);
    }
    // Finish insertion
    tpProxy.doneInserting();
    CkPrintf("[Main] Create tree-piece array\n");

    tpProxy.startWork();
  }

  Main(CkMigrateMessage *m){}

  /// Method called on reduction to indicate end of compuatations
  void done() {
    CkPrintf("[Main] Done with 1D Barnes-Hut computations\n");
    CkExit();
  }

  /// Construct the tree in an array form recursively
  void constructNode(int index, float xMin, float xMax) {
    // Interior node
    if (2*index<treeSize) {
      float xMid = (xMin+xMax)/2;
      tree[index] = BarnesNodeData(20.0, xMid, xMin, xMax);

      // Construct left child node
      constructNode(2*index, xMin, xMid);

      // Construct right child node
      constructNode(2*index+1, xMid, xMax);
    }
    // Leaf node
    else {
      float random = ((float) rand()) / (float) RAND_MAX;
      float xPos= xMin + random*(xMax - xMin);

      tree[index] = BarnesNodeData(20.0, xPos, xMin, xMax);
    }
  }
};

#include "barnes.def.h"
