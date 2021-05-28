#include <iostream>
#include <stdlib.h>
#include <set>

#include "TCheckEvent.h"
#include "const.h"

using namespace std;

void usage();
int main(int argc, char* argv[]) {
  set<int> runs;
  const char * config_file("conf/checkruns.conf");
  bool cflag = true;

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
        runs = ParseRS(optarg);
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

	if (!runs.size()) {
		cerr << FATAL << "no runs specified" << ENDL;
		exit(4);
	}

  if (cflag) 
    cout << INFO << "no config file specified, use default one: " << config_file << ENDL;
	TConfig fConf(config_file);
	fConf.ParseConfFile();
	SetupRCDB();

  TCheckEvent fCheckEvent;
	fCheckEvent.GetConfig(fConf);
	fCheckEvent.SetRuns(runs);
  fCheckEvent.CheckRuns();
  fCheckEvent.CheckVars();
  fCheckEvent.GetValues();
	fCheckEvent.ProcessValues();
  // fCheckEvent.CheckValues();
  fCheckEvent.Draw();

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
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
