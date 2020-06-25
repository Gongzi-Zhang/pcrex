#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TCheckStat.h"

using namespace std;

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  const char * config_file("conf/check.conf");
  const char * run_list = NULL;
  const char * out_name = NULL;
  const char * out_format = NULL;
  set<int> runs;
  set<int> slugs;
  int latest_run;
  bool check_latest_run = false;
  bool sign = false;
  bool dconf = true; // use default config file

  char opt;
  while((opt = getopt(argc, argv, "hc:r:l:R:s:f:n:S")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'S':
        sign = true;  // make sign correction
        break;
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
      case 'l':
        check_latest_run = true;
        if (!IsInteger(optarg)) {
          cerr << FATAL << "the argument to -l option must be an integer." << ENDL;
          exit(20);
        }
        latest_run = atoi(optarg);
        break;
      case 's':
        slugs = parseRS(optarg);
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

  TCheckStat fCheckStat(config_file);
  if (out_format)
    fCheckStat.SetOutFormat(out_format);
  if (out_name)
    fCheckStat.SetOutName(out_name);
  if (check_latest_run)
    fCheckStat.SetLatestRun(latest_run);
  if (runs.size() > 0)
    fCheckStat.SetRuns(runs);
  if (slugs.size() > 0)
    fCheckStat.SetSlugs(slugs);
  if (sign)
    fCheckStat.SetSign();

  fCheckStat.CheckRuns();
  fCheckStat.CheckVars();
  fCheckStat.GetValues();
  fCheckStat.CheckValues();
  fCheckStat.Draw();

  return 0;
}

void usage() {
  cout << "Check miniruns of specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: check.conf)" << endl
       << "\t -r: specify runs (seperate by comma, no space between them. ran range is supportted: 5678,6666-6670,6688)" << endl
       << "\t -R: specify run list file" << endl
       << "\t -l: the latest run mode, which will compare it to the before 10 production runs automatically." << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
       << "\t -f: set output file format: pdf or png" << endl
       << "\t -n: prefix of output pdf file" << endl
       << "\t -S: make sign correction" << endl
       << endl
       << "  Example:" << endl
       << "\t ./check -c myconf.conf -l 6666" << endl
       << "\t ./check -c myconf.conf -R slug123.lsit -p slug123" << endl
       << "\t ./check -c myconf.conf -r 6543,6677-6680 -s 125,127-130 -R run.list -n test" << endl;
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
