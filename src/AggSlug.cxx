#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TAggSlug.h"

using namespace std;

void usage();

int main(int argc, char* argv[]) {
  const char *config_file("conf/aggslug.conf");
  const char *out_dir = NULL;
  bool dconf = true;
  set<int> slugs;

  char opt;
  while((opt = getopt(argc, argv, "hc:r:s:a:d:")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'c':
        config_file = optarg;
        dconf = false;
        break;
      case 's':
        slugs = ParseRS(optarg);
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

  TAggSlug fAgg = TAggSlug();
	fAgg.GetConfig(fConf);
  fAgg.SetSlugs(fConf.GetRS());
  fAgg.SetSlugs(slugs);
  fAgg.CheckRuns();
	fAgg.CheckVars();
  fAgg.AggSlugs();

  return 0;
}

void usage() {
  cout << "AggSlug (minirun) for specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: conf/aggslug.conf)" << endl
       << "\t -s: specify slugs (seperated by comma, no space between them. ranges are supported: 5678-6789,8765)" << endl
       << "\t -d: prefix of name of output dir" << endl
       << endl
       << "  Example:" << endl
       << "\t ./aggslug -c myconf.conf -s 125,127-130 -d rootfiles" << endl;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
