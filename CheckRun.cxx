#include <iostream>
#include <stdlib.h>

#include "TCheckRun.h"
#include "const.h"

using namespace std;

void usage();

int main(int argc, char* argv[]) {
  int run;
  const char * config_file("checkrun.conf");
  const char * out_name = NULL;
  const char * out_format = NULL;

  char opt;
  while((opt = getopt(argc, argv, "hc:r:f:n:")) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'c':
        config_file = optarg;
        break;
      case 'r':
        run = atoi(optarg);
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

	if (run == 0 || run < START_RUN || run > END_RUN) {
		cerr << __PRETTY_FUNCTION__ << ":FATAL\t no run or invalid run specified." << endl;
		exit(4);
	}

  TCheckRun fCheckRun(config_file);
	fCheckRun.SetRun(run);
  if (out_format)
    fCheckRun.SetOutFormat(out_format);
  if (out_name)
    fCheckRun.SetOutName(out_name);

  fCheckRun.CheckRun();
  fCheckRun.CheckVars();
  fCheckRun.GetValues();
  fCheckRun.CheckValues();
  fCheckRun.Draw();

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
