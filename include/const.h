// const and universal definitions
#ifndef CONST_H
#define CONST_H

#include <string>
#include <map>
#include <vector>

using namespace std;

// run infos
#define START_RUN   3000
// #define PREX_END_RUN   4980
// #define CREX_START_RUN   5000
#define END_RUN     8888
#define START_SLUG  0
#define END_SLUG    223
#define PREX_AT_START_RUN 4102
#define PREX_AT_END_RUN 4135
#define PREX_AT_START_SLUG 501
#define PREX_AT_END_SLUG 510
#define CREX_AT_START_SLUG 4000
#define CREX_AT_END_SLUG 4019

// units
const double ppb = 1e-9;
const double ppm = 1e-6;
const double mm = 1e-3;
const double um = 1e-6;
const double nm = 1e-9;
const double mA = 1e-3;
const double uA = 1e-6;
const double nA = 1e-9;
const double mV = 1e-3;
const double uV = 1e-6;
const double nV = 1e-9;

map<string, const double> UNITS = {
	{"",		1},
  {"ppb", ppb},
  {"ppm", ppm},
  {"mm",  mm},
  {"um",  um},
  {"nm",  nm},
  {"mA",  mA},
  {"uA",  uA},
  {"nA",  nA},
  {"mV",  mV},
  {"uV",  uV},
  {"nV",  nV},
  {"1/mm", 1/mm},
  {"1/um", 1/um},
  {"ppm/um", ppm/um},
  {"C",   1},
};

const char * GetInUnit (string branch, string leaf) {
  if (branch.find("asym") != string::npos && branch.find("diff_bpm") != string::npos) 
    return "1/mm";
  // else if (branch.find("asym") != string::npos) 
  //   return "";
  else if (branch.find("bpm") != string::npos) 
    return "mm";
  else if (branch.find("yield") != string::npos) {
    if (branch.find("bcm") != string::npos) 
      return "uA";
    // else if (branch.find("bpm") != string::npos) 
    //   return "mm";
  } 
  return "";
}

const char * GetOutUnit (string branch, string leaf) {
  if (branch.find("asym") != string::npos && branch.find("diff_bpm") != string::npos) { // slope
    return "ppm/um";
  } else if (branch.find("asym") != string::npos) {
    if (leaf.find("mean") != string::npos || leaf.find("err") != string::npos)
      return "ppb";
    else // if (var.find("rms") != string::npos)
      return "ppm";
  } else if (branch.find("diff_bpm") != string::npos) {
    if (leaf.find("mean") != string::npos || leaf.find("err") != string::npos)
      return "nm";
    else // if (var.find("rms") != string::npos)
      return "um";
  } else if (branch.find("yield") != string::npos) {
    if (branch.find("bcm") != string::npos) 
      return "uA";
    if (branch.find("bpm") != string::npos) 
      return "mm";
  } else if (branch.find("charge") != string::npos) {
    return "C";
  } 
  return "";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
