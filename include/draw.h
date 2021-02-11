// auxiliary funcitons
#ifndef DRAW_H
#define DRAW_H

#include <vector>
#include <map>
#include <string>
#include "TCanvas.h"

enum Format {pdf, png};

Format format = pdf; // default pdf output
const char *out_name;
TCanvas *c;
map<int, const char *> legends = {
  {-1,  "left in"},
  {1,	"left out"},
  {2,	"right in"},
  {-2,  "right out"},
  {-3,  "up in"},
  {3,	"up out"},
  {4,	"down in"},
  {-4,  "down out"},
};
vector<int> flips;
vector<int> mcolors = {2, 4, 3, 6};
vector<int> mstyles = {20, 21, 22, 23};

void SetOutName(const char *name) {if (name) out_name = name; else cerr << WARNING << "null pointer passed." << ENDL;}
void SetOutFormat(const char *f);
const char * GetInUnit(string branch, string leaf="");
const char * GetOutUnit(string branch, string leaf="");

void SetOutFormat(const char *f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << FATAL << "Unknow output format: " << f << ENDL;
    exit(40);
  }
}

const char * GetInUnit (string branch, string leaf) {
  if (branch.find("asym") != string::npos && branch.find("diff") != string::npos) 
    return "1/mm";
  // else if (branch.find("asym") != string::npos) 
  //   return "";
  else if (branch.find("diff") != string::npos) 
    return "mm";
  else if (branch.find("yield") != string::npos) {
    if (branch.find("bcm") != string::npos) 
      return "uA";
    else if (branch.find("bpm") != string::npos) 
      return "mm";
  } 
  return "";
}

const char * GetOutUnit (string branch, string leaf) {
  if (branch.find("asym") != string::npos && branch.find("diff") != string::npos) { // slope
    return "ppm/um";
  } else if (branch.find("asym") != string::npos) {
    if (leaf.find("mean") != string::npos || leaf.find("err") != string::npos)
      return "ppb";
    else // if (var.find("rms") != string::npos)
      return "ppm";
  } else if (branch.find("diff") != string::npos) {
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
