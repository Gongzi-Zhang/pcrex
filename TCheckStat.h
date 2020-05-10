#ifndef TCHECKSTAT_H
#define TCHECKSTAT_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <glob.h>
#include "mysql.h"
#include <libgen.h>

#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCollection.h"
#include "TGraphErrors.h"
#include "TH1F.h"
#include "TPaveStats.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TAxis.h"

#include "const.h"
#include "line.h"
#include "TRun.h"
#include "TConfig.h"


typedef struct {double mean, err, rms;} DATASET;

enum StatsType { mean, rms };
enum Format {pdf, png};

using namespace std;

class TCheckStat {

    // ClassDe (TCheckStat, 0) // check statistics

  private:
    TConfig fConf;
    Format format = pdf;
    const char * out_name = "check";
    const char * dir	= "/adaqfs/home/apar/PREX/prompt/results/";
    const char * prefix = "prexPrompt_";
    const char * suffix = "_regress_postpan";
	  char	 midfix = '_';
    const char * tree   = "mini";
    bool  check_latest_run = false;
    int	  latest_run;
    int	  nRuns;
    int	  nBoldRuns;
    int	  nSlugs;
    int	  nMiniruns;
    int	  nVars;
    int	  nSolos;
    int	  nComps;
    int	  nSlopes;
    int	  nCors;
    set<int> fRuns;
    set<int> fSlugs;
    set<int> fBoldRuns;
    set<string>	  fVars;
    set<string>	  fSolos;
    set<string>	  fSoloPlots;
    set< pair<string, string> >	fComps;
    set< pair<string, string> >	fCompPlots;
    set< pair<string, string> >	fSlopes;
    set< pair<string, string> >	fSlopePlots;
    set< pair<string, string> >	fCors;
    set< pair<string, string> >	fCorPlots;
    map<string, VarCut>			fSoloCuts;
    map< pair<string, string>, CompCut>	fCompCuts;
    map< pair<string, string>, VarCut>  fSlopeCuts;
    map< pair<string, string>, CorCut>  fCorCuts;

    map<int, int> fSessions;
    map<int, int> fSigns;
    map<string, string> fUnits;
    map<pair<string, string>, pair<int, int>>	  fSlopeIndexes;
    map<string, StatsType> fStatsTypes;
    map<string, string> fVarNames;
    map<string, DATASET> vars_buf;
    // double slopes_buf[ROWS][COLS];
    // double slopes_err_buf[ROWS][COLS];
    int      rows, cols;
    double * slopes_buf;
    double * slopes_err_buf;

    vector<pair<int, int>> fMiniruns;
    map<string, set<pair<int, int>>>  fSoloBadMiniruns;
    map<pair<string, string>, set<pair<int, int>>>	  fCompBadMiniruns;
    map<pair<string, string>, set<pair<int, int>>>	  fSlopeBadMiniruns;
    map<pair<string, string>, set<pair<int, int>>>	  fCorBadMiniruns;
    map<string, vector<DATASET>> fVarValues;
    map<pair<string, string>, vector<double>> fSlopeValues;
    map<pair<string, string>, vector<double>> fSlopeErrs;

    TCanvas * c;
  public:
     TCheckStat(const char*);
     ~TCheckStat();
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutSuffix(const char * suf);
     void SetDir(const char * d);
     void SetLatestRun(int run);
     void SetRuns(set<int> runs);
     void SetBoldRuns(set<int> runs);
     void SetSlugs(set<int> slugs);
     void CheckRuns();
     void CheckVars();
     bool CheckVar(string exp);
     void GetValues();
     void CheckValues();
     void Draw();
     void DrawSolos();
     void DrawSlopes();
     void DrawComps();
     void DrawCors();
};

// ClassImp(TCheckStat);

TCheckStat::TCheckStat(const char* config_file) :
  fConf(config_file)
{
  fConf.ParseConfFile();
  fBoldRuns = fConf.GetBoldRuns();
  fRuns   = fConf.GetRuns();
  fVars	  = fConf.GetVars();
  fSolos  = fConf.GetSolos();
  fComps  = fConf.GetComps();
  fSlopes = fConf.GetSlopes();
  fCors	  = fConf.GetCors();
  nSlopes = fSlopes.size();
  nRuns   = fRuns.size();

  fSoloPlots  = fConf.GetSoloPlots();
  fCompPlots  = fConf.GetCompPlots();
  fSlopePlots = fConf.GetSlopePlots();
  fCorPlots   = fConf.GetCorPlots();
  fSoloCuts   = fConf.GetSoloCuts();
  fCompCuts   = fConf.GetCompCuts();
  fSlopeCuts  = fConf.GetSlopeCuts();
  fCorCuts    = fConf.GetCorCuts();

  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetTreeName())  tree = fConf.GetTreeName();

  gROOT->SetBatch(1);
}

TCheckStat::~TCheckStat() {
  cerr << __PRETTY_FUNCTION__ << ":INFO\t Release TCheckStat\n";
}

void TCheckStat::SetDir(const char * d) {
  struct stat info;
  if (stat( d, &info) != 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t can't access specified dir: " << dir << endl;
    exit(30);
  } else if ( !(info.st_mode & S_IFDIR)) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t not a dir: " << dir << endl;
    exit(31);
  }
  dir = d;
}

void TCheckStat::SetOutSuffix(const char * suf) {
  if (strcmp(suf, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(suf, "png") == 0) {
    format = png;
  } else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unknow output format: " << suf << endl;
    exit(40);
  }
}

void TCheckStat::SetLatestRun(int run) {
  if (run < START_RUN || run > END_RUN) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t latest run is out of range (" 
	 << START_RUN << "-" << END_RUN << "): " << run << endl;
    exit(41);
  }
  char * type = GetRunType(run);
  if (!type || strcmp(type, "Production") != 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t latest run is not a production run: " << run << endl;
    exit(42);
  }
  latest_run = run;
  check_latest_run = true;
}

void TCheckStat::SetRuns(set<int> runs) {
  for(set<int>::iterator it=runs.cbegin(); it != runs.cend(); it++) {
    int run = *it;
    if (run < START_RUN || run > END_RUN) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << endl;
      continue;
    }
    fBoldRuns.insert(run);
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
}

void TCheckStat::SetBoldRuns(set<int> runs) {
  for(set<int>::iterator it=runs.cbegin(); it != runs.cend(); it++) {
    int run = *it;
    if (run < START_RUN || run > END_RUN) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << endl;
      continue;
    }
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
  nBoldRuns = fBoldRuns.size();
}

void TCheckStat::SetSlugs(set<int> slugs) {
  for(set<int>::iterator it=slugs.cbegin(); it != slugs.cend(); it++) {
    int slug = *it;
    if (slug < START_SLUG || slug > END_SLUG) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid slug number (" << START_SLUG << "-" << END_SLUG << "): " << slug << endl;
      continue;
    }
    fSlugs.insert(slug);
  }
  nSlugs = fSlugs.size();
}

void TCheckStat::CheckRuns() {
  StartConnection();

  if (nSlugs > 0) {
    set<int> runs;
    for(set<int>::iterator it=fSlugs.cbegin(); it != fSlugs.cend(); it++) {
      runs = GetRunsFromSlug(*it);
      for (set<int>::iterator it_r=runs.cbegin(); it_r != runs.cend(); it_r++) {
        fRuns.insert(*it_r);
      }
    }
  }
  nRuns = fRuns.size();

  // check runs against database
  if (check_latest_run) {
    fRuns.clear();
    fBoldRuns.clear();
    fBoldRuns.insert(latest_run);
    int i=0;
    int run = latest_run - 1;
    while (i<10 && run > START_RUN) { // get 10 valid runs before latest_run
      char * type = GetRunType(run);
      if (type && strcmp(type, "Production") == 0) {
        fRuns.insert(run);
        i++;
      }
      run--;
    }
  } else {
    GetValidRuns(fRuns);
  }
  nRuns = fRuns.size();
  nBoldRuns = fBoldRuns.size();

  glob_t globbuf;
  bool flag = false;
  for (set<int>::iterator it = fRuns.cbegin(); it != fRuns.cend(); ) {
    int run = *it;
    const char * pattern  = Form("%s/*%d[_.]???*.root", dir, run);
    glob(pattern, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << __PRETTY_FUNCTION__ << ":WARNING\t no root file for run " << run << ". Ignore it.\n";
      it = fRuns.erase(it);
      continue;
    }
    fSessions[run] = globbuf.gl_pathc;
    if (!flag) {
      char * path = globbuf.gl_pathv[0];
      const char * bname = basename(path);
      pair<int, int> index1 = Index(bname, Form("%d", run));
      prefix = Sub(bname, 0, index1.first-0);
      const int l = Size(Form("%d", run));
      midfix = bname[index1.first+l];
      pair<int, int> index2 = Index(bname, ".root");
      suffix = Sub(bname, index1.first+l+4, index2.first-(index1.first+l+4));
      flag = true;
    }
    it++;
  }
  globfree(&globbuf);

  nRuns = fRuns.size();
  if (nRuns == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t No valid runs specified!\n";
    exit(10);
  }

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nRuns << " valid runs specified:\n";
  for(set<int>::iterator it=fRuns.cbegin(); it!=fRuns.cend(); it++) {
    cout << "\t" << *it << endl;
  }

  fSigns = GetSign(fRuns); // sign corrected
  EndConnection();
}

void TCheckStat::CheckVars() {
  srand(time(NULL));
  int s = rand() % nRuns;
  set<int>::iterator it_r=fRuns.cbegin();
  for(int i=0; i<s; i++)
    it_r++;

  while (it_r != fRuns.cend()) {
    int run = *it_r;
    const char * file_name = Form("%s/%s%d%c000%s.root", dir, prefix, run, midfix, suffix);
    TFile * f_rootfile = new TFile(file_name, "read");
    if (f_rootfile->IsOpen()) {
      TTree * tin = (TTree*) f_rootfile->Get(tree); // receive minitree
      vector<TString> * l_iv = (vector<TString>*) f_rootfile->Get("IVNames");
      vector<TString> * l_dv = (vector<TString>*) f_rootfile->Get("DVNames");
      if (tin != NULL && l_iv != NULL && l_dv != NULL) {
        TObjArray * l_var = tin->GetListOfBranches();
        bool error_var_flag = false;
        for (set<string>::iterator it_v=fVars.cbegin(); it_v != fVars.cend(); it_v++) {
          if (!l_var->FindObject(it_v->c_str())) {
            cerr << __PRETTY_FUNCTION__ << ":WARNING\t Variable not found: " << *it_v << endl;
            it_v = fVars.erase(it_v);
            error_var_flag = true;
          }
        }
        if (error_var_flag) {
          TIter next(l_var);
          TBranch *br;
          cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid variables:\n";
          while (br = (TBranch*) next()) {
            cout << "\t" << br->GetName() << endl;
          }
        }

        if (nSlopes>0) {
          rows = l_dv->size();
          cols = l_iv->size();
          slopes_buf = new double[rows*cols];
          slopes_err_buf = new double[rows*cols];
          // if (rows != ROWS || cols != COLS) {
          //   cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unmatched slope array size: " << rows << "x" << cols << " in run: " << run << endl;
          //   exit(20);
          // }
          bool error_dv_flag = false;
          bool error_iv_flag = false;
          for (set<pair<string, string>>::iterator it_s=fSlopes.cbegin(); it_s != fSlopes.cend(); ) {
            string dv = it_s->first;
            string iv = it_s->second;
            vector<TString>::iterator it_dv = find(l_dv->begin(), l_dv->end(), dv);
            vector<TString>::iterator it_iv = find(l_iv->begin(), l_iv->end(), iv);
            if (it_dv == l_dv->end()) {
              cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid dv name for slope: " << dv << endl;
              it_s = fSlopes.erase(it_s);
              error_dv_flag = true;
              continue;
            }
            if (it_iv == l_iv->end()) {
              cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid iv name for slope: " << iv << endl;
              it_s = fSlopes.erase(it_s);
              error_iv_flag = true;
              continue;
            }
            fSlopeIndexes[*it_s] = make_pair(it_dv-l_dv->cbegin(), it_iv-l_iv->cbegin());
            it_s++;
          }
          if (error_dv_flag) {
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid dv names:\n";
            for (vector<TString>::iterator it = l_dv->begin(); it != l_dv->end(); it++) 
              cout << "\t" << (*it).Data() << endl;
          }
          if (error_iv_flag) {
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid dv names:\n";
            for (vector<TString>::iterator it = l_iv->begin(); it != l_iv->end(); it++) 
              cout << "\t" << (*it).Data() << endl;
          }
        }
        f_rootfile->Close();
        break;
      }
    } 
      
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t root file of run: " << run << " doesn't exit or is broken, ignore it.\n";
    it_r = fRuns.erase(it_r);
    if (fBoldRuns.find(run) != fBoldRuns.cend())
      fBoldRuns.erase(run);

    if (it_r == fRuns.cend())
      it_r = fRuns.cbegin();
  }

  nRuns = fRuns.size();
  if (nRuns == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no valid runs, aborting.\n";
    exit(10);
  }

  nVars = fVars.size();
  nSlopes = fSlopes.size();
  if (nVars == 0 && nSlopes == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no valid variables specified, aborting.\n";
    exit(11);
  }

  for (set<string>::iterator it=fVars.cbegin(); it!=fVars.cend(); it++) {
    vars_buf[*it] = {1024, 1024, 1024};
  }

  for (set<string>::iterator it=fSolos.cbegin(); it!=fSolos.cend(); ) {
    if (!CheckVar(*it))
      it = fSolos.erase(it);
    else
      it++;
  }

  for (set<pair<string, string>>::iterator it=fComps.cbegin(); it!=fComps.cend(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
      it = fComps.erase(it);
      continue;
    }
    if (fStatsTypes[it->first] != fStatsTypes[it->second]) {
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t different statistical types for comparison in: " << it->first << " , " << it->second << endl;
      it = fComps.erase(it);
    }
    it++;
  }

  for (set<pair<string, string>>::iterator it=fCors.cbegin(); it!=fCors.cend(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second))
      it = fCors.erase(it);
    else
      it++;
  }

  nSolos = fSolos.size();
  nComps = fComps.size();
  nCors  = fCors.size();

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSolos << " valid solo variables specified:\n";
  for(set<string>::iterator it=fSolos.cbegin(); it!=fSolos.cend(); it++) {
    cout << "\t" << *it << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nComps << " valid comparisons specified:\n";
  for(set<pair<string, string>>::iterator it=fComps.cbegin(); it!=fComps.cend(); it++) {
    cout << "\t" << it->first << " , " << it->second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSlopes << " valid slopes specified:\n";
  for(set<pair<string, string>>::iterator it=fSlopes.cbegin(); it!=fSlopes.cend(); it++) {
    cout << "\t" << it->first << " : " << it->second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nCors << " valid correlations specified:\n";
  for(set<pair<string, string>>::iterator it=fCors.cbegin(); it!=fCors.cend(); it++) {
    cout << "\t" << it->first << " : " << it->second << endl;
  }
}

bool TCheckStat::CheckVar(string exp) {
  string var;
  StatsType stype = mean; // default to be mean
  if (exp.find('.') == string::npos) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t no statistical type (mean or rms) in: " << exp << endl;
    return false;
  }

  int idx = exp.find('.');
  var = exp.substr(0, idx);
  const char * stats = exp.substr(idx+1).c_str();
  if (strcmp(stats, "mean") == 0)
    stype = mean;
  else if (strcmp(stats, "rms") == 0)
    stype = rms;
  else {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t unknown statistical type in: " << exp << endl;
    return false;
  }

  if (fVars.find(var) == fVars.cend()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t Unknown variable in: " << exp << endl;
    return false;
  }

  fVarNames[exp] = var;
  fStatsTypes[exp] = stype;

  return true;
}

void TCheckStat::GetValues() {
  for (set<int>::iterator it_r=fRuns.cbegin(); it_r!=fRuns.cend(); it_r++) {
    int run = *it_r;
    for (int session=0; session<fSessions[run]; session++) {
      const char * file_name = Form("%s/%s%d%c%03d%s.root", dir, prefix, run, midfix, session, suffix);
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d\n", run, session)
           << Form("%s%d%c%03d%s.root", prefix, run, midfix, session, suffix) << endl;
      TTree * tin = (TTree*) f_rootfile.Get(tree); // receive minitree
      if (! tin) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such tree: " << tree << " in root file: " << file_name << endl;
        continue;
      }

      int minirun;
      tin->SetBranchAddress("minirun", &minirun);

      for (set<string>::iterator it_v=fVars.cbegin(); it_v!=fVars.cend(); it_v++)
        tin->SetBranchAddress(it_v->c_str(), &(vars_buf[*it_v]));

      if (nSlopes > 0) {
        tin->SetBranchAddress("coeff", slopes_buf);
        tin->SetBranchAddress("err_coeff", slopes_err_buf);
      }

      const int nentries = tin->GetEntries();  // number of miniruns
      for(int n=0; n<nentries; n++) { // loop through the miniruns
        tin->GetEntry(n);

        fMiniruns.push_back(make_pair(run, minirun));
        for (set<string>::iterator it_v=fVars.cbegin(); it_v!=fVars.cend(); it_v++) {
          double unit = 1;
          if (it_v->find("asym") != string::npos) {
            unit = ppm;
          }
          else if (it_v->find("diff")) {
            unit = um/mm;
          }

          vars_buf[*it_v].mean /= (unit*1e-3);
          vars_buf[*it_v].err  /= (unit*1e-3);
          vars_buf[*it_v].rms  /= unit;
          vars_buf[*it_v].mean *= fSigns[run];
          fVarValues[*it_v].push_back(vars_buf[*it_v]);
        }
        for (set<pair<string, string>>::iterator it_s=fSlopes.cbegin(); it_s!=fSlopes.cend(); it_s++) {
          double unit = ppm/(um/mm);
          fSlopeValues[*it_s].push_back(slopes_buf[fSlopeIndexes[*it_s].first*cols+fSlopeIndexes[*it_s].second]/unit);
          fSlopeErrs[*it_s].push_back(slopes_err_buf[fSlopeIndexes[*it_s].first*cols+fSlopeIndexes[*it_s].second]/unit);
        }
      }

      tin->Delete();
      f_rootfile.Close();
    }
  }
  nMiniruns = fMiniruns.size();

  cout << __PRETTY_FUNCTION__ << ":INFO\t Read " << nMiniruns << " miniruns in total\n";
}

void TCheckStat::CheckValues() {
  for (set<string>::iterator it=fSolos.begin(); it!=fSolos.end(); it++) {
    string var = *it;

    const double low_cut  = fSoloCuts[*it].low;
    const double high_cut = fSoloCuts[*it].high;
    const double stat_cut = fSoloCuts[*it].stability;
    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean, sigma = 0;
    for (int i=0; i<nMiniruns; i++) {
      double val;
      if (fStatsTypes[var] == mean)
        val = fVarValues[fVarNames[var]][i].mean;
      else if (fStatsTypes[var] == rms)
        val = fVarValues[fVarNames[var]][i].rms;

      if (i == 0) {
        mean = val;
        sigma = 0;
      }

      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (stat_cut != 1024 && abs(val-mean) > stat_cut*sigma)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in " << var
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        fSoloPlots.insert(var);
        fSoloBadMiniruns[var].insert(fMiniruns[i]);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
  }

  for (set<pair<string, string>>::iterator it=fComps.cbegin(); it!=fComps.cend(); it++) {
    string var1 = it->first;
    string var2 = it->second;
    const double low_cut  = fCompCuts[*it].low;
    const double high_cut = fCompCuts[*it].high;
    const double diff_cut = fCompCuts[*it].diff;
    for (int i=0; i<nMiniruns; i++) {
      double val1, val2;
      if (fStatsTypes[var1] == mean) {
        val1 = fVarValues[fVarNames[var1]][i].mean;
        val2 = fVarValues[fVarNames[var2]][i].mean;
      } else if (fStatsTypes[var1] == rms) {
        val1 = fVarValues[fVarNames[var1]][i].rms;
        val2 = fVarValues[fVarNames[var2]][i].rms;
      }

      if ( (low_cut  != 1024 && (val1 < low_cut  || val2 < low_cut))
        || (high_cut != 1024 && (val1 > high_cut || val2 > high_cut))
        || (diff_cut != 1024 && abs(val1-val2) > diff_cut)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Comp: " << var1 << " vs " << var2 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        fCompPlots.insert(*it);
        fCompBadMiniruns[*it].insert(fMiniruns[i]);
      }
    }
  }

  for (set<pair<string, string>>::iterator it=fSlopes.cbegin(); it!=fSlopes.cend(); it++) {
    // string dv = it->first;
    // string iv = it->second;
    const double low_cut  = fSlopeCuts[*it].low;
    const double high_cut = fSlopeCuts[*it].high;
    const double stat_cut = fSlopeCuts[*it].stability;

    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean = fSlopeValues[*it][0];
    double sigma = fSlopeErrs[*it][0];
    for (int i=0; i<nMiniruns; i++) {
      double val = fSlopeValues[*it][i];
      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (stat_cut != 1024 && abs(val-mean) > stat_cut*sigma)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in slope: " << it->first << " vs " << it->second 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        fSlopePlots.insert(*it);
        fSlopeBadMiniruns[*it].insert(fMiniruns[i]);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
  }

  for (set<pair<string, string>>::iterator it=fCors.cbegin(); it!=fCors.cend(); it++) {
    string yvar = it->first;
    string xvar = it->second;
    const double xlow_cut   = fCorCuts[*it].xlow;
    const double xhigh_cut  = fCorCuts[*it].xhigh;
    const double ylow_cut   = fCorCuts[*it].ylow;
    const double yhigh_cut  = fCorCuts[*it].yhigh;
    // const double 
    for (int i=0; i<nMiniruns; i++) {
      double xval, yval;
      if (fStatsTypes[xvar] == mean)
        xval = fVarValues[fVarNames[xvar]][i].mean;
      else if (fStatsTypes[xvar] == rms)
        xval = fVarValues[fVarNames[xvar]][i].rms;

      if (fStatsTypes[yvar] == mean)
        yval = fVarValues[fVarNames[yvar]][i].mean;
      else if (fStatsTypes[yvar] == rms)
        yval = fVarValues[fVarNames[yvar]][i].rms;

      if ( (xlow_cut  != 1024 && xval < xlow_cut)
        || (xhigh_cut != 1024 && xval > xhigh_cut)
        || (ylow_cut  != 1024 && yval < ylow_cut)
        || (yhigh_cut != 1024 && yval > yhigh_cut) ) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Cor: " << yvar << " vs " << xvar 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        fCorPlots.insert(*it);
        fCorBadMiniruns[*it].insert(fMiniruns[i]);
      }
    }
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t done with checking values\n";
}

void TCheckStat::Draw() {
  c = new TCanvas("c", "c", 800, 600);
  c->SetGridy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(1110);
  gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  gStyle->SetBarWidth(1.05);

  if (format == pdf)
    c->Print(Form("%s.pdf[", out_name));

  DrawSolos();
  DrawSlopes();
  DrawComps();
  DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << __PRETTY_FUNCTION__ << ":INFO\t done with drawing plots\n";
}

/*
void TCheckStat::DrawSolos() {
  for (set<string>::iterator it=fSoloPlots.cbegin(); it!=fSoloPlots.cend(); it++) {
    string var = *it;
    string var_name = fVarNames[var];
    StatsType vt = fStatsTypes[var];
    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * gc = NULL;
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double val, err;
      if (vt == mean) {
        val = fVarValues[var_name][i].mean;
        err = fVarValues[var_name][i].err;
      } else if (vt == rms) {
        val = fVarValues[var_name][i].rms;
        err = 0;
      }
      g->SetPoint(i, i+1, val);
      g->SetPointError(i, 0, err);
      
      if (fBoldRuns.find(fMiniruns[i].first) != fBoldRuns.cend()) {
        g_bold->SetPoint(ibold, i+1, val);
        g_bold->SetPointError(ibold, 0, err);
        ibold++;
      }
      if (fSoloBadMiniruns[var].find(fMiniruns[i]) != fSoloBadMiniruns[var].cend()) {
        g_bad->SetPoint(ibad, i+1, val);
        g_bad->SetPointError(ibad, 0, err);
        ibad++;
      }
    }
    g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    g->SetMarkerStyle(20);
    g->SetTitle(var.c_str());
    gc = (TGraphErrors*) g->Clone();
    g_bold->SetMarkerStyle(20);
    g_bold->SetMarkerSize(1.3);
    g_bold->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerColor(kRed);

    g->Fit("pol0");
    TF1 * fit= g->GetFunction("pol0");
    double mean_value = fit->GetParameter(0);

    TH1F * pull = NULL; 
    if (vt == mean) {
      pull = new TH1F("pull", "", nMiniruns, 0, nMiniruns);

      for (int i=0; i<nMiniruns; i++) {
        double ratio = 0;
        if (fVarValues[var_name][i].err != 0)
          ratio = (fVarValues[var_name][i].mean-mean_value)/fVarValues[var_name][i].err;

        pull->SetBinContent(i+1, ratio);
      }
      pull->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    }

    TAxis * ax = NULL;
    if (vt == mean) {
      c->cd();
      TPad * p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
      p1->Draw();
      p1->SetGridy();
      p1->SetBottomMargin(0);
      TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
      p2->Draw();
      p2->SetGrid();
      p2->SetTopMargin(0);
      p2->SetBottomMargin(0.17);

      p1->cd();
      g->GetXaxis()->SetLabelSize(0);
      g->GetXaxis()->SetNdivisions(-(nMiniruns+1));
      g->Draw("AP");
      gc->Draw("P same");
      g_bold->Draw("P same");
      g_bad->Draw("P same");
      p1->Update();
      
      p2->cd();
      pull->SetStats(kFALSE);
      pull->SetFillColor(kGreen);
      // pull->SetLineColor(kGreen);
      // pull->LabelsDeflate("X");
      // pull->LabelsOption("v");
      pull->SetBarOffset(0.5);
      pull->SetBarWidth(1);
      pull->Draw("B");
      ax = pull->GetXaxis();
      p2->Update();
    } else if (vt == rms) {
      c->SetBottomMargin(0.16);
      g->Draw("AP");
      ax = g->GetXaxis();
      gc->Draw("P same");
      g_bold->Draw("P same");
      g_bad->Draw("P same");
    }

    ax->SetNdivisions(-(nMiniruns+1));
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    // ax->SetLabelOffset(0.02);
    for (int i=0; i<=nMiniruns; i++) {
      ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    }

    c->Modified();
    c->Print(Form("%s.pdf", out_name));
    c->Clear();
    if (pull) {
      pull->Delete();
      pull = NULL;
    }
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Solos.\n";
}
*/

void TCheckStat::DrawSolos() {
  for (set<string>::iterator it=fSoloPlots.cbegin(); it!=fSoloPlots.cend(); it++) {
    string var = *it;
    string var_name = fVarNames[var];
    StatsType vt = fStatsTypes[var];
    string unit = "ppm";
    if (var.find("asym") != string::npos) {
      if (fStatsTypes[var] == mean)
        unit = "ppb";
      else if (fStatsTypes[var] == rms)
        unit = "ppm";
    } else if (var.find("diff") != string::npos) {
      if (fStatsTypes[var] == mean)
        unit = "nm";
      else if (fStatsTypes[var] == rms)
        unit = "um";
    }
    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * gc = NULL;
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double val, err;
      if (vt == mean) {
        val = fVarValues[var_name][i].mean;
        err = fVarValues[var_name][i].err;
      } else if (vt == rms) {
        val = fVarValues[var_name][i].rms;
        err = 0;
      }
      g->SetPoint(i, i+1, val);
      g->SetPointError(i, 0, err);
      
      if (fBoldRuns.find(fMiniruns[i].first) != fBoldRuns.cend()) {
        g_bold->SetPoint(ibold, i+1, val);
        g_bold->SetPointError(ibold, 0, err);
        ibold++;
      }
      if (fSoloBadMiniruns[var].find(fMiniruns[i]) != fSoloBadMiniruns[var].cend()) {
        g_bad->SetPoint(ibad, i+1, val);
        g_bad->SetPointError(ibad, 0, err);
        ibad++;
      }
    }
    g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    g->SetMarkerStyle(20);
    g->SetTitle((var + ";;" + unit).c_str());
    gc = (TGraphErrors*) g->Clone();
    g_bold->SetMarkerStyle(20);
    g_bold->SetMarkerSize(1.3);
    g_bold->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerColor(kRed);

    g->Fit("pol0");
    TF1 * fit= g->GetFunction("pol0");
    double mean_value = fit->GetParameter(0);

    TGraph * pull = NULL; 
    if (vt == mean) {
      pull = new TGraph();

      for (int i=0; i<nMiniruns; i++) {
        double ratio = 0;
        if (fVarValues[var_name][i].err != 0)
          ratio = (fVarValues[var_name][i].mean-mean_value)/fVarValues[var_name][i].err;

        pull->SetPoint(i, i+1, ratio);
      }
      pull->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    }

    TAxis * ax = NULL;
    if (vt == mean) {
      c->cd();
      TPad * p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
      p1->Draw();
      p1->SetGridy();
      p1->SetBottomMargin(0);
      TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
      p2->Draw();
      p2->SetGrid();
      p2->SetTopMargin(0);
      p2->SetBottomMargin(0.17);

      p1->cd();
      g->GetXaxis()->SetLabelSize(0);
      g->GetXaxis()->SetNdivisions(-(nMiniruns+1));
      g->Draw("AP");
      gc->Draw("P same");
      g_bold->Draw("P same");
      g_bad->Draw("P same");
      p1->Update();
      
      p2->cd();
      // pull->SetStats(kFALSE);
      pull->SetFillColor(kGreen);
      pull->SetLineColor(kGreen);
      // pull->LabelsDeflate("X");
      // pull->LabelsOption("v");
      // pull->SetBarOffset(0.5);
      // pull->SetBarWidth(1);
      pull->Draw("AB");
      ax = pull->GetXaxis();
      p2->Update();
    } else if (vt == rms) {
      c->SetBottomMargin(0.16);
      g->Draw("AP");
      ax = g->GetXaxis();
      gc->Draw("P same");
      g_bold->Draw("P same");
      g_bad->Draw("P same");
    }

    ax->SetNdivisions(-(nMiniruns+1));
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    // ax->SetLabelOffset(0.02);
    for (int i=0; i<=nMiniruns; i++) {
      ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, var.c_str()));

    c->Clear();
    if (pull) {
      pull->Delete();
      pull = NULL;
    }
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Solos.\n";
}

void TCheckStat::DrawSlopes() {
  for (set<pair<string, string>>::iterator it=fSlopePlots.cbegin(); it!=fSlopePlots.cend(); it++) {
    string unit = "ppb/nm";
    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * gc = NULL;
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double val, err;
      val = fSlopeValues[*it][i];
      err = fSlopeErrs[*it][i];
      g->SetPoint(i, i+1, val);
      g->SetPointError(i, 0, err);
      
      if (fBoldRuns.find(fMiniruns[i].first) != fBoldRuns.cend()) {
        g_bold->SetPoint(ibold, i+1, val);
        g_bold->SetPointError(ibold, 0, err);
        ibold++;
      }
      if (fSlopeBadMiniruns[*it].find(fMiniruns[i]) != fSlopeBadMiniruns[*it].cend()) {
        g_bad->SetPoint(ibad, i+1, val);
        g_bad->SetPointError(ibad, 0, err);
        ibad++;
      }
    }
    g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    g->SetMarkerStyle(20);
    g->SetTitle((it->first + "_" + it->second + ";;" + unit).c_str());
    gc = (TGraphErrors*) g->Clone();
    g_bold->SetMarkerStyle(20);
    g_bold->SetMarkerSize(1.3);
    g_bold->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerColor(kRed);

    g->Fit("pol0");
    TF1 * fit= g->GetFunction("pol0");
    double mean_value = fit->GetParameter(0);

    TGraph * pull = new TGraph;
    for (int i=0; i<nMiniruns; i++) {
      double ratio = 0;
      if (fSlopeErrs[*it][i] != 0)
        ratio = (fSlopeValues[*it][i]-mean_value)/fSlopeErrs[*it][i];

      pull->SetPoint(i, i+1, ratio);
    }
    pull->GetXaxis()->SetRangeUser(0, nMiniruns+1);

    c->cd();
    TPad * p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
    p1->Draw();
    p1->SetGridy();
    p1->SetBottomMargin(0);
    TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
    p2->Draw();
    p2->SetGrid();
    p2->SetTopMargin(0);
    p2->SetBottomMargin(0.17);

    p1->cd();
    g->GetXaxis()->SetLabelSize(0);
    g->GetXaxis()->SetNdivisions(-(nMiniruns+1));
    g->Draw("AP");
    gc->Draw("P same");
    g_bold->Draw("P same");
    g_bad->Draw("P same");
    p1->Update();
    
    p2->cd();
    pull->SetFillColor(kGreen);
    pull->SetLineColor(kGreen);
    pull->Draw("AB");
    TAxis * ax = pull->GetXaxis();
    ax->SetNdivisions(-(nMiniruns+1));
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    // ax->SetLabelOffset(0.02);
    for (int i=0; i<=nMiniruns; i++) {
      ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    }
    p2->Update();

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s_%s.png", out_name, it->first.c_str(), it->second.c_str()));
    c->Clear();
    pull->Delete();
    pull = NULL;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Slopes.\n";
}

void TCheckStat::DrawComps() {
  for (set<pair<string, string>>::iterator it=fCompPlots.cbegin(); it!=fCompPlots.cend(); it++) {
    string var1 = it->first;
    string var2 = it->second;
    string var_name1 = fVarNames[var1];
    string var_name2 = fVarNames[var2];
    StatsType vt = fStatsTypes[var1];
    string unit = "ppm";
    if (var1.find("asym") != string::npos) {
      if (vt == mean)
        unit = "ppb";
      else if (vt == rms)
        unit = "ppm";
    } else if (var1.find("diff") != string::npos) {
      if (vt == mean)
        unit = "nm";
      else if (vt == rms)
        unit = "um";
    }

    TGraphErrors * g1 = new TGraphErrors();
    TGraphErrors * g2 = new TGraphErrors();
    TGraphErrors * gc1 = NULL;
    TGraphErrors * gc2 = NULL;
    TGraphErrors * g_bold1 = new TGraphErrors();
    TGraphErrors * g_bold2 = new TGraphErrors();
    TGraphErrors * g_bad1  = new TGraphErrors();
    TGraphErrors * g_bad2  = new TGraphErrors();
    TH1F * h_diff = new TH1F("diff", "", nMiniruns, 0, nMiniruns);

    double min, max;
    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double val1, err1;
      double val2, err2;
      if (vt == mean) {
        val1 = fVarValues[var_name1][i].mean;
        err1 = fVarValues[var_name1][i].err;
        val2 = fVarValues[var_name2][i].mean;
        err2 = fVarValues[var_name2][i].err;
      } else if (vt == rms) {
        val1 = fVarValues[var_name1][i].rms;
        err1 = 0;
        val2 = fVarValues[var_name2][i].rms;
        err2 = 0;
      }
      if (i==0) 
        min = max = val1;

      if ((val1-err1) < min) min = val1-err1;
      if ((val1+err1) > max) max = val1+err1;
      if ((val2-err2) < min) min = val2-err2;
      if ((val2+err2) > max) max = val2+err2;

      g1->SetPoint(i, i+1, val1);
      g1->SetPointError(i, 0, err1);
      g2->SetPoint(i, i+1, val2);
      g2->SetPointError(i, 0, err2);
      h_diff->SetBinContent(i+1, val1-val2);
      
      if (fBoldRuns.find(fMiniruns[i].first) != fBoldRuns.cend()) {
        g_bold1->SetPoint(ibold, i+1, val1);
        g_bold1->SetPointError(ibold, 0, err1);
        g_bold2->SetPoint(ibold, i+1, val2);
        g_bold2->SetPointError(ibold, 0, err2);
        ibold++;
      }
      if (fCompBadMiniruns[*it].find(fMiniruns[i]) != fCompBadMiniruns[*it].cend()) {
        g_bad1->SetPoint(ibad, i+1, val1);
        g_bad1->SetPointError(ibad, 0, err1);
        g_bad2->SetPoint(ibad, i+1, val2);
        g_bad2->SetPointError(ibad, 0, err2);
        ibad++;
      }
    }
    double margin = (max-min)/10;
    min -= margin;
    max += margin;

    g1->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    h_diff->GetXaxis()->SetRangeUser(0, nMiniruns+1);

    Color_t color1 = 9;
    Color_t color2 = 28;
    g1->SetMarkerStyle(22);
    g2->SetMarkerStyle(23);
    g1->SetMarkerColor(color1);
    g2->SetMarkerColor(color2);
    g1->SetTitle(Form("#color[%d]{%s} & #color[%d]{%s};;%s", color1, it->first.c_str(), color2, it->second.c_str(), unit.c_str()));
    gc1 = (TGraphErrors*) g1->Clone();
    gc2 = (TGraphErrors*) g2->Clone();
    g_bold1->SetMarkerStyle(22);
    g_bold1->SetMarkerSize(1.3);
    g_bold1->SetMarkerColor(kBlue);
    g_bold2->SetMarkerStyle(23);
    g_bold2->SetMarkerSize(1.3);
    g_bold2->SetMarkerColor(kBlue);
    g_bad1->SetMarkerStyle(22);
    g_bad1->SetMarkerColor(kRed);
    g_bad2->SetMarkerStyle(23);
    g_bad2->SetMarkerColor(kRed);

    g1->Fit("pol0");
    g2->Fit("pol0");

    c->cd();
    TPad * p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
    p1->Draw();
    p1->SetGridy();
    p1->SetBottomMargin(0);
    TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
    p2->Draw();
    p2->SetGrid();
    p2->SetTopMargin(0);
    p2->SetBottomMargin(0.17);

    p1->cd();
    g1->GetXaxis()->SetLabelSize(0);
    g1->GetXaxis()->SetNdivisions(-(nMiniruns+1));
    g1->Draw("AP");
    g2->Draw("P same");
    p1->Update();
    g1->GetYaxis()->SetRangeUser(min, max);

    TPaveStats * st1 = (TPaveStats*) g1->FindObject("stats");
    st1->SetName("stats1");
    double width = st1->GetX2NDC() - st1->GetX1NDC();
    st1->SetX1NDC(0.1);
    st1->SetX2NDC(0.1 + width);
    st1->SetTextColor(color1);

    TPaveStats * st2 = (TPaveStats*) g2->FindObject("stats");
    st2->SetName("stats2");
    st2->SetTextColor(color2);

    gc1->Draw("P same");
    gc2->Draw("P same");
    g_bold1->Draw("P same");
    g_bold2->Draw("P same");
    g_bad1->Draw("P same");
    g_bad2->Draw("P same");
    p1->Update();
    
    p2->cd();
    h_diff->SetStats(kFALSE);
    h_diff->SetFillColor(kGreen);
    h_diff->SetBarOffset(0.5);
    h_diff->SetBarWidth(1);
    h_diff->Draw("B");
    TAxis * ax = h_diff->GetXaxis();
    ax->SetNdivisions(-(nMiniruns+1));
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    // ax->SetLabelOffset(0.02);
    for (int i=0; i<=nMiniruns; i++) {
      ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    }
    p2->Update();

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s-%s.png", out_name, var1.c_str(), var2.c_str()));
    c->Clear();
    h_diff->Delete();
    h_diff = NULL;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Comparisons.\n";
}

void TCheckStat::DrawCors() {
  for (set<pair<string, string>>::iterator it=fCorPlots.cbegin(); it!=fCorPlots.cend(); it++) {
    string xvar = it->second;
    string yvar = it->first;
    string xvar_name = fVarNames[xvar];
    string yvar_name = fVarNames[yvar];
    StatsType xvt = fStatsTypes[xvar];
    StatsType yvt = fStatsTypes[yvar];
    string xunit = "ppm";
    string yunit = "ppm";
    if (xvar.find("asym") != string::npos) {
      if (xvt == mean)
        xunit = "ppb";
      else if (xvt == rms)
        xunit = "ppm";
    } else if (xvar.find("diff") != string::npos) {
      if (xvt == mean)
        xunit = "nm";
      else if (xvt == rms)
        xunit = "um";
    }
    if (yvar.find("asym") != string::npos) {
      if (yvt == mean)
        yunit = "ppb";
      else if (yvt == rms)
        yunit = "ppm";
    } else if (yvar.find("diff") != string::npos) {
      if (yvt == mean)
        yunit = "nm";
      else if (yvt == rms)
        yunit = "um";
    }

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * gc = NULL;
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double xval, xerr;
      double yval, yerr;
      if (xvt == mean) {
        xval = fVarValues[xvar_name][i].mean;
        xerr = fVarValues[xvar_name][i].err;
      } else if (xvt == rms) {
        xval = fVarValues[xvar_name][i].rms;
        xerr = 0;
      }
      if (yvt == mean) {
        yval = fVarValues[yvar_name][i].mean;
        yerr = fVarValues[yvar_name][i].err;
      } else if (yvt == rms) {
        yval = fVarValues[yvar_name][i].rms;
        yerr = 0;
      }

      g->SetPoint(i, xval, yval);
      g->SetPointError(i, xerr, yerr);
      
      if (fBoldRuns.find(fMiniruns[i].first) != fBoldRuns.cend()) {
        g_bold->SetPoint(ibold, xval, yval);
        g_bold->SetPointError(ibold, xerr, yerr);
        ibold++;
      }
      if (fCorBadMiniruns[*it].find(fMiniruns[i]) != fCorBadMiniruns[*it].cend()) {
        g_bad->SetPoint(ibold, xval, yval);
        g_bad->SetPointError(ibold, xerr, yerr);
        ibad++;
      }
    }

    g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    g->SetMarkerStyle(20);
    g->SetTitle((it->first + " vs " + it->second + ";" + xunit + ";" + yunit).c_str());
    gc = (TGraphErrors*) g->Clone();
    g_bold->SetMarkerStyle(20);
    g_bold->SetMarkerSize(1.3);
    g_bold->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerColor(kRed);

    g->Fit("pol1");

    c->cd();
    g->Draw("AP");
    gc->Draw("P same");
    g_bold->Draw("P same");
    g_bad->Draw("P same");
    
    // TAxis * ax = g->GetXaxis();
    // ax->SetNdivisions(-(nMiniruns+1));
    // ax->ChangeLabel(1, -1, 0);  // erase first label
    // ax->ChangeLabel(-1, -1, 0); // erase last label
    // // ax->SetLabelOffset(0.02);
    // for (int i=0; i<=nMiniruns; i++) {
    //   ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    // }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s_vs_%s.png", out_name, xvar.c_str(), yvar.c_str()));
    c->Clear();
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Correlations.\n";
}
#endif
