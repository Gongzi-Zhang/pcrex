#include <iostream>
#include <stdlib.h>
#include <getopt.h>
#include <set>
#include <vector>
#include <map>

#include "line.h"
#include "TRun.h"

using namespace std;

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  const char * run_list = NULL;
  set<int> runs;
  set<int> slugs;

  vector<const char *> options = {
    "slug",
    "exp",
    "type",
    "target",
    "run_flag",
    "ihwp",
    "wien_flip", 
    "arm_flag",
  };
  map<string, int> width;
  {
    width["run"] = 4;
    width["exp"] = 5;
    width["slug"] = 4;
    width["type"] = 11;
    width["target"] = 9;
    width["run_flag"] = 8;
    width["ihwp"] = 4;
    width["wien_flip"] = 11; 
    width["arm_flag"] = 8;
  }

  const int nopt = options.size();
  map<string, bool> do_it;
  static struct option long_options[16];  // FIXME;
  int o;
  for (o=0; o<nopt; o++) {
    long_options[o] = {options[o], no_argument, 0, 0};
    do_it[options[o]] = false;  // initialization
  }
  long_options[o++] = {"help", no_argument, 0, 'h'};
  long_options[o++] = {0, 0, 0, 0}; // end of long option

  int option_index = 0;
  char opt;
  while((opt = getopt_long(argc, argv, "hr:s:", long_options, &option_index)) != -1)
    switch (opt) {
      case 'h':
        usage();
        exit(0);
      case 'r':
        runs = parseRS(optarg);
        break;
      case 's':
        slugs = parseRS(optarg);
        break;
      case 0:
        do_it[options[option_index]] = true;
        break;
      default:
        usage();
        exit(1);
    }

  for (set<int>::iterator it=slugs.cbegin(); it != slugs.cend(); it++) {
    set<int> sruns = GetRunsFromSlug(*it);
    for (set<int>::iterator it_r = sruns.cbegin(); it_r != sruns.cend(); it_r++) {
      runs.insert(*it_r);
    }
  }

  StartConnection();

  printf("%-4s | ", "run");
  for (int i=0; i<nopt; i++) {
    if (do_it[options[i]])
      printf("%-*s | ", width[options[i]], options[i]);
  }
  printf("\n");

  const char * padding = "---------------";
  printf("%.4s---", padding);
  for (int i=0; i<nopt; i++) {
    if (do_it[options[i]])
      printf("%.*s---", width[options[i]], padding);
  }
  printf("\n");
  for (set<int>::iterator it=runs.cbegin(); it != runs.cend(); it++) {
    int run = *it;
    printf("%-4d | ", run);

    if (do_it["slug"])
      printf("%-*d | ", width["slug"], GetSlugNumber(run));

    if (do_it["exp"])
      printf("%-*s | ", width["exp"], GetExperiment(run));
      
    if (do_it["type"])
      printf("%-*s | ", width["type"], GetRunType(run));

    if (do_it["target"])
      printf("%-*s | ", width["target"], GetTarget(run));

    if (do_it["run_flag"])
      printf("%-*s | ", width["run_flag"], GetRunFlag(run));

    if (do_it["ihwp"])
      printf("%-*s | ", width["ihwp"], GetIHWP(run));

    if (do_it["wien_flip"])
      printf("%-*s | ", width["wien_flip"], GetWienFlip(run));

    if (do_it["arm_flag"])
      printf("%-*d | ", width["arm_flag"], GetArmFlag(run));

    printf("\n");
  }
  EndConnection();

  return 0;
}

void usage() {
  cout << "get run info of specified runs/slugs" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -r: specify runs (seperate by comma, no space between them. run range is supportted: 5678,6666-6670,6688)" << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
       << "\t --slug: get slug number" << endl
       << "\t --exp: get experiment (PREXII or CREX)" << endl
       << "\t --type: get run type" << endl
       << "\t --target: get target" << endl
       << "\t --run_flag: get run flag" << endl
       << "\t --ihwp: get ihwp state" << endl
       << "\t --wien_flip: get wien flip state" << endl
       << "\t --arm_flag: get arm flag" << endl
       << endl
       << "  Example:" << endl
       << "\t ./runinfo -r 6345-6456 --slug" << endl
       << "\t ./runinfo -r 6543,6677-6680 -s 125,127-130 --target" << endl;
}

set<int> parseRS(const char * input) {
  if (!input) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t empty input for -r or -s" << endl;
    return {};
  }
  set<int> vals;
  vector<char*> fields = Split(input, ',');
  for(int i=0; i<fields.size(); i++) {
    char * val = fields[i];
    if (Contain(val, "-")) {
      vector<char*> range = Split(val, '-');
      if (!IsInteger(range[0]) || !IsInteger(range[1])) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t invalid range input" << endl;
        exit(3);
      }
      const int start = atoi(range[0]);
      const int end   = atoi(range[1]);
      if (start > end) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t for range input: start must less than end" << endl;
        exit(4);
      }
      for (int j=start; j<=end; j++) {
        vals.insert(j);
      }
    } else {
      if (!IsInteger(val)) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t run/slug must be an integer number" << endl;
        exit(4);
      }
      vals.insert(atoi(val));
    }
  }
  return vals;
}

