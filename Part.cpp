//
//	Part.cpp
//
#include <stdio.h>
#include <cmath>
#include "Part.h"

void Part::print()
{
  double r = sqrt(x*x+y*y);
  printf("(%d,%d) R=%.2lf (%.2lf,%.2lf,%.2lf) dir (%.2lf,%.2lf,%.2lf)\n",
	ir,iz,r, x,y,z, u,v,w);
};	// Part2::print
