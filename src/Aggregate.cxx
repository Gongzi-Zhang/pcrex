#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TAggregate.h"

using namespace std;

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  const char * config_file("conf/aggregate.conf");
  const char * run_list = NULL;
  const char * out_dir = NULL;
  bool dconf = true;
  set<int> runs;
  set<int> slugs;

  char opt;
  while((opt = getopt(argc, argv, "hc:r:R:s:d:")) != -1)
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
      case 'd':
        out_dir = optarg;
        break;
      default:
        usage();
        exit(1);
    }

  if (dconf)
    cout << INFO << "use default config file: " << config_file << ENDL;

  TAggregate fAgg(config_file, run_list);
  if (out_dir)
    fAgg.SetOutDir(out_dir);
  if (runs.size() > 0)
    fAgg.SetRuns(runs);
  if (slugs.size() > 0)
    fAgg.SetSlugs(slugs);

	fAgg.CheckRuns();
	fAgg.CheckVars();
  fAgg.Aggregate();

  return 0;
}

void usage() {
  cout << "Aggregate (minirun) for specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: conf/aggregate.conf)" << endl
       << "\t -r: specify runs (seperate by comma, no space between them. ran range is supported: 5678,6666-6670,6688)" << endl
       << "\t -R: specify run list file" << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
       << "\t -d: prefix of name of output dir" << endl
       << endl
       << "  Example:" << endl
       << "\t ./aggregate -c myconf.conf -R slug123.list -n slug123" << endl
       << "\t ./aggregate -c myconf.conf -r 6543,6677-6680 -s 125,127-130 -R run.list -d rootfiles" << endl;
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
