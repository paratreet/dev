#include "pup.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
using namespace std;
#include "ball1d.h"
#include "ball.decl.h"

/// Define DEBUG(x) to x if you need to print out a lot of statements
#define DEBUG(x)

/* readonly */ CProxy_Main mainProxy;
/* readonly */ CProxy_BallTreePiece tpProxy;

/**
Trivial Ball Tree piece
Each Treepiece stores a single tree node (either internal node or leaf)
*/
class BallTreePiece : public CBase_BallTreePiece {
  public:
    /// Tree node (1 node for 1 tree piece)
    BallNodeData node;

    /// Consumer (only initialized for a leaf)
    BallConsumer<BallTreePiece, BallKey> *cons;

    /// Index of the first leaf
    BallKey firstLeaf;

    /// Size of the tree
    BallKey treeSize;

    /// Counter to keep a track of number of remote node requests sent
    int remoteCounter = 0;

    BallTreePiece(BallNodeData tpnode, BallKey firstLeaf, BallKey treeSize) : node(tpnode), firstLeaf(firstLeaf), treeSize(treeSize) {
      /// Create a constructor only if the node is a leaf
      if (thisIndex >= firstLeaf)
        cons = new BallConsumer<BallTreePiece, BallKey>(*this, node);
    }

    /// Check if all remote requests have completed
    void checkDone() {
      if (remoteCounter == 0) {
        DEBUG(CkPrintf("[%d]remoteCounter == 0\n", thisIndex);)
        if (thisIndex >= firstLeaf){
          cout << "Particle " << thisIndex << " has neighbors:";
          for (int i = 0; i < cons->neighbors.size(); i++)
          cout << " " << cons->neighbors[i];
          cout << endl;
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

    BallTreePiece(CkMigrateMessage *m) {}

    /// Method called to request a node
    template <class Consumer>
    void requestKey(const BallKey &bk, Consumer &c) {
      if (bk < 1 || bk >= treeSize)
        CkPrintf("BallParaTree: Requested INVALID tree node %d\n", (int)bk);
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
    void requestChildren(const BallKey &bk, Consumer &c) {
      requestKey(leftChild(*this,bk),c);
      requestKey(rightChild(*this,bk),c);
    }

    /// Entry method called to request for a remote node
    void requestRemoteNode(BallKey bk, int consIndex) {
      if (bk >= firstLeaf)
        thisProxy[consIndex].consumeRemoteLeaf(node, bk);
      else
        thisProxy[consIndex].consumeRemoteNode(node, bk);
    }

    /// Entry method called to respond to a remote node request which is an internal node
    void consumeRemoteNode(const BallNodeData &n, const BallKey &key) {
      remoteCounter--;
      cons->consumeNode(n, key); //Call consumer's node method now that remote node is available
      checkDone();
    }

    /// Entry method called to respond to a remote node request which is a leaf node
    void consumeRemoteLeaf(const BallLeafData &n, const BallKey &key) {
      remoteCounter--;
      cons->consumeLeaf(n, key); //Call consumer's leaf method now that remote leaf is available
      checkDone();
    }
};

class Main : public CBase_Main {
  public:
    BallNodeData *tree;
    BallKey treeSize, treeRoot, firstLeaf;

  Main(CkArgMsg *m) {
    int depth = 3;
    if (m->argc == 2) {
      depth = atoi(m->argv[1]);
    }
    treeSize = (BallKey)pow(2, depth);
    treeRoot = 1;
    firstLeaf = pow(2, depth-1);

    tree = new BallNodeData[treeSize];
    constructNode(treeRoot, 0.0, 100.0);

    mainProxy = thisProxy;
    tpProxy = CProxy_BallTreePiece::ckNew();

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
    CkPrintf("[Main] Done with 1D Ball-Search computations\n");
    CkExit();
  }

  /// Construct the tree in an array form recursively
  void constructNode(int index, float xMin, float xMax) {
    // Interior node
    if (2*index<treeSize) {
			cout<<"Tree Node: Ind:"<<index<<" xMin:"<<xMin<<" xMax:"<<xMax<<endl;
      float xMid = (xMin+xMax)/2;
      tree[index] = BallNodeData(20.0, xMid, 0.0f, xMin, xMax);

      // Construct left child node
      constructNode(2*index, xMin, xMid);

      // Construct right child node
      constructNode(2*index+1, xMid, xMax);
    }
    // Leaf node
    else {
      float random = ((float) rand()) / (float) RAND_MAX;
      float xPos= xMin + random*(xMax - xMin);

			cout<<"Tree Leaf: Ind:"<<index<<" xPos:"<<xPos<<endl;
      tree[index] = BallNodeData(20.0, xPos, 25.0f, xMin, xMax);
    }
  }
};

#include "ball.def.h"
