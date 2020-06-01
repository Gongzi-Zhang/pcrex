#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TMulPlot.h"

using namespace std;

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  const char * config_file("mul_plot.conf");
  const char * run_list = NULL;
  const char * out_name = NULL;
  const char * out_format = NULL;
  const char * ft = NULL;
  bool logy = false;
  bool dconf = true;
  set<int> runs;
  set<int> slugs;

  char opt;
  while((opt = getopt(argc, argv, "hlc:r:R:s:t:n:f:")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'c':
        config_file = optarg;
        dconf = false;
        break;
      case 'r':
        runs = parseRS(optarg);
        break;
      case 'R':
        run_list = optarg;
        break;
      case 's':
        slugs = parseRS(optarg);
        break;
      case 't':
        ft = optarg;
        break;
      case 'l':
        logy = true;
        break;
      case 'n':
        out_name = optarg;
        break;
      case 'f':
        out_format = optarg;
        break;
      default:
        usage();
        exit(1);
    }

  if (dconf)
    cout << __PRETTY_FUNCTION__ << "INFO:\t use default config file: " << config_file << endl;

  TMulPlot fMulPlot(config_file, run_list);
  if (out_name)
    fMulPlot.SetOutName(out_name);
  if (out_format)
    fMulPlot.SetOutFormat(out_format);
  if (ft)
    fMulPlot.SetFileType(ft);
  if (logy)
    fMulPlot.SetLogy(logy);
  if (runs.size() > 0)
    fMulPlot.SetRuns(runs);
  if (slugs.size() > 0)
    fMulPlot.SetSlugs(slugs);

  fMulPlot.Draw();

  return 0;
}

void usage() {
  cout << "Draw mulplots for specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: mul_plot.conf)" << endl
       << "\t -r: specify runs (seperate by comma, no space between them. ran range is supportted: 5678,6666-6670,6688)" << endl
       << "\t -R: specify run list file" << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
       << "\t -t: specify root file type: japan or postpan (default: postpan)" << endl
       << "\t -l: logy" << endl
       << "\t -n: prefix of name of output" << endl
       << "\t -f: output file format (pdf or png: default pdf)" << endl
       << endl
       << "  Example:" << endl
       << "\t ./mulplot -c myconf.conf -R slug123.lsit -t japan -n slug123" << endl
       << "\t ./mulplot -c myconf.conf -r 6543,6677-6680 -s 125,127-130 -R run.list -n test -f png" << endl;
}

set<int> parseRS(const char * input) {
  if (!input) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t empty input for -r or -s" << endl;
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
