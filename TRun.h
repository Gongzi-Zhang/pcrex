#ifndef TRUN_H
#define TRUN_H

#include <iostream>
#include <glob.h>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TCut.h"

#include "line.h"

vector<const char *>  GetCutFiles  (const int run);
vector<const char *>  GetPedestals (const int run);
int     GetSessions (const int run);
void    Register    (const char * var);
void    Register    (vector<const char *> vs);
map<string, pair<double, double>> GetValues   (const int run);
map<string, pair<double, double>> GetValues   (const int run, vector<char *> vars);
map<string, pair<double, double>> GetSlowValues   (const int run, vector<char *> vars);
char *  FindCutFile(glob_t, const int run);

const char * cut_dir = "/adaqfs/home/apar/PREX/japan/Parity/prminput";
const char * japan_output_dir = "/chafs2/work1/apar/japanOutput";

set<string> vars;
map<string, double> var_buf;
map<string, pair<double, double>> values = { {"bcm_an_us", {0, 0}} };
int total_entries = 0;
int valid_entries = 0;
double total_charge = 0;
double valid_charge = 0;
TCanvas * c = new TCanvas("c", "c", 800, 600);

TCut cut = "ErrorFlag == 0";

vector<const char *> GetCutFiles (const int run) {
  vector<const char *> cut_files;
  glob_t globbuf;
  const char * pattern;
  vector<const char *> prefix = {
    "prexCH_beamline",  // beamline
    "prexCH_beamline_eventcuts",  // beamline eventcut
    "prexinj_beamline", // injector beamline
    "prexinj_helicity", // injector helicity
    "prex_maindet",     // main det.
    "prex_maindet_eventcuts", // main det eventcuts
    "prex_ring_stability",    // ring stability
    "prex_sam",         // prex sam
  };

  for (int i=0; i<prefix.size(); i++) {
    pattern = Form("%s/%s.*.map", cut_dir, prefix[i]);
    glob(pattern, 0, NULL, &globbuf);
    cut_files.push_back(FindCutFile(globbuf, run));
  }

  // bad event cut
  pattern = Form("%s/prex_bad_events.%d.map", cut_dir, run);
  glob(pattern, 0, NULL, &globbuf);
  if (globbuf.gl_pathc > 0)
    cut_files.push_back(basename(globbuf.gl_pathv[0]));

  globfree(&globbuf);
  return cut_files;
}

vector<const char *> GetPedestals (const int run) {
  vector<const char *> pedestals;
  glob_t globbuf;
  const char * pattern;
  vector<const char *> prefix = {
    "prexCH_beamline_pedestal",
    "prexinj_beamline_pedestal",
    "prex_maindet_pedestal",
    "prex_sam_pedestal",
  };

  for (int i=0; i<prefix.size(); i++) {
    pattern = Form("%s/%s.*.map", cut_dir, prefix[i]);
    glob(pattern, 0, NULL, &globbuf);
    pedestals.push_back(FindCutFile(globbuf, run));
  }

  globfree(&globbuf);
  return pedestals;
}

char * FindCutFile(glob_t globbuf, const int run) {
  for (int i=globbuf.gl_pathc-1; i>=0; i--) {
    const char * run_ranges = Split(basename(globbuf.gl_pathv[i]), '.')[1];
    int start_run;
    if (!Contain(run_ranges, "-")) {
      start_run = atoi(run_ranges);
      if (start_run != run)
        continue;
    } else {
      vector<char *> runs = Split(run_ranges, '-');
      start_run = atoi(runs[0]);
      if (start_run > run)
        continue;
      if (runs.size() == 2 && atoi(runs[1]) < run)
        continue;
    }
    return basename(globbuf.gl_pathv[i]);
  }
  return NULL;
}

int GetSessions (const int run) {
  glob_t globbuf;
  const char * pattern = Form("%s/prexPrompt_pass2_%d.???.root", japan_output_dir, run);
  glob(pattern, 0, NULL, &globbuf);
  return globbuf.gl_pathc;
}

void Register (const char * v) {
  vars.insert(v);
  var_buf[v] = 0;
}

void Register (vector<const char *> vs) {
  for (const char * v : vs) {
    vars.insert(v);
    var_buf[v] = 0;
  }
}
  
map<string, pair<double, double>> GetValues (const int run) {
  const int s = GetSessions(run);
  for (int i=0; i<s; i++) {
    double ErrorFlag;
    const char * root_file = Form("%s/prexPrompt_pass2_%d.%03d.root", japan_output_dir, run, i);
    TFile fin(root_file, "read");
    if (!fin.IsOpen()) {
      cerr << __PRETTY_FUNCTION__ << "FATAL:\t Can open root file: " << root_file;
      exit(4);
    }
    TTree * tin = (TTree*) fin.Get("evt");
    if (! tin) {
      cerr << __PRETTY_FUNCTION__ << "FATAL:\t Can receive evt tree from root file: " << root_file;
      exit(5);
    }
    for (set<string>::iterator it=vars.begin(); it != vars.end(); it++) {
      const char * var = (*it).c_str();
      // FIXME: check var validness
      tin->SetBranchAddress(var, &(var_buf[var]));
    }
    tin->SetBranchAddress("ErrorFlag", &ErrorFlag);

    int n = tin->GetEntries();
    for (int i=0; i<n; i++) {
      // if (i % 10000 == 0)
      //   cout << "Processing " << i << " entry" << endl;

      tin->GetEntry(i);
      total_charge += var_buf["bcm_an_us"];
      if (ErrorFlag == 0) { // cut
        valid_entries++;
        valid_charge += var_buf["bcm_an_us"];
        for (set<string>::iterator it=vars.begin(); it != vars.end(); it++) {
          string var = *it;
          values[var].first += var_buf[var];
          values[var].second += var_buf[var] * var_buf[var];
        }
      }
    }
    total_entries += n;
  }

  for (set<string>::iterator it=vars.begin(); it != vars.end(); it++) {
    string var = *it;
    double mean = values[var].first / valid_entries;
    double rms = sqrt((values[var].second / valid_entries) - mean * mean);
    values[var].first = mean;
    values[var].second = rms;
  }
  total_charge /= 120;  // helicity frequency
  total_charge /= 1e6;
  valid_charge /= 120;  
  valid_charge /= 1e6;

  values["entries"] = {total_entries, valid_entries};
  values["charge"]  = {total_charge,  valid_charge};
  return values;
}

map<string, pair<double, double>> GetValues (const int run, vector<char *> vars) {
  map<string, pair<double, double>> res;
  const int s = GetSessions(run);
  gROOT->SetBatch(1);
  for (int i=0; i<s; i++) {
    double ErrorFlag;
    const char * root_file = Form("%s/prexPrompt_pass2_%d.%03d.root", japan_output_dir, run, i);
    TFile fin(root_file, "read");
    if (!fin.IsOpen()) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can open root file: " << root_file;
      return res;
    }
    TTree * tin = (TTree*) fin.Get("evt");
    if (! tin) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can receive evt tree from root file: " << root_file;
      return res;
    }

    for (char * var : vars) {
      c->cd();
      const char * hname = Form("h_%s", var);
      tin->Draw(Form("%s>>%s", var, hname), "ErrorFlag == 0");
      TH1F *h = (TH1F*) gROOT->FindObject(hname);
      if (h) {
        res[var] = make_pair(h->GetMean(), h->GetRMS());
        h->Delete();
      } else {
        res[var] = make_pair(0, 0);
      }
    }
  }
  return res;
}

map<string, pair<double, double>> GetSlowValues (const int run, vector<char *> vars) {
  map<string, pair<double, double>> res;
  const int s = GetSessions(run);
  gROOT->SetBatch(1);
  for (int i=0; i<s; i++) {
    double ErrorFlag;
    const char * root_file = Form("%s/prexPrompt_pass2_%d.%03d.root", japan_output_dir, run, i);
    TFile fin(root_file, "read");
    if (!fin.IsOpen()) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can open root file: " << root_file;
      return res;
    }
    TTree * tin = (TTree*) fin.Get("slow");
    if (! tin) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can receive evt tree from root file: " << root_file;
      return res;
    }

    for (char * var : vars) {
      c->cd();
      const char * hname = Form("h_%s", var);
      tin->Draw(Form("%s>>%s", var, hname));
      TH1F *h = (TH1F*) gROOT->FindObject(hname);
      if (h) {
        res[var] = make_pair(h->GetMean(), h->GetRMS());
        h->Delete();
      } else {
        res[var] = make_pair(0, 0);
      }
    }
  }
  return res;
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
