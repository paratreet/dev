#include "pup.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
using namespace std;
#include "../../barnes1d.h"
#include "barnes.decl.h"

#define DEBUG(x) 

/* readonly */ CProxy_Main mainProxy;
/* readonly */ CProxy_BarnesTreePiece tpProxy;

class BarnesTreePiece : public CBase_BarnesTreePiece {
  public:
    BarnesNodeData node;
    BarnesConsumer<BarnesTreePiece, BarnesKey> *cons;
    //std::vector<BarnesNodeData> nodes;
    BarnesKey firstLeaf;
    BarnesKey treeSize;
    int remoteCounter = 0;
   
    /*
    BarnesTreePiece(std::vector<BarnesNodeData> tpnodes) {
      nodes.insert(nodes.end(), tpnodes.begin(), tpnodes.end())
    }
    */
    BarnesTreePiece(BarnesNodeData tpnode, BarnesKey firstLeaf, BarnesKey treeSize) : node(tpnode), firstLeaf(firstLeaf), treeSize(treeSize) {
      if (thisIndex >= firstLeaf)
        cons = new BarnesConsumer<BarnesTreePiece, BarnesKey>(*this, node);
    }

    void checkDone() {
      if (remoteCounter == 0) {
        DEBUG(CkPrintf("[%d]remoteCounter == 0\n", thisIndex);)
        if (thisIndex >= firstLeaf)
          DEBUG(CkPrintf("[%d]acc: %f\n", thisIndex, cons->acc);)
        contribute(CkCallback(CkReductionTarget(Main, done), mainProxy));
      }
    }

    void startWork() {
      DEBUG(CkPrintf("[%d]startWork()\n", thisIndex);)
      remoteCounter = 0;
      if (thisIndex >= firstLeaf)
        requestNode(1, *cons);
      checkDone();
    }

    BarnesTreePiece(CkMigrateMessage *m) {}

    template <class Consumer>
    void requestNode(BarnesKey bk, Consumer &c) {
      if (bk < 1 || bk >= treeSize)
        CkPrintf("BarnesParaTree: Requested INVALID tree node %d\n", (int)bk);
		  else if (bk == thisIndex) {
			  if (bk >= firstLeaf)
				  c.consumeLocalLeaf(node, bk);
			  else
				  c.consumeLocalNode(node, bk);
		  }
      else {
        remoteCounter++;
        thisProxy[bk].requestRemoteNode(bk, thisIndex);
      }
    }

    void requestRemoteNode(BarnesKey bk, int consIndex) {
      if (bk >= firstLeaf)
        thisProxy[consIndex].consumeRemoteLeaf(node, bk);
      else
        thisProxy[consIndex].consumeRemoteNode(node, bk);
    }

    void consumeRemoteNode(const BarnesNodeData &n, const BarnesKey &key) {
      remoteCounter--;
      cons->consumeLocalNode(n, key);
      checkDone();
    }

    void consumeRemoteLeaf(const BarnesLeafData &n, const BarnesKey &key) {
      remoteCounter--;
      cons->consumeLocalLeaf(n, key);
      checkDone();
    }
};

class Main : public CBase_Main {
  public:
    BarnesNodeData *tree;
    BarnesKey treeSize, treeRoot, firstLeaf;

  Main(CkArgMsg *m) {
    int depth = 20;
    treeSize = (BarnesKey)pow(2, depth);
    treeRoot = 1;
    firstLeaf = pow(2, depth-1);

    tree = new BarnesNodeData[treeSize];
    constructNode(treeRoot, 0.0, 100.0);

    mainProxy = thisProxy;
    tpProxy = CProxy_BarnesTreePiece::ckNew();

    for (int i = 1; i < treeSize; i++) {
      tpProxy[i].insert(tree[i], firstLeaf, treeSize);
    }
    tpProxy.doneInserting();
    CkPrintf("Main inserted tp array\n");

    tpProxy.startWork();
  }

  Main(CkMigrateMessage *m){}

  void done() {
    CkPrintf("DONE!!!\n");
    CkExit();
  }

  void constructNode(int index, float xMin, float xMax) {
    ///interior node
    if (2*index<treeSize) {
      float xMid = (xMin+xMax)/2;
      tree[index] = BarnesNodeData(20.0, xMid, xMin, xMax);

      //construct left child node
      constructNode(2*index, xMin, xMid);

      //construct right child node
      constructNode(2*index+1, xMid, xMax);
    }
    ///leaf node
    else {
      float random = ((float) rand()) / (float) RAND_MAX;
      float xPos= xMin + random*(xMax - xMin);

      //cout<<"Tree Leaf: Ind:"<<index<<" xPos:"<<xPos<<endl;
      tree[index] = BarnesNodeData(20.0, xPos, xMin, xMax);
    }
  }
};

#include "barnes.def.h"
