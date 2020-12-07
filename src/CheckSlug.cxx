#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TCheckSlug.h"

using namespace std;

void usage();

int main(int argc, char* argv[]) {
  const char * config_file("conf/checkslug.conf");
  const char * out_name = NULL;
  const char * out_format = NULL;
  set<int> slugs;
  // bool sign = false;
  bool dconf = true; // use default config file

  char opt;
  while((opt = getopt(argc, argv, "hc:s:i:w:f:n:S")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      // case 'S':
      //   sign = true;  // make sign correction
      //   break;
      case 'c':
        config_file = optarg;
        dconf = false;
        break;
      case 's':
        slugs = ParseRS(optarg);
        break;
			case 'i':
				SetIHWP(optarg);
				break;
			case 'w':
				SetWienFlip(optarg);
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

	if (!slugs.size()) {
		cerr << FATAL << "no valid slugs specified" << ENDL;
		exit(4);
	}

  if (out_format)
    SetOutFormat(out_format);
  if (out_name)
    SetOutName(out_name);

  if (dconf) 
    cout << INFO << "use default config file: " << config_file << ENDL;
	TConfig fConf(config_file);
	fConf.ParseConfFile();

  TCheckSlug fCheckSlug;
  fCheckSlug.GetConfig(fConf);
	fCheckSlug.SetSlugs(slugs);
  // if (sign)
  //   fCheckSlug.SetSign();

  // fCheckSlug.CheckRuns();
  // fCheckSlug.CheckVars();
  // fCheckSlug.GetValues();
  // fCheckSlug.CheckValues();
  fCheckSlug.Draw();

  return 0;
}

void usage() {
  cout << "Check miniruns of specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -c: specify config file (default: check.conf)" << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
			 << "\t -i: set wanted ihwp state (in, out)" << endl
			 << "\t -w: set wien flip (left, right, horizontal, up, down, vertical)" << endl
       << "\t -f: set output file format: pdf or png" << endl
       << "\t -n: prefix of output pdf file" << endl
       << "\t -S: make sign correction" << endl
       << endl
       << "  Example:" << endl
       << "\t ./check -c myconf.conf -s 125,127-130 -n test" << endl;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
