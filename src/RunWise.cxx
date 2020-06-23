#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TRunWise.h"

using namespace std;

void usage();
set<int> parseSlugs(const char *);	// parse slugs

int main(int argc, char* argv[]) {
  const char * config_file("conf/runwise.conf");
  const char * out_name = NULL;
  const char * out_format = NULL;
  set<int> slugs;
  bool cycle = false;
  bool dconf = true; // use default config file

  char opt;
  while((opt = getopt(argc, argv, "hCc:s:f:n:")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'C':
        cycle=true;
        break;
      case 'c':
        config_file = optarg;
        dconf = false;
        break;
      case 's':
        slugs = parseSlugs(optarg);
        break;
      case 'f':
        out_format = optarg;
        break;
      case 'n':
        out_name = optarg;
        break;
      default:
        usage();
        exit(1);
    }

  if (dconf) 
    cout << __PRETTY_FUNCTION__ << "INFO:\t use default config file: " << config_file << endl;

  TRunWise fRunWise(config_file);
  if (out_format)
    fRunWise.SetOutFormat(out_format);
  if (out_name)
    fRunWise.SetOutName(out_name);
  if (slugs.size() > 0)
    fRunWise.SetSlugs(slugs);
  if (cycle)
    fRunWise.SetIV("cycle");

  fRunWise.CheckSlugs();
  fRunWise.CheckVars();
  fRunWise.GetValues();
  fRunWise.CheckValues();
  fRunWise.Draw();

  return 0;
}

void usage() {
  cout << "Check runwise statistics of specified slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: check.conf)" << endl
       << "\t -s: specify slugs (seperate by comma, no space between them. ran range is supportted: 4001,4002-4005,4011)" << endl
       << "\t -f: set output file format: pdf or png" << endl
       << "\t -n: prefix of output pdf file" << endl
       << endl
       << "  Example:" << endl
       << "\t ./runwise -c myconf.conf -s 125,127-130 -n test" << endl;
}

set<int> parseSlugs(const char * input) {
  if (!input) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t empty input for -s" << endl;
    return {};
  }
  set<int> vals;
  vector<char*> fields = Split(input, ',');
  for(int i=0; i<fields.size(); i++) {
    char * val = fields[i];
    if (Contain(val, "-")) {
      vector<char*> range = Split(val, '-');
      if (!IsInteger(range[0]) || !IsInteger(range[1])) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t invalid range input" << endl;
        exit(3);
      }
      const int start = atoi(range[0]);
      const int end   = atoi(range[1]);
      if (start > end) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t for range input: start must less than end" << endl;
        exit(4);
      }
      for (int j=start; j<=end; j++) {
        vals.insert(j);
      }
    } else {
      if (!IsInteger(val)) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t run/slug must be an integer number" << endl;
        exit(4);
      }
      vals.insert(atoi(val));
    }
  }
  return vals;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
