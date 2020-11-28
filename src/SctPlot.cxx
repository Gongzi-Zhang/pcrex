#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TSctPlot.h"

using namespace std;

void usage();
set<int> parseSlugs(const char *);	// parse slugs

int main(int argc, char* argv[]) {
  const char * config_file("conf/sctplot.conf");
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
    cout << INFO << "use default config file: " << config_file << ENDL;

  TSctPlot fSctPlot(config_file);
  if (out_format)
    fSctPlot.SetOutFormat(out_format);
  if (out_name)
    fSctPlot.SetOutName(out_name);
  if (slugs.size() > 0)
    fSctPlot.SetSlugs(slugs);
  if (cycle)
    fSctPlot.SetIV("cycle");

  fSctPlot.CheckSlugs();
  fSctPlot.CheckVars();
  fSctPlot.GetValues();
  fSctPlot.CheckValues();
  fSctPlot.Draw();

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
    cerr << ERROR << "empty input for -s" << ENDL;
    return {};
  }
  set<int> vals;
  vector<char*> fields = Split(input, ',');
  for(int i=0; i<fields.size(); i++) {
    char * val = fields[i];
    if (Contain(val, "-")) {
      vector<char*> range = Split(val, '-');
      if (!IsInteger(range[0]) || !IsInteger(range[1])) {
        cerr << FATAL << "invalid range input" << ENDL;
        exit(3);
      }
      const int start = atoi(range[0]);
      const int end   = atoi(range[1]);
      if (start > end) {
        cerr << FATAL << "for range input: start must less than end" << ENDL;
        exit(4);
      }
      for (int j=start; j<=end; j++) {
        vals.insert(j);
      }
    } else {
      if (!IsInteger(val)) {
        cerr << FATAL << "run/slug must be an integer number" << ENDL;
        exit(4);
      }
      vals.insert(atoi(val));
    }
  }
  return vals;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
