#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>
#include <set>
#include <vector>
#include <map>

#include "io.h"
#include "line.h"
#include "rcdb.h"
#include "TRun.h"

using namespace std;


void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  set<int> runs;
  set<int> slugs;

	static struct option long_options[10];
	int o;
  long_options[o++] = {"slug",			required_argument, 0, 1};
  long_options[o++] = {"arm_flag",	required_argument, 0, 2};
  long_options[o++] = {"wien_flip", required_argument, 0, 3};
  long_options[o++] = {"ihwp",			required_argument, 0, 4};
  long_options[o++] = {"run_flag",  required_argument, 0, 5};
  long_options[o++] = {"run_type",	required_argument, 0, 6};
	long_options[o++] = {"target",		required_argument, 0, 7};
  long_options[o++] = {"exp",				required_argument, 0, 8};
  long_options[o++] = {0, 0, 0, 0}; // end of long option

	int option_index = 0;
	char opt;
  while((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 1:
        slugs = parseRS(optarg);
        break;
			case 2:
				SetArmFlag(optarg);
				break;
			case 3:
				SetWienFlip(optarg);
				break;
			case 4:
				SetIHWP(optarg);
				break;
			case 5:
				SetRunFlag(optarg);
				break;
			case 6:
				SetRunType(optarg);
				break; 
			case 7:
				SetTarget(optarg);
				break;
			case 8:
				SetExp(optarg);
				break;
			default:
        usage();
        exit(1);
    }

	SetupRCDB();
	StartConnection();
	set<int> vruns = GetRuns();
	if (slugs.size()) {
		for (int s : slugs) {
			set<int> sruns = GetRunsFromSlug(s);
			for (int r : sruns) 
				if (vruns.find(r) != vruns.end())
					runs.insert(r);
		}
	} else 
		runs = vruns;
	EndConnection();
	cout << BINFO << "Get " << runs.size() << " runs" << ENDL;
	for (int r : runs)
		cout << r << endl;
	return (0);
}

void usage() {
  cout << "get runs from rcdb with specific conditions" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t --slug: specify slug (seperate by comma, no space between them. Range is supportted: 100-101,102,104-110)" << endl
       << "\t --arm_flag: specify arm flag (both (default), left, right, single, all)" << endl
       << "\t --wien_flip: specify wien flip state (FLIP-HORIZONTAL (default), FLIP-LEFT, FLIP-RIGHT, VERTICAL(UP), VERTICAL(DOWN))" << endl
       << "\t --ihwp: specify ihwp state (BOTH (default), IN, OUT)" << endl
       << "\t --run_flag: specify run flag (Good (default), Suspecious ...)" << endl
       << "\t --run_type: speicify run type (Production (default), A_T, Calibration, Test, Junk ...)" << endl
       << "\t --target: specify target (Ca48 (default), Home, D-208Pb5-D, Carbon 1% ... )" << endl
       << "\t --exp: specify experiment (PREX2 or CREX (default))" << endl
       << endl
       << "  Example:" << endl
       << "\t ./getrun --slug 111,121-131,141" << endl
       << "\t ./getrun --slug 112, --target 'Carbon 1%'" << endl
       << "\t ./getrun --arm_flag single" << endl
       << "\t ./getrun --wien_flip FLIP-LEFT, --ihwp IN" << endl
       << "\t ./getrun --run_flag Suspecious --type A_T" << endl;
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
