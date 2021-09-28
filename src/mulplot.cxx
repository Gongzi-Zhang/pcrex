#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TMulPlot.h"

using namespace std;

void usage();
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
  while((opt = getopt(argc, argv, "hlc:r:R:s:n:f:")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'c':
        config_file = optarg;
        dconf = false;
        break;
      case 'r':
        runs = ParseRS(optarg);
        break;
      case 'R':
        run_list = optarg;
        break;
      case 's':
        slugs = ParseRS(optarg);
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

	if (!(runs.size() + slugs.size())) {
		cerr << FATAL << "no runs or slugs specified" << ENDL;
		exit(4);
	}

	for (int rs : ParseRSfile(run_list))
	{
		if (rs < 500)
			slugs.insert(rs);
		else
			runs.insert(rs);
	}

  if (dconf)
    cout << INFO << "use default config file: " << config_file << ENDL;
	TConfig fConf(config_file);
	fConf.ParseConfFile();
	SetupRCDB();

  TMulPlot fMulPlot;
	fMulPlot.GetConfig(fConf);
	logy = logy || (fConf.GetScalar("logy") ? true : false);
  if (logy)
    fMulPlot.SetLogy(logy);
  if (runs.size() > 0)
    fMulPlot.SetRuns(runs);
  if (slugs.size() > 0)
    fMulPlot.SetSlugs(slugs);

  if (out_name)
    SetOutName(out_name);
  if (out_format)
    SetOutFormat(out_format);


  fMulPlot.Draw();
	// fMulPlot.GetOutliers();

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
       << "\t -n: prefix of name of output" << endl
       << "\t -f: output file format (pdf or png: default pdf)" << endl
       << endl
       << "  Example:" << endl
       << "\t ./mulplot -c myconf.conf -R slug123.list -n slug123" << endl
       << "\t ./mulplot -c myconf.conf -r 6543,6677-6680 -s 125,127-130 -R run.list -l -n test -f png" << endl;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
