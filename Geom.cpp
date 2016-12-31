//
//	Geom.cpp
//
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "Geom.h"

extern int dorun;

Geom::Geom()
{
  prlev = 0;
  PI = 2*acos(0.0);
  sinf = 1.0e+9;
  dorun = 1;
  mat = NULL;
};

Geom:: ~Geom()
{
  delete [] mat;
};	// Geom:: ~Geom

void Geom::setPrlev(int prl)
{
  prlev = prl;
};

void Geom::init(double gap)
{
  //	simple gap geometry
  nz = 3;
  zgrid[0] = 0.0; zgrid[1] = 35.0; zgrid[2] = gap+35.0;  zgrid[3] = gap+70.0;
  heit = zgrid[nz];
  printf("nz=%d:",nz);
  for(int j=0; j <= nz; j++) printf(" %.1lf",zgrid[j]);
  printf("\n");
  nr = 2;
  rgrid[0] = 95.0;	rgrid[1] = 95.0 + 44.0;
  printf("nr=%d:",nr);
  for(int j=0; j < nr; j++) printf(" %.1lf",rgrid[j]);
  printf("\n");
  //	assign material
  mat = new int[nr*nz];
  for(int ij=0; ij < nr*nz; ij++) mat[ij] = 1;
  mat[4] = 0;
  printf("Material map:\n");
  for(int j=0; j < nz; j++) {
    for(int i=0; i < nr; i++) printf(" %2d",mat[i*nz+j]);
    printf("\n");
  }
};	// Geom::init (simple gap)

void Geom::init(int nz_i, double zgrid_i[], int nr_i, double rgrid_i[],
        int matmap[])
{
  //    geometry with key
  nz = nz_i;
  for(int i=0; i <= nz; i++) zgrid[i] = zgrid_i[i];
  heit = zgrid[nz];
  printf("nz=%d:",nz);
  for(int j=0; j <= nz; j++) printf(" %.1lf",zgrid[j]);
  printf("\n");
  nr = nr_i;
  for(int i=0; i < nr; i++) rgrid[i] = rgrid_i[i];
  printf("nr=%d:",nr);
  for(int j=0; j < nr; j++) printf(" %.1lf",rgrid[j]);
  printf("\n");
  //    assign material
  mat = new int[nr*nz];
  for(int ij=0; ij < nr*nz; ij++) mat[ij] = matmap[ij];
  printf("Material map:\n");
    for(int j=0; j < nz; j++) {
      for(int i=0; i < nr; i++) printf(" %2d",mat[i*nz+j]);
      printf("\n");
  }
};      // Geom::init (key)

double Geom::getR()
{
  return rgrid[nr-1];
};	// Geom::getR

double Geom::getRgrid(int jrsrc)
{
  return rgrid[jrsrc];
};	// Geom::getRgrid

int Geom::getMat(Part P)
{
  int ij = P.ir*nz + P.iz;
  return mat[ij];
};	// Geom::getMat

double Geom::ranf()
{
  return rand() / (double) RAND_MAX;
};	// Geom::ranf

Path Geom::getCell(Part P)
{
  //	determine cell
  double z = P.z;
  int jz = 0;
  for(int j=0; j < nz; j++) {
    if(z > zgrid[j]) jz = j;
  }
  //	radial position
  double r = sqrt(P.x*P.x + P.y*P.y);
  int jr = 0;
  for(int j=0; j < nr; j++) {
    if(r > rgrid[j]) jr = j+1;
  }
  if((jz < 0) || (jr < 0)) {
    printf("*** %s:%d Cannot determine cell number\n",__FILE__,__LINE__);
    printf("  ir=%d iz=%d for (%.3lf,%.3lf,%.3lf)\n",
	jr,jz,P.x,P.y,P.z);
    exit(0);
  }
  Path path;
  path.ir = jr;	path.iz = jz;
  return path;
};	// Geom::getCell

Path Geom::howfar(Part P)
{
  if(prlev > 2) {
    printf("\n== Geom::howfar entering\n");
    P.print();
  }
  double sr,sr1,sr2;
  int jr = P.ir;
  int jz = P.iz;
  int jr1,jr2;
  if(P.ir == 0) {
    sr = Rfar(P.ir,0, P);
    jr = 1;
  }
  else {
    sr1 = Rfar(P.ir,0, P);
    jr1 = P.ir+1;
    sr2 = Rfar(P.ir-1,1, P);
    jr2 = P.ir-1;
    if(sr1 < sr2) {
      sr = sr1;
      jr = jr1;
    }
    else {
      sr = sr2;
      jr = jr2;
    }
  }
  //	z- direction
  double sz = sinf;
  if(P.w > 0) {
    sz = Zfar(P.iz+1,1, P);
    jz = P.iz+1;
  }
  else if(P.w < 0) {
    sz = Zfar(P.iz,0, P);
    jz = P.iz-1;
  }
  //	compare with r
  double smin;
  if(sz < sr) {
    smin = sz;
    jr = P.ir;
  }
  else {
    smin = sr;
    jz = P.iz;
  }
  Path path;
  path.s = smin;
  path.ir = jr;  path.iz = jz;
  assert((P.iz != jz) || (P.ir != jr));
  if(prlev > 2) {
    printf("== Geom:howfar\n");
    printf("    path.s=%.2lf jr=%d jz=%d\n",path.s,jr,jz);
    printf(" P:"); P.print();
  }
  return path;
};	// Geom::howfar

double Geom::Zfar(int jz, int out, Part P)
{
  if(prlev > 1) {
    printf("jz=%d zg=%.2lf out=%d ",jz,zgrid[jz],out);
    printf("Particle:"); P.print();
  }
  if(fabs(P.w) < 1.0e-10) return sinf;
  double zg = zgrid[jz];
  double s = sinf;
  if(out) {
    s = (zg-P.z)/P.w;
    if(s <= 0) s = sinf;
    if(prlev > 2) printf("s=%.2lf\n",s);
  }
  else {
    s = (zg-P.z)/P.w;
    if(s <= 0) s = sinf;
    if(prlev > 2) printf("s=%.2lf\n",s);
  }
  if(prlev > 2) {
    printf("== Geom::Zfar zg=%.1lf out=%d s=%.2lf\n",zg,out,s);
    P.print();
  }
  return s;
};      // Geom::Zfar

double Geom::Rfar(int jr, int out, Part P)
{
  //	ref)  Alex F. Bielajaw,
  //	HOWFAR and HOWNEAR Geometry Modling for
  //	Monte Carlo Particle Transport
  //	Aug.14,1995, PIRS-0341. NRCS, Canada
  double R = rgrid[jr];
  double A = P.u*P.u + P.v*P.v;
  if(A < 1.0e-20) return sinf;
  double B = P.x*P.u + P.y*P.v;
  double C = P.x*P.x + P.y*P.y - R*R;
  double s = Quadric(A,B,C,out);
  if(prlev > 2) {
    printf("== Geom::Rfar R=%.2lf out=%d s=%.2lf\n",R,out,s);
    printf("   A=%.1le B=%.1le C=%.1le s=%.2le\n", A,B,C,s);
  }
  return s;
};	// Geom::Rfar

double Geom::Quadric(double A, double B, double C, int out)
{
  double det = B*B - A*C;
  double s;
  if(det < 0) {
    return sinf;
  }
  if(out) {	// particle is outside
    if(B >= 0) {
      if(A > 0) return sinf;
      s = -(B+sqrt(det))/A;
    }
    else {
      s = C/(sqrt(det)-B);
      if(s < 0) s = 0;
    }
  }
  else {	// particle is inside
    if(B <= 0) {
      if(A > 0) s = (sqrt(det)-B)/A;
      else return sinf;
    }
    else {
      s = -C/(sqrt(det)+B);
      if(s < 0) s = 0.0;
    }
  }
  return s;
};	// Geom::Quadric
