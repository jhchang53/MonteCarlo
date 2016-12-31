//
//	MC.h
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "MC.h"

extern "C" {
#include <cairo/cairo-svg.h>
};

extern int dorun;

MC::MC()
{
  prlev = 0;
  plot = 0;
  PI = 2*acos(0.0);
};

MC::~MC()
{
};	// MC::~MC

void MC::setMaterial(double rann_i, double rsrc_i, int nmat_i,
	double Amass_i[], double sigtot_i[], double cratio_i[])
{
  rann = rann_i;
  rsrc = rsrc_i;
  nmat = nmat_i;
  printf("rann=%.4lf rsrc=%.4lf nmat=%d\n",rann,rsrc,nmat);
  Amass = new double[nmat];
  sigtot = new double[nmat];
  cratio = new double[nmat];
  for(int m=0; m < nmat; m++) {
    Amass[m] = Amass_i[m];
    sigtot[m] = sigtot_i[m];
    cratio[m] = cratio_i[m];
    printf(" mat=%d  Amass=%.1lf sigtot=%.4lf cratio=%.3lf\n",
	m+1, Amass[m],sigtot[m],cratio[m]);
  }
};	// MC::setMaterial

void MC::init(Geom *geom_i)
{
  geom = geom_i;
  nr = geom->get_nr();
  nz = geom->get_nz();
  heit = geom->getHeit();
  R = geom->getR();
  //	prepare cairo
  if(plot) {
    int hwidth = 1000;
    double rmax = 120.0;	// jhchang
    int zh = 2000;
    surfacexy = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
        2*hwidth,2*hwidth);
    crxy = cairo_create(surfacexy);
    scale = hwidth/rmax;
    xbias = hwidth;
    ybias = hwidth;
    zbias = zh-100;
    surfaceyz = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
        2*hwidth,zh);
    cryz = cairo_create(surfaceyz);
    surfacexz = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
        2*hwidth,zh);
    crxz = cairo_create(surfacexz);
  }
  //
  //	skip some random number
  for(int i=0; i < 300; i++) ranf();
  //
  geom->setPrlev(prlev);
};	// MC::init

void MC::run(int nhist, double gap_in, double plug_in, double dzbin_in)
{
  gap = gap_in;
  plug = plug_in;
  dzbin = dzbin_in;
  printf("gap=%.2lf plug=%.2lf dzbin=%.2lf\n",gap,plug,dzbin);
  tally_init();
  int nh;
  for(nh=0; dorun && (nh < nhist); nh++) {
    P = Born();
    if(plot) cairoborn(P);

    int inside = 1;
    for(int i=0; inside && (i < 100000); i++) {
      inside = Flight(nh);
    }
    if(!inside) {
      if(plot) cairodead(P);
      if(prlev) { printf("*** escaped :"); P.print(); }
      tally(P);
    }
    else {
      printf("*** not much :"); P.print();
      printf("  vert=%.3le  1-abs(w)=%.3le\n",sqrt(1.0-P.w*P.w),1-fabs(P.w));
      exit(0);
    }
  }
  printf("=== after %d histories\n",nh);
  if(plot) {
    char pngxy[256],pngyz[256],pngxz[256];
    sprintf(pngxy,"trackxy.png");
    cairo_surface_write_to_png(surfacexy,pngxy);
    printf("== %s written.\n",pngxy);
    sprintf(pngyz,"trackyz.png");
    cairo_surface_write_to_png(surfaceyz,pngyz);
    printf("== %s written.\n",pngyz);
    sprintf(pngxz,"trackxz.png");
    cairo_surface_write_to_png(surfacexz,pngxz);
    printf("== %s written.\n",pngxz);
  }
};	// MC::run

double MC::ranf()
{
  return rand() / (double) RAND_MAX;
};

Part MC::Born()
{
  //	born at central cylinder
  double Ri2 = rann*rann;
  double Ro = rsrc;
  double Ro2 = rsrc*rsrc;
  int nok = 1;
  double x,y;
  while(nok) {
    x = Ro*(2*ranf()-1);
    y = Ro*(2*ranf()-1);
    double r2 = x*x + y*y;
    nok = 0;
    if(r2 > Ro2) nok = 1;
    if(r2 < Ri2) nok = 1;
  }
  double z0 = 0.0;
  double z1 = heit-plug-gap;
  double dz = z1-z0;
  double z = z1 - dz*ranf();
  if(z > (heit-plug-gap)/2) z += plug+gap;
  Part P;
  P.x = x; P.y = y;  P.z = z;
  //
  //	determine direction
  double w = 1.0 - 2*ranf();
#define STREAM
#ifdef STREAM
  double onew = 1.0-fabs(w);
  //    to avoid z-direction streaming
  // 	in z-periodic non-scattering problem
  while(onew < 1.0e-5) {
    w = 1.0 - 2*ranf();
    onew = 1.0-fabs(w);
  }
#endif
  double sinth = sqrt(1-w*w);
  double phi = 2*PI*ranf();
  P.u = sinth*cos(phi);
  P.v = sinth*sin(phi);
  P.w = w;
  //	determine cell
  nborn++;
  Path path = geom->getCell(P);
  P.ir = path.ir;
  P.iz = path.iz;
  if(prlev > 1) {
    printf(" Born: "); P.print();
  }
  return P;
};	// MC::Born

int MC::Flight(int history)
{
  if(prlev) {
   printf("start: "); P.print();
  }
  if(plot) cairomoveto(P);
  //	compute flight distance
  double tau = -log(1.0-ranf());
  for(int f=0; f < 1000; f++) {
    //	call geometry
    Path path = geom->howfar(P);
    if(path.s > 1000.0) {
      printf("*** Too far. s=%.2le ir=%d iz=%d hist=%d\n",
	path.s,path.ir,path.iz,history);
      P.print();
      geom->setPrlev(9);
      geom->howfar(P);
      exit(0);
    }
    double sigt;
    int m = geom->getMat(P) - 1;
    if(m < 0) sigt = 1.0e-10;
    else sigt = sigtot[m];
    double dtau = sigt*path.s;
    double tau0 = tau;
    tau -= dtau;
    double s;
    if(tau < 0.0) {
      s = tau0/sigt;	// region is not changed
    }
    else {
      s = path.s;
      P.ir = path.ir;
      P.iz = path.iz;
    }
    P.x = P.x + s*P.u;
    P.y = P.y + s*P.v;
    P.z = P.z + s*P.w;
    //	adjust to cyclic z-direction
    if(P.iz < 0) {
      P.iz += nz;
      P.z  += heit;
    }
    else if(P.iz >= nz) {
      P.iz -= nz;
      P.z  -= heit;
    }
    if(tau < 0) {
      return Collide();
    }
    if(P.ir >= nr) return 0;	// escaped
  }
  printf("** Too strait\n");
  exit(0);
};	// MC::Flight

int MC::Collide()
{
#ifdef CTALLY
  ctally(P);
#endif
  rtally(P);
  if(prlev > 0) {
    printf("== Collision at "); P.print();
  }
  //
  int m = geom->getMat(P) - 1;
  double crat = 1.0;
  double Am = 1.0e+6;
  if(m >= 0) {
    crat = cratio[m];
    Am = Amass[m];
  }
  if(crat < 1.0) {
    double flive = ranf();
    if(flive > crat) return 0;        // kill particle
  }
  //
  double u = P.u;  double v = P.v;  double w = P.w;
  double s0 = 1.0;
  double cx = s0*u/(Am+1);
  double cy = s0*v/(Am+1);
  double cz = s0*w/(Am+1);
  double Vnx = s0*u - cx;
  double Vny = s0*v - cy;
  double Vnz = s0*w - cz;
  double Vn = sqrt(Vnx*Vnx+Vny*Vny+Vnz*Vnz);
  //    new angle in CMS
  double costhe = 1.0 - 2*ranf();
  double sinthe = sqrt(1-costhe*costhe);
  double phi = 2*PI*ranf();
  double uc = sinthe*cos(phi);
  double vc = sinthe*sin(phi);
  double wc = costhe;
  //
  double Vcx = Vn*uc;
  double Vcy = Vn*vc;
  double Vcz = Vn*wc;
  //
  double vox = Vcx + cx;
  double voy = Vcy + cy;
  double voz = Vcz + cz;
  //
  double speed = sqrt(vox*vox + voy*voy + voz*voz);
  P.u = vox/speed;
  P.v = voy/speed;
  P.w = voz/speed;
  if(prlev > 1) {
    printf(" new direction:"), P.print();
  }
  return 1;
};	// MC::Collide

void MC::tally_init()
{
  nborn = 0;
  nzbin = (int) (heit/dzbin + 0.99);
  dzbin = heit/nzbin;	// adjust bin size
  nrbin = 200;
  printf(" heit=%.2lf dzbin=%.2lf nzbin=%d R=%.4lf nrbin=%d\n",
	heit,dzbin,nzbin, R,nrbin);
  zbin = new double[nzbin];
  czbin = new double[nzbin];
  for(int j=0; j < nzbin; j++) {
    zbin[j] = 0;
    czbin[j] = 0;
  }
  crbin = new double[nrbin];
  for(int j=0; j < nrbin; j++) crbin[j] = 0.0;
};	// MC::tally_init

void MC::tally(Part P)
{
  int jz = P.z/dzbin + 0.5;
  if(jz < 0) jz += nzbin;
  jz = jz%nzbin;
  zbin[jz] += (P.x*P.u+P.y*P.v)/sqrt(P.x*P.x+P.y*P.y);
};	// MC::tally

void MC::ctally(Part P)
{
  int jz = P.z/dzbin + 0.5;
  if(jz < 0) jz += nzbin;
  jz = jz%nzbin;
  czbin[jz] += 1.0;
};      // MC::ctally

void MC::rtally(Part P)
{
  double r = sqrt(P.x*P.x+P.y*P.y);
  int jz = nrbin*r/R;
  crbin[jz] += 1.0;
};	// MC::rtally

void MC::ztally_print()
{
  double sum = 0;
#ifdef CTALLY
  double csum = 0;
#endif
  for(int j=0; j < nzbin; j++){
    sum += zbin[j];
#ifdef CTALLY
    csum += czbin[j];
#endif
  }
  printf("  total tally=%.0lf dzbin=%.2lf ",sum,dzbin);
#ifdef CTALLY
  printf("  zbin[0]=%.2lf czbin[0]=%.2lf\n",zbin[0],czbin[0]);
#else
  printf("  zbin[0]=%.2lf\n",zbin[0]);
#endif
  double avg = sum/nzbin;
#ifdef CTALLY
  double cavg = csum/nzbin;
#endif
  if(prlev) {
    printf("ztally:\n");
    int jhalf = nzbin/2;
    for(int j=0; j < nzbin; j++) {
      double zloc = (j+0.5)*dzbin;
#ifdef CTALLY
      printf("%.2lf %.4lf %.4lf\n",zloc,zbin[j]/avg,czbin[j]/cavg);
#else
      printf("%.2lf %.4lf\n",zloc,zbin[j]/avg);
#endif
    }
    printf("\n");
  }
  double sum0 = 0;
  double sum1 = 0;
  double sum2 = 0;
  double fpeak = 1.0;
#ifdef CTALLY
  double csum0 = 0;
  double csum1 = 0;
  double csum2 = 0;
  double cfmin = 1.0;
#endif
  for(int j=0; j < nzbin; j++) {
    double f = zbin[j]/avg;
    sum0 += 1;	
    sum1 += f;
    sum2 += f*f;
    if(f>fpeak) fpeak = f;
#ifdef CTALLY
    double cf = czbin[j]/cavg;
    csum0 += 1;
    csum1 += cf;
    csum2 += cf*cf;
    if(cf < cfmin) cfmin =cf;
#endif
  }
  double favg = sum1/sum0;
  double fvar = (sum2 - 2*sum1*favg + favg*favg*sum0)/(nzbin-1);
  printf("  favg=%.5lf  fstd=%.5lf   fpeak=%.5lf\n",favg,sqrt(fvar),fpeak);
  delete [] zbin;
#ifdef CTALLY
  double cfavg = csum1/csum0;
  double cfvar = (csum2 - 2*csum1*cfavg + cfavg*cfavg*csum0)/(nzbin-1);
  printf(" cfavg=%.5lf cfstd=%.5lf   cfmin=%.5lf\n",cfavg,sqrt(cfvar),cfmin);
  delete [] czbin;
#endif
};	// MC::ztally

void MC::rtally_print()
{
  double PI = 2*acos(0.0);
  double csum = 0;
  double Asum = 0;
  double *area = new double[nrbin];
  for(int j=0; j < nrbin; j++){
    csum += crbin[j];
    double ro = (j+1)*R/nrbin;
    double ri = j*R/nrbin;
    area[j] = PI*(ro*ro-ri*ri);
    Asum += area[j];
  }
  
  printf("  total tally=%.0lf Asum=%.3le ",csum,Asum);
  double *crden = new double[nrbin];
  if(prlev) {
    printf("  crbin:\n");
    for(int j=0; j < nrbin; j++) {
      printf(" %.0lf",crbin[j]);
      if(j%10 == 9) printf("\n");
    }
    printf("\n");
  }
  for(int j=0; j < nrbin; j++) {
    // crden[j] = Asum*crbin[j]/(area[j]*csum);
    crden[j] = Asum*crbin[j]/(area[j]*nborn);
  }
  printf("  cden:\n");
#define DETAIL
#ifdef DETAIL
  for(int j=0; j < nrbin; j++) {
    printf("%.6lf %.4lf\n",j*R/nrbin,crden[j]);
  }
#else
  for(int j=0; j < nrbin; j++) {
    printf(" %.4lf",crden[j]);
    if(j%10 == 9) printf("\n");
  }
  printf("\n");
#endif
};	// MC::rtally_print

void MC::cairomoveto(Part Q)
{
  assert(plot);
  if(Q.ir == 0) cairo_set_source_rgb(crxy,1.0,0.0,0.0);
  else if(Q.ir == 1) cairo_set_source_rgb(crxy,0.0,1.0,0.0);
  else if(Q.ir == 2) cairo_set_source_rgb(crxy,0.0,0.0,1.0);
  else cairo_set_source_rgb(crxy,0.5,0.5,0.5);
  //	z color is used
  if(Q.iz == 0) cairo_set_source_rgb(cryz,1.0,0.0,0.0);
  else if(Q.iz == 1) cairo_set_source_rgb(cryz,0.0,1.0,0.0);
  else if(Q.iz == 2) cairo_set_source_rgb(cryz,0.0,0.0,1.0);
  else cairo_set_source_rgb(cryz,0.5,0.5,0.5);
  if(Q.iz == 0) cairo_set_source_rgb(crxz,1.0,0.0,0.0);
  else if(Q.iz == 1) cairo_set_source_rgb(crxz,0.0,1.0,0.0);
  else if(Q.iz == 2) cairo_set_source_rgb(crxz,0.0,0.0,1.0);
  else cairo_set_source_rgb(crxz,0.5,0.5,0.5);

  int px = Q.x*scale + xbias;
  int py = ybias - Q.y*scale;
  int pz = zbias - Q.z*scale;
  cairo_move_to(crxy,px,py);
  cairo_move_to(cryz,py,pz);
  cairo_move_to(crxz,px,pz);
};	// MC::cairomoveto

void MC::cairolineto(Part Q)
{
  assert(plot);
  int px = Q.x*scale + xbias;
  int py = ybias - Q.y*scale;
  int pz = zbias - Q.z*scale;
  cairo_line_to(crxy,px,py); cairo_stroke(crxy);
  cairo_line_to(cryz,py,pz); cairo_stroke(cryz);
  cairo_line_to(crxz,px,pz); cairo_stroke(crxz);
};      // Geom::cairolineto

void MC::cairoborn(Part Q)
{
  assert(plot);
  double colR = 0.2;
  double colG = 0.2;
  double colB = 0.2;
  if(Q.iz == 0) colR = 1.0;
  else if(Q.iz == 1) colG = 1.0;
  else if(Q.iz == 2) colB = 1.0;
  else if(Q.iz == 3) colR = 0.8;
  cairo_set_source_rgb(crxy,colR,colG,colB);
  cairo_set_source_rgb(cryz,colR,colG,colB);
  cairo_set_source_rgb(crxz,colR,colG,colB);

  int px = Q.x*scale + xbias;
  int py = ybias - Q.y*scale;
  int pz = zbias - Q.z*scale;
  
  cairo_arc(crxy,px,py, 4, 0, 2*PI);	cairo_stroke(crxy);
  cairo_arc(cryz,py,pz, 4, 0, 2*PI);    cairo_stroke(cryz);
  cairo_arc(crxz,px,pz, 4, 0, 2*PI);    cairo_stroke(crxz);
};	// MC::cairoborn

void MC::cairodead(Part Q)
{
  assert(plot);
  double colR = 0.1;
  double colG = 0.1;
  double colB = 0.1;
  cairo_set_source_rgb(crxy,colR,colG,colB);
  cairo_set_source_rgb(cryz,colR,colG,colB);
  cairo_set_source_rgb(crxz,colR,colG,colB);

  int px = Q.x*scale + xbias;
  int py = ybias - Q.y*scale;
  int pz = zbias - Q.z*scale;

  cairo_arc(crxy,px,py, 4, 0, 2*PI);    cairo_stroke(crxy);
  cairo_arc(cryz,py,pz, 4, 0, 2*PI);    cairo_stroke(cryz);
  cairo_arc(crxz,px,pz, 4, 0, 2*PI);    cairo_stroke(crxz);
};	// MC::cairodead
