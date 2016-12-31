//
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <csignal>
#include "IniParser.h"
#include "Geom.h"
#include "MC.h"

int dorun;

void signalHandler(int signum)
{
  printf("** Stopped.\n");
  dorun = 0;
};

int main(int argc, char **argv)
{
  char inifn[256];
  strcpy(inifn,"mc.inp");
  if(argc > 1) strcpy(inifn,argv[1]);
  IniParser *inp = new IniParser();
  if(inp->readFile(inifn) == 0) {
    printf("*** mc input data file %s is missing.\n",inifn);
    exit(0);
  }
  double gap = inp->getDouble("gap",1.0);
  double zgrid[10];
  int nzp = inp->getDoubleArray("zgrid",zgrid,10);
  int nz = nzp-1;
  double rgrid[10];
  int nr = inp->getDoubleArray("rgrid",rgrid,10);
  int matgap[10];
  inp->getIntArray("matgap",matgap,10);

  double dzbin = inp->getDouble("dzbin",0.1);
  double plug = inp->getDouble("plug",2.0);
  double rsrc[3];
  inp->getDoubleArray("rsrc",rsrc,3);	// source region
  double sigtot[10],Amass[10],cratio[10];
  for(int m=0; m < 10; m++) {
    Amass[m] = 12.0;
    sigtot[m] = 0.43;
    cratio[m] = 1.0;
  }
  int nmat = inp->getDoubleArray("sigtot",sigtot,10);
  inp->getDoubleArray("Amass",Amass,10);
  inp->getDoubleArray("cratio",cratio,10);
  int *matmap = new int[nr*nz];
  for(int ij=0; ij < nr*nz; ij++) matmap[ij] = 1;
  inp->getIntArray("matmap",matmap,nr*nz);

  int nhist = inp->getInt("nhist",1);
  delete inp;

  Geom *geom = new Geom();
  //
  // geom->init(gap);
  geom->init(nz,zgrid,nr,rgrid, matmap);
  //
  MC *mc = new MC();
  mc->init(geom);
  mc->setMaterial(rsrc[0], rsrc[1], nmat,Amass,sigtot,cratio);
  signal(SIGINT, signalHandler);
  mc->run(nhist, gap, plug, dzbin);
  mc->rtally_print();
  // delete mc;
  // delete geom;
};
