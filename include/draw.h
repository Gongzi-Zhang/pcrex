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
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
