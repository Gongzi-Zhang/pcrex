#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TCheckStat.h"

using namespace std;

void usage();

int main(int argc, char* argv[]) {
  const char * config_file("conf/check.conf");
  const char * run_list = NULL;
  set<int> runs;
  set<int> slugs;
  bool sign = false;
  bool dconf = true; // use default config file

  char opt;
  while((opt = getopt(argc, argv, "hc:r:R:s:a:i:w:f:n:S")) != -1)
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
        runs = ParseRS(optarg);
        break;
      case 'R':
        run_list = optarg;
        break;
      // case 'l':
      //   break;
      case 's':
        slugs = ParseRS(optarg);
        break;
			case 'a':
				SetArmFlag(optarg);
				break;
			case 'i':
				SetIHWP(optarg);
				break;
			case 'w':
				SetWienFlip(optarg);
				break;
      case 'f':
				if (optarg)
					SetOutFormat(optarg);
				else 
					cerr << WARNING << "no format specified for -f option" << ENDL;
        break;
      case 'n':
				if (optarg)
					out_name = optarg;
				else
					cerr << WARNING << "no name specified for -n option" << ENDL;
        break;
      default:
        usage();
        exit(1);
    }

	if (!(runs.size() + slugs.size())) {
		cerr << FATAL << "no runs or slugs specified" << ENDL;
		exit(4);
	}

  if (dconf) 
    cout << INFO << "use default config file: " << config_file << ENDL;
	TConfig fConf(config_file, run_list);
	fConf.ParseConfFile();

  TCheckStat fCheckStat;
	fCheckStat.GetConfig(fConf);
  if (runs.size() > 0)
    fCheckStat.SetRuns(runs);
  if (slugs.size() > 0)
    fCheckStat.SetSlugs(slugs);
  if (sign)
    fCheckStat.SetSign();

  // fCheckStat.CheckRuns();
  // fCheckStat.CheckVars();
  // fCheckStat.GetValues();
  // fCheckStat.CheckValues();
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
       << "\t -s: specify slugs (the same syntax as -r)" << endl
			 << "\t -a: indicate arm flag you want (both, left, right, singlearm, all: default both): of course, single arm specification will also include both arms running, but not vice versa." << endl
			 << "\t -i: set wanted ihwp state (in, out)" << endl
			 << "\t -w: set wien flip (left, right, horizontal, up, down, vertical)" << endl
       << "\t -f: set output file format: pdf or png" << endl
       << "\t -n: prefix of output pdf file" << endl
       << "\t -S: make sign correction" << endl
       << endl
       << "  Example:" << endl
       << "\t ./check -c myconf.conf -l 6666" << endl
       << "\t ./check -c myconf.conf -R slug123.lsit -p slug123" << endl
       << "\t ./check -c myconf.conf -r 6543,6677-6680 -s 125,127-130 -R run.list -n test" << endl;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
