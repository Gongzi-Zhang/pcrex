#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TCheckRS.h"

using namespace std;

void usage();

int main(int argc, char* argv[]) {
  const char * config_file("conf/checkrs.conf");
  const char * out_name = NULL;
  const char * out_format = NULL;
  set<int> slugs;
  set<int> runs;
  const char *type = "slug";
  bool sign = false;
  bool dconf = true; // use default config file

  char opt;
  while((opt = getopt(argc, argv, "hc:s:r:a:i:w:t:f:n:SR")) != -1)
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
      case 'r':
        runs = ParseRS(optarg);
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
      case 't':
        SetRunType(optarg);
        break;
      case 'f':
        out_format = optarg;
        break;
      case 'n':
        out_name = optarg;
        break;
      case 'S':
        sign = true;
        break;
      case 'R':
        type = "run";
        break;
      default:
        usage();
        exit(1);
    }

  if (dconf) 
    cout << INFO << "use default config file: " << config_file << ENDL;
	TConfig fConf(config_file);
	fConf.ParseConfFile();

  TCheckRS fCheckRS(type);
  fCheckRS.GetConfig(fConf);
  fCheckRS.SetSlugs(slugs);
  if (strcmp(type, "run") == 0) {
    fCheckRS.SetRuns(runs);
  }
  if (sign)
    fCheckRS.SetSign(sign);

  if (out_format)
    SetOutFormat(out_format);
  if (out_name)
    SetOutName(out_name);

  // fCheckRS.CheckRuns();
  // fCheckRS.CheckVars();
  // fCheckRS.GetValues();
  // fCheckRS.CheckValues();
  fCheckRS.Draw();

  return 0;
}

void usage() {
  cout << "Check miniruns of specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: check.conf)" << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
			 << "\t -a: set arm flag (both, left, right)" << endl
			 << "\t -i: set wanted ihwp state (IN, OUT)" << endl
			 << "\t -w: set wien flip (FLIP-LEFT, FLIP-RIGHT, Vertical(UP), Version(DOWN))" << endl
			 << "\t -t: set run type (Production, A-T...)" << endl
       << "\t -f: set output file format: pdf or png" << endl
       << "\t -n: prefix of output pdf file" << endl
       << "\t -S: make sign correction" << endl
       << endl
       << "  Example:" << endl
       << "\t ./checkrs -c myconf.conf -s 125,127-130 -n test" << endl;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
