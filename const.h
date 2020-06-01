// const and universal definitions
#ifndef CONST_H
#define CONST_H

#include <string>
#include <map>

#define START_RUN   5346
#define END_RUN     7430
#define START_SLUG  100
#define END_SLUG    185
// #define ROWS      69	// number of dv in slopes
// #define COLS      5	// number of iv in slopes

const double ppb = 1e-9;
const double ppm = 1e-6;
const double mm = 1e-3;
const double um = 1e-6;
const double nm = 1e-9;

std::map<std::string, const double> UNITS = {
  {"ppb", ppb},
  {"ppm", ppm},
  {"mm",  mm},
  {"um",  um},
  {"nm",  nm},
};
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
