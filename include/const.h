// const and universal definitions
#ifndef CONST_H
#define CONST_H

#include <string>
#include <map>

#define START_RUN   3000
#define END_RUN     7430
#define START_SLUG  0
#define END_SLUG    185
#define PREX_AT_START_RUN 4102
#define PREX_AT_END_RUN 4135
#define PREX_AT_START_SLUG 501
#define PREX_AT_END_SLUG 510
#define CREX_AT_START_SLUG 4000
#define CREX_AT_END_SLUG 4019
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
