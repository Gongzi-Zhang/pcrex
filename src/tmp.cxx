#include <iostream>
#include <stdlib.h>
#include <set>

#include "line.h"
#include "TBase.h"

using namespace std;

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  const char * config_file("conf/check.conf");
  const char * run_list = NULL;
  set<int> runs;
  set<int> slugs;
  bool dconf = true; // use default config file

  char opt;
  while((opt = getopt(argc, argv, "hc:r:R:s:a:i:w:S")) != -1)
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
      // case 'l':
      //   break;
      case 's':
        slugs = parseRS(optarg);
        break;
			case 'a':
				if (strcmp(optarg, "both") == 0)
					SetArmFlag(botharms);
				else if (strcmp(optarg, "left") == 0)
					SetArmFlag(leftarm);
				else if (strcmp(optarg, "right") == 0)
					SetArmFlag(rightarm);
				else if (strcmp(optarg, "all") == 0)
					SetArmFlag(allarms);
				else {
					cerr << ERROR << "Unknown arm flag: " << optarg << ENDL;
					exit(4);
				}
				break;
			case 'i':
				cout << INFO << "set required ihwp to " << optarg << ENDL;
				if (strcmp(optarg, "in") == 0)
					SetIHWP(in_hwp);
				else if (strcmp(optarg, "out") == 0)
					SetIHWP(out_hwp);
				else if (strcmp(optarg, "both") == 0)
					SetIHWP(both_hwp);
				else {
					cerr << ERROR << "Unknown IHWP state: " << optarg << ENDL;
					exit(4);
				}
				break;
			case 'w':
				cout << INFO << "set required wien flip to " << optarg << ENDL;
				if (strcmp(optarg, "left") == 0)
					SetWienFlip(wienleft);
				else if (strcmp(optarg, "right") == 0)
					SetWienFlip(wienright);
				else if (strcmp(optarg, "horizontal") == 0)
					SetWienFlip(wienhorizontal);
				else if (strcmp(optarg, "up") == 0)
					SetWienFlip(wienup);
				else if (strcmp(optarg, "down") == 0)
					SetWienFlip(wiendown);
				else if (strcmp(optarg, "vertical") == 0)
					SetWienFlip(wienvertical);
				else {
					cerr << ERROR << "Unknown Wien state: " << optarg << ENDL;
					exit(4);
				}
				break;
      default:
        usage();
        exit(1);
    }

  if (dconf) 
    cout << INFO << "use default config file: " << config_file << ENDL;

	TBase fCheck(config_file, run_list);
  if (runs.size() > 0)
    fCheck.SetRuns(runs);
  if (slugs.size() > 0)
    fCheck.SetSlugs(slugs);

  fCheck.CheckRuns();
  fCheck.CheckVars();
  fCheck.GetValues();
	fCheck.PrintStatistics();

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
       << endl
       << "  Example:" << endl
       << "\t ./check -c myconf.conf -l 6666" << endl
       << "\t ./check -c myconf.conf -R slug123.lsit -p slug123" << endl
       << "\t ./check -c myconf.conf -r 6543,6677-6680 -s 125,127-130 -R run.list -n test" << endl;
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
