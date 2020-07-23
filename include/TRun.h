#ifndef TRUN_H
#define TRUN_H

#include <iostream>
#include <unistd.h>
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

char hostname[32];
int  discarded = gethostname(hostname, 32);

vector<const char *>  GetCutFiles  (const int run);
vector<const char *>  GetPedestals (const int run);
int     GetJapanSessions  (const int run);
int     GetRegSessions    (const int run);
map<string, pair<double, double>> GetEvtValues    (const int run, vector<char *> vars, TCut cut);
map<string, pair<double, double>> GetRegValues    (const int run, vector<char *> vars, TCut cut);
map<string, pair<double, double>> GetSlowValues   (const int run, vector<char *> vars, TCut cut);
char *  FindCutFile(glob_t, const int run);

const char * cut_dir =
  (Contain(hostname, "aonl") || Contain(hostname, "adaq")) ?  
    "/adaqfs/home/apar/PREX/japan/Parity/prminput" : 
    ".";
const char * japan_dir = 
  (Contain(hostname, "aonl") || Contain(hostname, "adaq")) ?  
    "/chafs2/work1/apar/japanOutput" : 
    (Contain(hostname, "ifarm") ? 
      "/lustre/expphy/volatile/halla/parity/prex-respin2/japanOutput" :
      "."
    );
const char * reg_dir = 
  (Contain(hostname, "aonl") || Contain(hostname, "adaq")) ?  
    "/chafs2/work1/apar/postpan-outputs" :
    (Contain(hostname, "ifarm") ? 
      "/lustre/expphy/volatile/halla/parity/prex-respin2/postpan_respin" :
      "."
    );
      

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
  if (cut_files.size() == 0) 
    cut_files.push_back(Form("No cut files for run: %d founded in specified dir: %s/", run, cut_dir));

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
  if (pedestals.size() == 0) 
    pedestals.push_back(Form("No pedestal files for run: %d founded in specified dir: %s/", run, cut_dir));

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

int GetJapanSessions (const int run) {
  glob_t globbuf;
  const char * pattern = Form("%s/prexPrompt_pass2_%d.???.root", japan_dir, run);
  glob(pattern, 0, NULL, &globbuf);
  return globbuf.gl_pathc;
}

int GetRegSessions (const int run) {
  glob_t globbuf;
  const char * pattern = Form("%s/prexPrompt_%d_???_regress_postpan.root", reg_dir, run);
  glob(pattern, 0, NULL, &globbuf);
  return globbuf.gl_pathc;
}

map<string, pair<double, double>> GetEvtValues (const int run, vector<char *> vars, TCut cut = "ErrorFlag == 0") {
  map<string, pair<double, double>> res;
  long long entries = 0;
  const int s = GetJapanSessions(run);
  gROOT->SetBatch(1);
  TCanvas * c = new TCanvas("c", "c", 800, 600);
  for (int i=0; i<s; i++) {
    double ErrorFlag;
    const char * root_file = Form("%s/prexPrompt_pass2_%d.%03d.root", japan_dir, run, i);
    TFile fin(root_file, "read");
    if (!fin.IsOpen()) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can't open root file: " << root_file;
      return res;
    }
    TTree * tin = (TTree*) fin.Get("evt");
    if (! tin) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can't receive evt tree from root file: " << root_file;
      return res;
    }

    long long n = tin->GetEntries(cut);
    double mean, rms;
    for (char * var : vars) {
      c->cd();
      const char * hname = Form("h_%s", var);
      tin->Draw(Form("%s>>%s", var, hname), cut);
      TH1F *h = (TH1F*) gROOT->FindObject(hname);
      if (h) {
        mean = h->GetMean();
        rms  = h->GetRMS();
        h->Delete();
        h = NULL;
      } else {
        mean = 0;
        rms  = 0;
      }
      if (res.find(var) != res.end()) {
        double m = (entries*res[var].first + n*mean) / (entries + n);
        double r = ( entries*pow(res[var].second, 2) + pow(res[var].first, 2)
                   + n*pow(rms, 2) + pow(mean, 2) ) / (entries + n) - pow(m, 2);
        res[var] = make_pair(m, r);
      } else {
        res[var] = make_pair(mean, rms);
      }
    }
    entries += n;
  }
  c->Close();
  delete c;
  c = NULL;
  res["entries"] = make_pair(entries, entries);
  return res;
}

map<string, pair<double, double>> GetSlowValues (const int run, vector<char *> vars, TCut cut = "") {
  map<string, pair<double, double>> res;
  long long entries = 0;
  const int s = GetJapanSessions(run);
  gROOT->SetBatch(1);
  TCanvas * c = new TCanvas("c", "c", 800, 600);
  for (int i=0; i<s; i++) {
    double ErrorFlag;
    const char * root_file = Form("%s/prexPrompt_pass2_%d.%03d.root", japan_dir, run, i);
    TFile fin(root_file, "read");
    if (!fin.IsOpen()) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can't open root file: " << root_file;
      return res;
    }
    TTree * tin = (TTree*) fin.Get("slow");
    if (! tin) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can't receive evt tree from root file: " << root_file;
      return res;
    }

    long long n = tin->GetEntries(cut);
    double mean, rms;
    for (char * var : vars) {
      c->cd();
      const char * hname = Form("h_%s", var);
      tin->Draw(Form("%s>>%s", var, hname), cut);
      TH1F *h = (TH1F*) gROOT->FindObject(hname);
      if (h) {
        mean = h->GetMean();
        rms  = h->GetRMS();
        h->Delete();
        h = NULL;
      } else {
        mean = 0;
        rms  = 0;
      }
      if (res.find(var) != res.end()) {
        double m = (entries*res[var].first + n*mean) / (entries + n);
        double r = ( entries*pow(res[var].second, 2) + pow(res[var].first, 2)
                   + n*pow(rms, 2) + pow(mean, 2) ) / (entries + n) - pow(m, 2);
        res[var] = make_pair(m, r);
      } else {
        res[var] = make_pair(mean, rms);
      }
    }
    entries += n;
  }
  c->Close();
  delete c;
  c = NULL;
  res["entries"] = make_pair(entries, entries);
  return res;
}

map<string, pair<double, double>> GetRegValues (const int run, vector<char *> vars, TCut cut = "ok_cut") {
  map<string, pair<double, double>> res;
  long long entries = 0;
  const int s = GetRegSessions(run);
  gROOT->SetBatch(1);
  TCanvas * c = new TCanvas("c", "c", 800, 600);
  for (int i=0; i<s; i++) {
    double ErrorFlag;
    const char * root_file = Form("%s/prexPrompt_%d_%03d_regress_postpan.root", reg_dir, run, i);
    TFile fin(root_file, "read");
    if (!fin.IsOpen()) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can't open root file: " << root_file << ENDL;
      return res;
    }
    TTree * tin = (TTree*) fin.Get("reg");
    if (! tin) {
      cerr << __PRETTY_FUNCTION__ << "ERROR:\t Can't receive reg tree from root file: " << root_file << ENDL;
      return res;
    }

    long long n = tin->GetEntries(cut);
    double mean, rms;
    for (char * var : vars) {
      c->cd();
      const char * hname = Form("h_%s", var);
      tin->Draw(Form("%s>>%s", var, hname), cut);
      TH1F *h = (TH1F*) gROOT->FindObject(hname);
      if (h) {
        mean = h->GetMean();
        rms  = h->GetRMS();
        h->Delete();
        h = NULL;
      } else {
        mean = 0;
        rms  = 0;
      }
      if (res.find(var) != res.end()) {
        double m = (entries*res[var].first + n*mean) / (entries + n);
        double r = ( entries*pow(res[var].second, 2) + pow(res[var].first, 2)
                   + n*pow(rms, 2) + pow(mean, 2) ) / (entries + n) - pow(m, 2);
        res[var] = make_pair(m, r);
      } else {
        res[var] = make_pair(mean, rms);
      }
    }
    entries += n;
  }
  c->Close();
  delete c;
  c = NULL;
  res["entries"] = make_pair(entries, entries);
  return res;
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
