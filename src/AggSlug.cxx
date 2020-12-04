#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TAggSlug.h"

using namespace std;

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  const char *config_file("conf/aggslug.conf");
  const char *out_dir = NULL;
  bool dconf = true;
  set<int> slugs;

  char opt;
  while((opt = getopt(argc, argv, "hc:r:R:s:a:d:")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'c':
        config_file = optarg;
        dconf = false;
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

  if (out_dir)
    SetOutDir(out_dir);
	CheckOutDir();

	TConfig fConf(config_file);
	fConf.ParseConfFile();

  TAggSlug fAgg;
	fAgg.GetConfig(fConf);
  if (slugs.size() == 0) {
		cerr << FATAL << "no slug specified" << ENDL;
		usage();
		exit(4);
	}
	fAgg.SetSlug(*slugs.begin());
	fAgg.CheckRuns();
	fAgg.CheckVars();
	for (int slug : slugs) {
    fAgg.SetSlug(slug);
		fAgg.GetValues();
		fAgg.AggSlug();
	}

  return 0;
}

void usage() {
  cout << "AggSlug (minirun) for specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: conf/aggregate.conf)" << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
       << "\t -d: prefix of name of output dir" << endl
       << endl
       << "  Example:" << endl
       << "\t ./aggslug -c myconf.conf -s 125,127-130 -d rootfiles" << endl;
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
