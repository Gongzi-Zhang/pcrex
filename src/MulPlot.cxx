#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TMulPlot.h"

using namespace std;

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  const char * config_file("conf/mul_plot.conf");
  const char * run_list = NULL;
  const char * out_name = NULL;
  const char * out_format = NULL;
  bool logy = false;
  bool dconf = true;
  set<int> runs;
  set<int> slugs;

  char opt;
  while((opt = getopt(argc, argv, "hlc:r:R:s:a:n:f:")) != -1)
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
      case 'l':
        logy = true;
        break;
			case 'a':
				SetArmFlag(optarg);
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
    cout << INFO << "use default config file: " << config_file << ENDL;

  TMulPlot fMulPlot(config_file, run_list);
  if (out_name)
    fMulPlot.SetOutName(out_name);
  if (out_format)
    fMulPlot.SetOutFormat(out_format);
  if (logy)
    fMulPlot.SetLogy(logy);
  if (runs.size() > 0)
    fMulPlot.SetRuns(runs);
  if (slugs.size() > 0)
    fMulPlot.SetSlugs(slugs);

  fMulPlot.Draw();
	fMulPlot.GetOutliers();

  return 0;
}

void usage() {
  cout << "Draw mulplots for specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: mul_plot.conf)" << endl
       << "\t -r: specify runs (seperate by comma, no space between them. ran range is supported: 5678,6666-6670,6688)" << endl
       << "\t -R: specify run list file" << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
       << "\t -l: set log scale" << endl
			 << "\t -a: indicate arm flag you want (both, left, right: default both): of course, single arm specification will also include both arms running, but not vice versa." << endl
       << "\t -n: prefix of name of output" << endl
       << "\t -f: output file format (pdf or png: default pdf)" << endl
       << endl
       << "  Example:" << endl
       << "\t ./mulplot -c myconf.conf -R slug123.list -n slug123" << endl
       << "\t ./mulplot -c myconf.conf -r 6543,6677-6680 -s 125,127-130 -R run.list -l -n test -f png" << endl;
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
