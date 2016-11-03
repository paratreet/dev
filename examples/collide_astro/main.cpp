/*
Simple Charm++ astro style collision detection program--
Dr. Orion Lawlor, lawlor@alaska.edu, 2016-11-02 (Public Domain)
 */
#include <stdio.h>
#include <string.h>
#include "collidecharm.h"
#include "collide_astro.decl.h"

CProxy_main mid;
CProxy_Hello arr;
int nElements;

void printCollisionHandler(void *param,int nColl,Collision *colls)
{
	CkPrintf("**********************************************\n");
	CkPrintf("*** Collision handler called-- %d records:\n",nColl);
	int nPrint=nColl;
	const int maxPrint=30;
	if (nPrint>maxPrint) nPrint=maxPrint;
	for (int c=0;c<nPrint;c++) {
		CkPrintf("%d:%d hits %d:%d\n",
			colls[c].A.chunk,colls[c].A.number,
			colls[c].B.chunk,colls[c].B.number);
	}
	CkPrintf("**********************************************\n");
	mid.maindone();
}

class main : public CBase_main
{
public:
  int ntimes;
  main(CkMigrateMessage *m) {}
  main(CkArgMsg* m)
  {
    nElements=4*CkNumPes();
    if(m->argc > 1) nElements = atoi(m->argv[1]);
    delete m;
    CkPrintf("Running collide on %d processors for %d array elements\n",
	     CkNumPes(),nElements);
    mid = thishandle;
    
    CkVector3d origin(0,0,0);
    double km=1.0e3;
    CkVector3d gridcell(10.0*km,10.0*km,10.0*km);
    CollideGrid3d gridMap(origin,gridcell);
    CollideHandle collide=CollideCreate(gridMap,
    	CollideSerialClient(printCollisionHandler,0));
    
    
    ntimes=0;
    arr = CProxy_Hello::ckNew(collide,nElements);
    arr.DoIt();
  };

  void maindone(void)
  {
    if (++ntimes>10) {
	    CkPrintf("All done\n");
	    CkExit();
	} else {
		arr.DoIt();
	}
  };
};


class Particle {
public:
	CkVector3d P,V; // position (m), velocity (m/s)
	float mass; // (Kg)
	float radius; // (meters)
	
	void pup(PUP::er &p) {
		p|P; p|V;
		p|mass; p|radius;
	}
	
};

// Return a random double-precision value from -1.0 to +1.0
double get_rand_double(void) {
	int randmax=0xfffff;
	double v=0.0;
	for (int rounds=0;rounds<3;rounds++) {		
		v+=rand()&randmax; // add new bits
		v*=1.0/randmax; // shift down to unit scale
	}
	return v*2.0-1.0;
}

class Hello : public CBase_Hello
{
	CollideHandle collide;
	std::vector<Particle> particles;
	std::vector<bbox3d> boxes;
public:
  Hello(const CollideHandle &collide_) :collide(collide_)
  {
	  CkPrintf("Creating element %d on PE %d\n",thisIndex,CkMyPe());
	  CollideRegister(collide,thisIndex);
	  
	  // Create particle data
	  int nParticles=10000;
	  srand(1000+thisIndex);
	  double au=1.5e11; // approx 1 astronomial unit, in meters
	  CkVector3d pancake(5.0*au,5.0*au,0.001*au);
	  for (int i=0;i<nParticles;i++) {
	  	// make a random point inside a unit-radius disk:
	  	double x,y,z;
	  	do {
	  		x=get_rand_double();
	  		y=get_rand_double();
	  	} while (x*x+y*y>1.0);
	  	z=get_rand_double();
	  	// scale to pancake size
	  	Particle pt;
	  	pt.P=CkVector3d(x*pancake.x,y*pancake.y,z*pancake.z);
	  	pt.V=-pt.P*0.99999e-6; // <- shoot all particles toward center (ensure collisions)
	  	pt.radius=0.5e3; 
	  	pt.mass=1.0e9;
	  	particles.push_back(pt);
	  }
  }

  Hello(CkMigrateMessage *m) : CBase_Hello(m) {}
  void pup(PUP::er &p) {
     p|collide;
     p|particles;
     if (p.isUnpacking())
	CollideRegister(collide,thisIndex);
  }
  ~Hello() {
     CollideUnregister(collide,thisIndex);
  }

  void DoIt(void)
  {
	CkPrintf("Element %04d taking timestep\n",thisIndex);
	// Move particles
	for (long i=0;i<particles.size();i++) {
		Particle &pt=particles[i];
		double dt=100000.0;
		pt.P+=pt.V*dt;
	} 
	
	// Copy out new bounding boxes
	long nBoxes=particles.size();
	boxes.resize(nBoxes);
	for (long i=0;i<nBoxes;i++) {
		CkVector3d r(particles[i].radius,particles[i].radius,particles[i].radius);
		boxes[i]=bbox3d(particles[i].P-r, particles[i].P, particles[i].P+r);
	} 
	
	CollideBoxesPrio(collide,thisIndex,nBoxes,&boxes[0],NULL);
  }
};

#include "collide_astro.def.h"

