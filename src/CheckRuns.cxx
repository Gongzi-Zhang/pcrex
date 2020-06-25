#include <iostream>
#include <stdlib.h>
#include <set>

#include "TCheckRuns.h"
#include "const.h"

using namespace std;

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  set<int> runs;
  const char * config_file("conf/checkruns.conf");
  bool cflag = true;
  const char * out_name = NULL;
  const char * out_format = NULL;

  char opt;
  while((opt = getopt(argc, argv, "hc:r:f:n:")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'c':
        config_file = optarg;
        cflag = false;
        break;
      case 'r':
        runs = parseRS(optarg);
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

  if (cflag) 
    cout << INFO << "no config file specified, use default one: " << config_file << ENDL;
  TCheckRuns fCheckRuns(config_file);
  if (runs.size() > 0)
    fCheckRuns.SetRuns(runs);
  if (out_format)
    fCheckRuns.SetOutFormat(out_format);
  if (out_name)
    fCheckRuns.SetOutName(out_name);

  fCheckRuns.CheckRuns();
  fCheckRuns.CheckVars();
  fCheckRuns.GetValues();
  fCheckRuns.CheckValues();
  fCheckRuns.Draw();

  return 0;
}

void usage() {
  cout << "Check miniruns of specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: check.conf)" << endl
       << "\t -r: specify run" << endl
       << "\t -f: set output file format: pdf or png" << endl
       << "\t -n: prefix of output pdf file" << endl
       << endl
       << "  Example:" << endl
       << "\t ./check -c myconf.conf -r 6666" << endl;
}

set<int> parseRS(const char * input) {
  if (!input) {
    cerr << ERROR << "empty input for -r or -s" << ENDL;
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
