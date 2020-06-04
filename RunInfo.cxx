#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>
#include <set>
#include <vector>
#include <map>

#include "line.h"
#include "rcdb.h"
#include "TRun.h"

using namespace std;

enum fType {d, s, f, g}; // function type: return type
vector<const char *> rcdb_options = {
  "slug",
  "exp",
  "type",
  "target",
  "run_flag",
  "ihwp",
  "wien_flip", 
  "arm_flag",
};
map<string, int> rcdb_width = {
  {"run",	      4},
  {"exp",	      5},
  {"slug",	    4},
  {"type",	    11},
  {"target",	  10},
  {"run_flag",	8},
  {"ihwp",	    4},
  {"wien_flip",	11},
  {"arm_flag",	8},
};
map<string, int (*) (const int)> fd = {
  {"slug",     &GetSlugNumber},
  {"arm_flag", &GetArmFlag},
};
map<string, char * (*) (const int)> fs = {
  {"exp",       &GetExperiment},
  {"type",      &GetRunType},
  {"target",    &GetTarget},
  {"run_flag",  &GetRunFlag},
  {"ihwp",      &GetIHWP},
  {"wien_flip", &GetWienFlip},
};
map<string, float * (*) (const int)> ff = {
};
map<string, double * (*) (const int)> fg = {
};
map<string, fType> ftype;


void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  set<int> runs;
  set<int> slugs;
  vector<char *> evt_vars;
  vector<char *> reg_vars;
  vector<char *> slow_vars;
  bool rcdb = false;

  const int nopt = rcdb_options.size();
  for (int i=0; i<nopt; i++) {
    if (rcdb_width.find(rcdb_options[i]) == rcdb_width.end()) {
      cerr << "Warning:\t No width specified for option " << rcdb_options[i]
           << ". Use default value: 8" << endl;
      rcdb_width[rcdb_options[i]] = 8;
    }
    if (fd.find(rcdb_options[i]) != fd.end())
      ftype[rcdb_options[i]] = d;
    else if (fs.find(rcdb_options[i]) != fs.end())
      ftype[rcdb_options[i]] = s;
    else {
      cerr << "FATAL:\t Unsupported option " << rcdb_options[i] << endl
           << "\tIf it is an evt branch, please add it in evt.option" << endl;
      exit(4);
    }
  }

  map<string, bool> do_it;
  static struct option long_options[16];  // FIXME;
  int o;
  for (o=0; o<nopt; o++) {
    long_options[o] = {rcdb_options[o], no_argument, 0, 0};
    do_it[rcdb_options[o]] = false;  // initialization
  }
  long_options[o++] = {"cutfiles",  no_argument, 0, 1};
  long_options[o++] = {"pedestals", no_argument, 0, 2};
  long_options[o++] = {"evt", required_argument, 0, 3};
  long_options[o++] = {"regression", required_argument, 0, 4};
  long_options[o++] = {"slow",  required_argument, 0, 5};
  long_options[o++] = {"charge",  no_argument, 0, 6};
  long_options[o++] = {"help",  no_argument, 0, 'h'};
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
        rcdb = true;
        do_it[rcdb_options[option_index]] = true;
        break;
      case 1:
        do_it["cutfiles"] = true;
        break;
      case 2:
        do_it["pedestals"] = true;
        break;
      case 3: // evt tree
        evt_vars = Split(optarg, ',');
        do_it["evt"] = true;
        break;
      case 4: // reg. tree
        reg_vars = Split(optarg, ',');
        do_it["reg"] = true;
        break;
      case 5: // slow tree
        slow_vars = Split(optarg, ',');
        do_it["slow"] = true;
        break;
      case 6: // charge
        do_it["charge"] = true;
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

  if (runs.size() == 0) {
    cerr << "FATAL:\t no run specified" << endl;
    usage();
    exit(4);
  }

  // check evt variables
  if (do_it["evt"]) {
    for (char * var : evt_vars) {
      StripSpaces(var);
    }

    ifstream fin("evt.branches");
    if (!fin.is_open()) {
      cerr << "ERROR:\t can't read the config file: evt.branches. Skip reading evt." << endl;
      do_it["evt"] = false;
      goto ENDEVT;
    }
    set<string> branches;
    string branch;
    while (getline(fin, branch)) {
      size_t end = branch.find_last_not_of(" \t");
      branches.insert( (end == string::npos) ? branch : branch.substr(0, end+1));
    }
    fin.close();

    for (vector<char *>::iterator it=evt_vars.begin(); it!=evt_vars.end(); it++) {
      if (find(branches.begin(), branches.end(), *it) == branches.end()) {
        if ((*it)[0] != '\0')
          cerr << "WARNING:\t Unknown branch: " << *it << ". Ignore it." << endl
               << "Please refer to reg.branches for correct variable name" << endl;

        it = evt_vars.erase(it);  
        if (it == evt_vars.end())
          break;
      }
    }

    if (evt_vars.size() == 0) {
      cerr << "ERROR:\t no valid evt variables specified" << endl;
      do_it["evt"] = false;
      goto ENDEVT;
    }

    cout << "INFO:\t " << evt_vars.size() << " valid evt variables specified: " << endl;
    for (char * var : evt_vars) {
      cout << "\t" << var << endl;
    }
  }
ENDEVT:

  // check slow variables
  if (do_it["slow"]) {
    for (char * var : slow_vars) {
      StripSpaces(var);
    }

    ifstream fin("slow.branches");
    if (!fin.is_open()) {
      cerr << "ERROR:\t can't read the config file: slow.branches. Skip reading slow" << endl;
      do_it["slow"] = false;
      goto ENDSLOW;
    }
    set<string> branches;
    string branch;
    while (getline(fin, branch)) {
      size_t end = branch.find_last_not_of(" \t");
      branches.insert( (end == string::npos) ? branch : branch.substr(0, end+1));
    }

    for (vector<char *>::iterator it=slow_vars.begin(); it!=slow_vars.end(); it++) {
      if (find(branches.begin(), branches.end(), *it) == branches.end()) {
        if ((*it)[0] != '\0')
          cout << "WARNING:\t Unknown branch: " << *it << ". Ignore it." << endl
               << "Please refer to reg.branches for correct variable name" << endl;
        it = slow_vars.erase(it);  
        if (it == slow_vars.end())
          break;
      }
    }

    if (slow_vars.size() == 0) {
      cerr << "ERROR:\t no valid slow variables specified" << endl;
      do_it["slow"] = false;
      goto ENDSLOW;
    }

    cout << "INFO:\t " << slow_vars.size() << " valid slow variables specified: " << endl;
    for (char * var : slow_vars) {
      cout << "\t" << var << endl;
    }
  }
ENDSLOW:

  // check reg variables
  if (do_it["reg"]) {
    for (char * var : reg_vars) {
      StripSpaces(var);
    }

    ifstream fin("reg.branches");
    if (!fin.is_open()) {
      cerr << "ERROR:\t can't read the config file: reg.branches. Skip reading reg" << endl;
      do_it["reg"] = false;
      goto ENDREG;
    }
    set<string> branches;
    string branch;
    while (getline(fin, branch)) {
      size_t end = branch.find_last_not_of(" \t");
      branches.insert( (end == string::npos) ? branch : branch.substr(0, end+1));
    }

    for (vector<char *>::iterator it=reg_vars.begin(); it!=reg_vars.end(); it++) {
      if (find(branches.begin(), branches.end(), *it) == branches.end()) {
        if ((*it)[0] != '\0')
          cout << "WARNING:\t Unknown branch: " << *it << ". Ignore it." << endl
               << "Please refer to reg.branches for correct variable name" << endl;
        it = reg_vars.erase(it);  
        if (it == reg_vars.end())
          break;
      }
    }

    if (reg_vars.size() == 0) {
      cerr << "ERROR:\t no valid reg variables specified" << endl;
      do_it["reg"] = false;
      goto ENDREG;
    }

    cout << "INFO:\t " << reg_vars.size() << " valid reg variables specified: " << endl;
    for (char * var : reg_vars) {
      cout << "\t" << var << endl;
    }
  }
ENDREG:

  if (rcdb || do_it["evt"] || do_it["reg"] || do_it["slow"]) {
    if (rcdb) StartConnection();

    int width = 0;
    map<string, pair<double, double>> values;

    cout << endl << endl;
    // head
    printf("%-4s | ", "run");
    width += (4+3);
    if (rcdb) {
      for (int i=0; i<nopt; i++) {
        if (do_it[rcdb_options[i]]) {
          printf("%-*s | ", rcdb_width[rcdb_options[i]], rcdb_options[i]);
          width += (rcdb_width[rcdb_options[i]] + 3);
        }
      }
    }

    for (char * var : evt_vars) {
      printf("%-25s | ", var);
      width += (10 + 5 + 10 + 3);
    }
    for (char * var : slow_vars) {
      printf("%-25s | ", var);
      width += (10 + 5 + 10 + 3);
    }
    for (char * var : reg_vars) {
      printf("%-25s | ", var);
      width += (10 + 5 + 10 + 3);
    }
    putchar('\n');

    for (int i=1; i<width; i++)
      putchar('-');
    putchar('\n');

    for (set<int>::iterator it=runs.cbegin(); it != runs.cend(); it++) {
      int run = *it;
      printf("%-4d | ", run);
      for (int i=0; i<nopt; i++) {
        if (do_it[rcdb_options[i]])
          if (ftype[rcdb_options[i]] == d)
            printf("%-*d | ", rcdb_width[rcdb_options[i]], fd[rcdb_options[i]](run));
          else if (ftype[rcdb_options[i]] == s)
            printf("%-*s | ", rcdb_width[rcdb_options[i]], fs[rcdb_options[i]](run));
      }

      // evt tree
      if (do_it["evt"]) {
        values = GetEvtValues(run, evt_vars);
        if (values.size() == 0) { // error in reading values
          for (char * var : evt_vars) {
            printf("%-10.5g +/- %-10.5g | ", -1, -1);
          }
        } else {
          for (char * var : evt_vars) {
            printf("%-10.5g +/- %-10.5g | ", values[var].first, values[var].second);
          }
        }
      }

      // slow tree
      if (do_it["slow"]) {
        values = GetSlowValues(run, slow_vars);
        if (values.size() == 0) {
          for (char * var : evt_vars) {
            printf("%-10.4g +/- %-10.4g | ", -1, -1);
          }
        } else {
          for (char * var : slow_vars) {
            printf("%-10.4g +/- %-10.4g | ", values[var].first, values[var].second);
          }
        }
      }

      if (do_it["reg"]) {
        values = GetRegValues(run, reg_vars);
        if (values.size() == 0) {
          for (char * var : reg_vars) {
            printf("%-10.4g +/- %-10.4g | ", -1, -1);
          }
        } else {
          for (char * var : reg_vars) {
            printf("%-10.4g +/- %-10.4g | ", values[var].first, values[var].second);
          }
        }
      }
      putchar('\n');
    }

    for (int i=1; i<width; i++)
      putchar('-');
    putchar('\n');

    if (rcdb) EndConnection();
  }

  if (do_it["cutfiles"]) {
    printf("--------------------cutfiles--------------------\n");
    for (set<int>::iterator it=runs.cbegin(); it != runs.cend(); it++) {
      int run = *it;
      printf("%d:\n", run);
      for (const char * f : GetCutFiles(run))
        printf("\t%s\n", f);

      putchar('\n');
    }
    printf("------------------------------------------------\n");
  }

  if (do_it["pedestals"]) {
    printf("--------------------pedestals--------------------\n");
    for (set<int>::iterator it=runs.cbegin(); it != runs.cend(); it++) {
      int run = *it;
      printf("%d:\n", run);
      for (const char * f : GetPedestals(run))
        printf("\t%s\n", f);

      putchar('\n');
    }
    printf("------------------------------------------------\n");
  }

  if (do_it["charge"]) {
    map<string, pair<double, double>> val;
    char bcm[] = "bcm_an_ds";
    vector<char *> vars; // FIXME: which bcm should I use?
    vars.push_back(bcm);
    for (set<int>::iterator it=runs.cbegin(); it != runs.cend(); it++) {
      int run = *it;
      printf("%-4d | ", run);

      double charge;
      val = GetEvtValues(run, vars, "");  // total charge
      charge = val["entries"].first*val[bcm].first/120*1e-6; // total charge
      printf("%-8.3g | ", charge);

      val = GetEvtValues(run, vars);
      charge = val["entries"].first*val[bcm].first/120*1e-6; // valid charge
      printf("%-8.3g | \n", charge);
    }
  }
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
       << "\t --cutfiles: get corresponding cutfiles" << endl
       << "\t --pedestals: get corresponding pedestals" << endl
       << "\t --charge: get charge" << endl
       << "\t --evt: followed by evt branches, separated by comma (case sensitive)" << endl
       << "\t --regression: followed by reg. branches, separated by comma (case sensitive)" << endl
       << "\t --slow: followed by slow variables, separated by comma (case sensitive)" << endl
       << endl
       << "  Example:" << endl
       << "\t ./runinfo -r 6345-6456 --slug" << endl
       << "\t ./runinfo -r 6543,6677-6680 -s 125,127-130 --target" << endl
       << "\t ./runinfo -r 6543-6566,6677-6680 -s 125,127-130 --evt bcm_an_us,bpm4aX" << endl
       << "\t ./runinfo -r 6543,6566,6677 --reg diff_bpm12X" << endl
       << "\t ./runinfo -r 6543-6566,6677 --slow HALLA_dpp" << endl;
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

/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
