#include <iostream>
#include "TCheckStat.h"
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
  const char * config_file("default.conf");
  int run=0;
  bool check_current_run = false;
  for (int i=1; i<argc; i++) {
    if (strcmp(argv[i], "-c") == 0) {
      if (i+1==argc || argv[i+1][0] == '-') {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\tconfig file for option -c needed" << endl;
        exit(1);
      }
      config_file = argv[++i];
      continue;
    } else if (strcmp(argv[i], "-r") == 0) {
      if (i+1==argc || argv[i+1][0] == '-') {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\trun number for option -r needed" << endl;
        exit(1);
      }
      run = atoi(argv[++i]);
      check_current_run = true;
      if (run < 5924 || run > 9999) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\tinvalid run number: " << argv[i] << endl;
        exit(2);
      }
    } else {
      cerr << __PRETTY_FUNCTION__ << ":FATAL\tunrecognized option" << endl;
      cout << "Usage: ./check -c conf_file [-r run]";
      exit(1);
    }
  }

  TCheckStat fCheckStat(config_file, check_current_run, run);
  fCheckStat.SetRuns();
  fCheckStat.SetVars();
  fCheckStat.GetValues();
  fCheckStat.CheckValues();
  fCheckStat.Draw();

  return 0;
}
