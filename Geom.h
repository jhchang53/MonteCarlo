#ifndef _inc_Geom
#define _inc_Geom
//
//	Geom.h
//

#include "Part.h"
#include "Path.h"

class Geom {
public:
  Geom();
  ~Geom();
  void setPrlev(int prl);
  void init(double gap);
  void init(int nz, double zgrid[], int nr, double rgrid[], int matmap[]);
  int get_nr() { return nr; };
  int get_nz() { return nz; };
  double getHeit() { return heit; }
  double getR();
  double getRgrid(int jrsrc);
  int getMat(Part P);
  Path getCell(Part P);
  Path howfar(Part P);
private:
  double ranf();
  double Zfar(int jr, int out, Part P);
  double Rfar(int jr, int out, Part P);
  double Quadric(double A, double B, double C, int out);

  int prlev,nr,nz;
  double PI;
  double sinf;
  double rgrid[10],zgrid[10];
  double heit;

  int *mat;
};
#endif
