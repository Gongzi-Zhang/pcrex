#include <iostream>
#include <fstream>
#include <glob.h>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "TFile.h"
#include "TTree.h"
#include "TEntryList.h"
#include "TLeaf.h"

#include "line.h"
#include "rcdb.h"

typedef struct {double mean, err, rms;} DATASET;

void usage();
set<int> parseRS(const char * input);

bool dit_agg(int run) {
  int miniruns = 0;  // number of miniruns for each session

  // get miniruns for each session
  const char * pattern = Form("/chafs2/work1/apar/japanOutput/prexPrompt_pass2_%d.???.root", run);
  glob_t globbuf;
  glob(pattern, 0, NULL, &globbuf);
  if (globbuf.gl_pathc == 0) {
    cerr << "FATAL:\t no japan root file for run: " << run << ENDL;
    return false;
  }
  const int sessions = globbuf.gl_pathc;
  for (int s=0; s<sessions; s++) {
    const char * file_name = globbuf.gl_pathv[s];
    TFile fin(file_name, "read");
    if (!fin.IsOpen()) {
      cerr << "WARNING:\t can't open japan root file: " << file_name << ENDL;
      continue;
    }
    TTree * tin = (TTree *)fin.Get("mul");  // mul tree
    if (!tin) {
      cerr << "WARNING:\t can't read mul tree from root file: " << file_name << ENDL;
      continue;
    }
    const int n = tin->Draw(">>elist", "ErrorFlag == 0", "entrylist");
    TEntryList * elist = (TEntryList *) gDirectory->Get("elist");
    if (!elist) {
      cerr << "FATAL:\t find 0 valid pattern in mul tree in root file: " << file_name << ENDL;
      return false;
    }
    TLeaf * l = tin->GetLeaf("BurstCounter");
    if (!l) {
      cerr << "WARNING:\t can't receive BurstCounter leaf from mul tree in root file: " << file_name << ENDL;
      continue;
    }
    l->GetBranch()->GetEntry(elist->GetEntry(n-1));
    miniruns += (l->GetValue() + 1); // number of miniruns
    cout << "INFO:\t find " << (l->GetValue() + 1) << " miniruns for session " << s << " of run: " << run << ENDL;

    delete tin;
    fin.Close();
  }
  if (miniruns == 0) {
    cerr << "FATAL:\t find no miniruns for run: " << run << ENDL;
    return false;
  } else {
    cout << "INFO:\t find " << miniruns << " miniruns for run: " << run << ENDL;
  }

  const char * dir = "/chafs2/work1/apar/aggRootfiles/dithering_1X";
  pattern = Form("%s/minirun_aggregator_%d_*.root", dir, run);
  glob(pattern, 0, NULL, &globbuf);
  if (globbuf.gl_pathc != miniruns) {
    cerr << "FATAL:\t unmatched miniruns between BurstCounter in japan tree" << endl
      << "\tand the number of minirun agg. root files in dir: " << dir << endl
      << "\t" << miniruns << " miniruns from BurstCounter" << endl
      << "\t" << globbuf.gl_pathc << " minirun agg.  root files in dir: " << dir << endl;
    return false; // comment out this line if you don't want burst mismatch check
  }
  globfree(&globbuf);



  int minirun;
  map<string, DATASET> vbuf;
  TTree * tout = new TTree("mini", "dithering corrected mini statistics");

  const char * f_branches_name = "dit_agg.branches";
  set<string> branches;
  string branch;
  ifstream f_branches(f_branches_name);
  if (!f_branches.is_open()) {
    cerr << "FATAL:\t can't open dit branches file: " << f_branches_name;
    return false; 
  }
  while (getline(f_branches, branch)) {
    size_t end = branch.find_last_not_of(" \t");
    branches.insert( (end == string::npos) ? branch : branch.substr(0, end+1));
  }
  f_branches.close();

  tout->Branch("run", &run, "run/I");
  tout->Branch("minirun", &minirun, "minirun/I");
  vector<const char *> dvs = {
    "usl", "usr",
    "atl1", "atl2", "atr1", "atr2",
    "sam1", "sam2", "sam3", "sam4", "sam5", "sam6", "sam7", "sam8",
  };
  vector<const char *> ivs = { "4aX", "4aY", "4eX", "4eY", "11X12X" };
  // tout->Branch("coeff", &coeff, "coeff[][]/D");
  // tout->Branch("err_coeff", &err_coeff, "err_coeff[][]/D");
  for(string b : branches) {
    vbuf[b] = {0, 0, 0};
    tout->Branch(b.c_str(), &vbuf[b], "mean/D:err:rms");
  }
  for (const char * dv : dvs) {
    for (const char * iv : ivs) {
      const char * b = Form("dit_%s_%s_slope", dv, iv);
      vbuf[b] = {0, 0, 0};
      tout->Branch(b, &vbuf[b], "mean/D:err:rms");
    }
  }

  // read dit corrected minirun average result
  for(int m=0; m<miniruns; m++) {
    const char *file_name = Form("%s/minirun_aggregator_%d_%d.root", dir, run, m);
    TFile fin(file_name, "read");
    if (!fin.IsOpen()) {
      cerr << "WARNING:\t can't open minirun agg. root file: " << file_name << ENDL;
      continue;
    }
    TTree * tin = (TTree *)fin.Get("agg");  // agg tree
    if (!tin) {
      cerr << "WARNING:\t can't read agg tree from root file: " << file_name << ENDL;
      continue;
    }

    minirun = m;
    for (string b : branches) {
      tin->SetBranchAddress(Form("%s_mean", b.c_str()),        &(vbuf[b].mean));
      tin->SetBranchAddress(Form("%s_mean_error",  b.c_str()), &(vbuf[b].err));
      tin->SetBranchAddress(Form("%s_rms",  b.c_str()),        &(vbuf[b].rms));
    }
    for (const char * dv : dvs) {
      for (const char * iv : ivs) {
        const char * b = Form("dit_%s_%s_slope", dv, iv);
        tin->SetBranchAddress(b,                    &vbuf[b]);
        tin->SetBranchAddress(Form("%s_error", b),  &vbuf[b].err);
      }
    }
    tin->GetEntry(0);
    tout->Fill();
    delete tin;
    fin.Close();
  }

  TFile fout(Form("dithering/prexPromt_dit_1X_agg_%d.root", run), "recreate");
  tout->Write();
  delete tout;
  fout.Close();

  return true;
}

int main(int argc, char ** argv) {
  set<int> runs;
  set<int> slugs;

  char opt;
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
    for (int run : GetRunsFromSlug(slug)) {
      runs.insert(run);
    }
  }

  GetValidRuns(runs);
  EndConnection();
  if (runs.size() == 0) {
    cerr << "FATAL:\t no valid runs specified." << ENDL;
    usage();
    exit(4);
  }
  cout << "INFO:\t you specify " << runs.size() << " runs:" << ENDL;
  for (int run : runs)
    cout << "\t" << run << endl;

  for (int run : runs)
    dit_agg(run);
}

void usage() {
  cout << "dithering aggragotor, collect data from each minirun" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -r: specify runs (seperate by comma, no space between them. ran range is supportted: 5678,6666-6670,6688)" << endl
       << "\t -s: specify slugs (the same syntax as -r)" << endl
       << endl
       << "  Example:" << endl
       << "\t ./dit_agg -s 4001,4003-4004" << endl
       << "\t ./dit_agg -r 6543,6677-6680" << endl;
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
