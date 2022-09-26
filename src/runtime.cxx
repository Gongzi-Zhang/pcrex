#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <set>
#include <vector>
#include <map>

#include "io.h"
#include "line.h"
#include "rcdb.h"
#include "TRun.h"

using namespace std;

enum fType {d, s, f, g}; // function type: return type

void usage();
set<int> parseRS(const char *);	// parse runs|slugs

int main(int argc, char* argv[]) {
  set<int> runs;
  set<int> slugs;

	int opt;
  while((opt = getopt(argc, argv, "hr:s:")) != -1)
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
      default:
        usage();
        exit(1);
    }

	StartConnection();
  for (int slug : slugs) {
    set<int> sruns = GetRunsFromSlug(slug);
    for (int run : sruns) {
      runs.insert(run);
    }
  }

  if (runs.size() == 0) {
    cerr << "FATAL:\t no run specified" << ENDL;
    usage();
		EndConnection();
    exit(4);
  }

	struct tm tm;
	time_t t_start, t_end;
	printf("run  |     start time      |      end time       | length (min)\n");
	for (int run : runs)
	{
		char * start = GetStartTime(run);
		strptime(start, "%Y-%m-%d %H:%M:%S", &tm);
		t_start = mktime(&tm);
		char * end = GetEndTime(run);
		strptime(end, "%Y-%m-%d %H:%M:%S", &tm);
		t_end = mktime(&tm);
		printf("%4d | %-19s | %-19s | %-8.4f\n", run, start, end, (t_end-t_start)/60.);
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
       << endl
       << "  Example:" << endl
       << "\t ./runinfo -r 6345-6456" << endl
       << "\t ./runinfo -r 6543,6677-6680 -s 125,127-130" << endl;
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
