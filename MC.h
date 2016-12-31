//
//	MC.h
//
#include "Geom.h"
extern "C" {
#include <cairo/cairo-svg.h>
};

class MC {
public:
  MC();
  ~MC();
  void init(Geom *geom);
  void setMaterial(double rann, double rsrc, int nmat,
	double Amass[], double sigtot[], double cratio[]);
  void run(int nhist, double gap, double plug, double dzbin);
  void ztally_print();
  void rtally_print();
private:
  double ranf();
  Part Born();
  int Flight(int history);
  int Collide();

  void tally_init();
  void tally(Part P);

  void ctally(Part P);
  void rtally(Part P);

  void cairoborn(Part Q);
  void cairomoveto(Part Q);
  void cairolineto(Part Q);
  void cairodead(Part Q);

  int prlev,plot;
  double PI;
  Geom *geom;
  int nr,nz;
  double heit,R;
  Part P;
  //	material property
  double rann,rsrc;
  int nmat;
  double *Amass,*sigtot,*cratio;
  //	source region
  double gap,plug;
  //	tally
  int nborn;
  double dzbin;
  int nzbin;
  double *zbin;
  double *czbin;
  int nrbin;
  double *crbin;
  //
  //	cairo variables
  cairo_surface_t *surfacexy,*surfaceyz,*surfacexz;
  cairo_t *crxy,*cryz,*crxz;
  int xbias,ybias,zbias;
  double scale;
};
