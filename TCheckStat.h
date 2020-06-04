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
#include "TLegend.h"

#include "const.h"
#include "line.h"
#include "rcdb.h"
#include "TConfig.h"


typedef struct {double mean, err, rms;} DATASET;

enum StatsType { mean, rms };
enum Format {pdf, png};

map<int, const char *> legends = {
  {1,   "left in"},
  {-1,  "left out"},
  {2,   "right in"},
  {-2,  "right out"},
  {3,   "up in"},
  {-3,  "up out"},
};

using namespace std;

class TCheckStat {

    // ClassDe (TCheckStat, 0) // check statistics

  private:
    TConfig fConf;
    Format format = pdf;
    const char *out_name = "check";
    const char *dir	= "/adaqfs/home/apar/PREX/prompt/results/";
    const char *prefix = "prexPrompt_";
    const char *suffix = "_regress_postpan";
	  char	midfix = '_';
    const char *tree   = "mini";
    bool  check_latest_run = false;
    bool  sign = false;
    vector<int> flips;
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
    set< pair<string, string> >	fComps;
    set< pair<string, string> >	fSlopes;
    set< pair<string, string> >	fCors;
    vector<string>	                fSoloPlots;
    vector< pair<string, string> >	fCompPlots;
    vector< pair<string, string> >	fSlopePlots;
    vector< pair<string, string> >	fCorPlots;
    map<string, VarCut>			            fSoloCuts;
    map< pair<string, string>, VarCut>	fCompCuts;
    map< pair<string, string>, VarCut>  fSlopeCuts;
    map< pair<string, string>, VarCut>  fCorCuts;

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
     void SetOutFormat(const char * f);
     void SetDir(const char * d);
     void SetLatestRun(int run);
     void SetRuns(set<int> runs);
     void SetBoldRuns(set<int> runs);
     void SetSlugs(set<int> slugs);
     void SetSign() {sign = true;}
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

     // auxiliary funcitons
     const char * GetUnit(string var);
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

void TCheckStat::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unknow output format: " << f << endl;
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
  for(set<int>::const_iterator it=runs.cbegin(); it != runs.cend(); it++) {
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
  for(set<int>::const_iterator it=runs.cbegin(); it != runs.cend(); it++) {
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
  for(set<int>::const_iterator it=slugs.cbegin(); it != slugs.cend(); it++) {
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
    for(set<int>::const_iterator it=fSlugs.cbegin(); it != fSlugs.cend(); it++) {
      runs = GetRunsFromSlug(*it);
      for (set<int>::const_iterator it_r=runs.cbegin(); it_r != runs.cend(); it_r++) {
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

  bool flag = false;
  for (set<int>::const_iterator it = fRuns.cbegin(); it != fRuns.cend(); ) {
    int run = *it;
    const char * pattern  = Form("%s/*%d[_.]???*.root", dir, run);
    glob_t globbuf;
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
    globfree(&globbuf);
  }

  nRuns = fRuns.size();
  if (nRuns == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t No valid runs specified!\n";
    EndConnection();
    exit(10);
  }

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nRuns << " valid runs specified:\n";
  for(int run : fRuns) {
    cout << "\t" << run << endl;
  }

  fSigns = GetSign(fRuns); // sign corrected
  for (map<int, int>::const_iterator it = fSigns.cbegin(); it!=fSigns.cend(); it++) {
    if (find(flips.cbegin(), flips.cend(), (*it).second) == flips.cend())
      flips.push_back((*it).second);
  }

  EndConnection();
}

void TCheckStat::CheckVars() {
  srand(time(NULL));
  int s = rand() % nRuns;
  set<int>::const_iterator it_r=fRuns.cbegin();
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
        for (set<string>::const_iterator it_v=fVars.cbegin(); it_v != fVars.cend(); it_v++) {
          if (!l_var->FindObject(it_v->c_str())) {
            cerr << __PRETTY_FUNCTION__ << ":WARNING\t Variable not found: " << *it_v << endl;
            it_v = fVars.erase(it_v);
            error_var_flag = true;
            if (it_v == fVars.cend()) 
              break;
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
          for (set<pair<string, string>>::const_iterator it_s=fSlopes.cbegin(); it_s != fSlopes.cend(); ) {
            string dv = it_s->first;
            string iv = it_s->second;
            vector<TString>::const_iterator it_dv = find(l_dv->cbegin(), l_dv->cend(), dv);
            vector<TString>::const_iterator it_iv = find(l_iv->cbegin(), l_iv->cend(), iv);
            if (it_dv == l_dv->end() || it_iv == l_iv->end()) {
							if (it_dv == l_dv->end()) {
								cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid dv name for slope: " << dv << endl;
								error_dv_flag = true;
							} else {
								cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid iv name for slope: " << iv << endl;
								error_iv_flag = true;
							}

							vector<pair<string, string>>::iterator it_p = find(fSlopePlots.begin(), fSlopePlots.end(), *it_s);
							if (it_p != fSlopePlots.cend())
								fSlopePlots.erase(it_p);

							map<pair<string, string>, VarCut>::const_iterator it_c = fSlopeCuts.find(*it_s);
							if (it_c != fSlopeCuts.cend())
								fSlopeCuts.erase(it_c);
              it_s = fSlopes.erase(it_s);
              continue;
            }
            fSlopeIndexes[*it_s] = make_pair(it_dv-l_dv->cbegin(), it_iv-l_iv->cbegin());
            it_s++;
          }
          if (error_dv_flag) {
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid dv names:\n";
            for (vector<TString>::const_iterator it = l_dv->cbegin(); it != l_dv->cend(); it++) 
              cout << "\t" << (*it).Data() << endl;
          }
          if (error_iv_flag) {
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid dv names:\n";
            for (vector<TString>::const_iterator it = l_iv->cbegin(); it != l_iv->cend(); it++) 
              cout << "\t" << (*it).Data() << endl;
          }
        }
        tin->Delete();
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

  for (set<string>::const_iterator it=fVars.cbegin(); it!=fVars.cend(); it++) {
    vars_buf[*it] = {1024, 1024, 1024};
  }

  for (set<string>::const_iterator it=fSolos.cbegin(); it!=fSolos.cend();) {
    if (!CheckVar(*it)) {
			vector<string>::iterator it_p = find(fSoloPlots.begin(), fSoloPlots.end(), *it);
			if (it_p != fSoloPlots.cend())
				fSoloPlots.erase(it_p);

			map<string, VarCut>::const_iterator it_c = fSoloCuts.find(*it);
			if (it_c != fSoloCuts.cend())
				fSoloCuts.erase(it_c);

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid solo variable: " << *it << endl;
      it = fSolos.erase(it);
		} else
      it++;
  }

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); ) {
		if (CheckVar(it->first) && CheckVar(it->second)
				&& fStatsTypes[it->first] == fStatsTypes[it->second]) {
			it++;
			continue;
		} 
			
    if (fStatsTypes[it->first] != fStatsTypes[it->second])
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t different statistical types for comparison in: " << it->first << " , " << it->second << endl;
    else 
			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid Comp variable: " << it->first << "\t" << it->second << endl;

		vector<pair<string, string>>::iterator it_p = find(fCompPlots.begin(), fCompPlots.end(), *it);
		if (it_p != fCompPlots.cend())
			fCompPlots.erase(it_p);

		map<pair<string, string>, VarCut>::const_iterator it_c = fCompCuts.find(*it);
		if (it_c != fCompCuts.cend())
			fCompCuts.erase(it_c);

		it = fComps.erase(it);
  }

  for (set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
			vector<pair<string, string>>::iterator it_p = find(fCorPlots.begin(), fCorPlots.end(), *it);
			if (it_p != fCorPlots.cend())
				fCorPlots.erase(it_p);

			map<pair<string, string>, VarCut>::const_iterator it_c = fCorCuts.find(*it);
			if (it_c != fCorCuts.cend())
				fCorCuts.erase(it_c);

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid Cor variable: " << it->first << "\t" << it->second << endl;
      it = fCors.erase(it);
		} else
      it++;
  }

  nSolos = fSolos.size();
  nComps = fComps.size();
  nCors  = fCors.size();

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSolos << " valid solo variables specified:\n";
  for(set<string>::const_iterator it=fSolos.cbegin(); it!=fSolos.cend(); it++) {
    cout << "\t" << *it << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nComps << " valid comparisons specified:\n";
  for(set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); it++) {
    cout << "\t" << it->first << " , " << it->second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSlopes << " valid slopes specified:\n";
  for(set<pair<string, string>>::const_iterator it=fSlopes.cbegin(); it!=fSlopes.cend(); it++) {
    cout << "\t" << it->first << " : " << it->second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nCors << " valid correlations specified:\n";
  for(set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); it++) {
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
  for (set<int>::const_iterator it_r=fRuns.cbegin(); it_r!=fRuns.cend(); it_r++) {
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

      for (set<string>::const_iterator it_v=fVars.cbegin(); it_v!=fVars.cend(); it_v++)
        tin->SetBranchAddress(it_v->c_str(), &(vars_buf[*it_v]));

      if (nSlopes > 0) {
        tin->SetBranchAddress("coeff", slopes_buf);
        tin->SetBranchAddress("err_coeff", slopes_err_buf);
      }

      const int nentries = tin->GetEntries();  // number of miniruns
      for(int n=0; n<nentries; n++) { // loop through the miniruns
        tin->GetEntry(n);

        fMiniruns.push_back(make_pair(run, minirun));
        for (set<string>::const_iterator it_v=fVars.cbegin(); it_v!=fVars.cend(); it_v++) {
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
          if (sign) {
            if (fSigns[run] == 0)
              vars_buf[*it_v].mean = 0;
            else 
              vars_buf[*it_v].mean *= (fSigns[run] > 0 ? 1 : -1);
          }
          fVarValues[*it_v].push_back(vars_buf[*it_v]);
        }
        for (set<pair<string, string>>::const_iterator it_s=fSlopes.cbegin(); it_s!=fSlopes.cend(); it_s++) {
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
  for (set<string>::const_iterator it=fSolos.cbegin(); it!=fSolos.cend(); it++) {
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
        if (find(fSoloPlots.cbegin(), fSoloPlots.cend(), *it) == fSoloPlots.cend())
          fSoloPlots.push_back(var);
        fSoloBadMiniruns[var].insert(fMiniruns[i]);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
  }

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); it++) {
    string var1 = it->first;
    string var2 = it->second;
    const double low_cut  = fCompCuts[*it].low;
    const double high_cut = fCompCuts[*it].high;
    for (int i=0; i<nMiniruns; i++) {
      double val1, val2;
      if (fStatsTypes[var1] == mean) {
        val1 = fVarValues[fVarNames[var1]][i].mean;
        val2 = fVarValues[fVarNames[var2]][i].mean;
      } else if (fStatsTypes[var1] == rms) {
        val1 = fVarValues[fVarNames[var1]][i].rms;
        val2 = fVarValues[fVarNames[var2]][i].rms;
      }

			double diff = abs(val1 - val2);
      if ( (low_cut  != 1024 && diff < low_cut)
        || (high_cut != 1024 && diff > high_cut)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Comp: " << var1 << " vs " << var2 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        if (find(fCompPlots.cbegin(), fCompPlots.cend(), *it) == fCompPlots.cend())
          fCompPlots.push_back(*it);
        fCompBadMiniruns[*it].insert(fMiniruns[i]);
      }
    }
  }

  for (set<pair<string, string>>::const_iterator it=fSlopes.cbegin(); it!=fSlopes.cend(); it++) {
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
        if (find(fSlopePlots.cbegin(), fSlopePlots.cend(), *it) == fSlopePlots.cend())
          fSlopePlots.push_back(*it);
        fSlopeBadMiniruns[*it].insert(fMiniruns[i]);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
  }

  for (set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); it++) {
    string yvar = it->first;
    string xvar = it->second;
    const double low_cut   = fCorCuts[*it].low;
    const double high_cut  = fCorCuts[*it].high;
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

			/*
      if () {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Cor: " << yvar << " vs " << xvar 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        if (find(fCorPlots.cbegin(), fCorPlots.cend(), *it) == fCorPlots.cend())
          fCorPlots.push_back(*it);
        fCorBadMiniruns[*it].insert(fMiniruns[i]);
      }
			*/
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

void TCheckStat::DrawSolos() {
  for (vector<string>::const_iterator it=fSoloPlots.cbegin(); it!=fSoloPlots.cend(); it++) {
    string var = *it;
    string var_name = fVarNames[var];
    StatsType vt = fStatsTypes[var];
    string unit = GetUnit(var);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

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

      int ipoint = g_flips[fSigns[fMiniruns[i].first]]->GetN();
      g_flips[fSigns[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val);
      g_flips[fSigns[fMiniruns[i].first]]->SetPointError(ipoint, 0, err);

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
    if (sign)
      g->SetTitle((var + " (sign corrected);;" + unit).c_str());
    else
      g->SetTitle((var + ";;" + unit).c_str());
    g_bold->SetMarkerStyle(21);
    g_bold->SetMarkerSize(1.3);
    g_bold->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerSize(1.2);
    g_bad->SetMarkerColor(kRed);
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]]->SetMarkerStyle(23-i);
      g_flips[flips[i]]->SetMarkerColor((4-i)*10+1);
      if (flips.size() > 1) {
        g_flips[flips[i]]->Fit("pol0");
        g_flips[flips[i]]->GetFunction("pol0")->SetLineColor((4-i)*10+1);
        g_flips[flips[i]]->GetFunction("pol0")->SetLineWidth(1);
      }
    }

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

    TLegend * l = new TLegend(0.1, 0.9-0.05*flips.size(), 0.25, 0.9);
    TPaveStats * st;
    map<int, TPaveStats *> sts;
    TPad * p1;
    TPad * p2;
    c->cd();
    if (vt == mean) {
      p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
      p1->SetBottomMargin(0);
      p1->SetRightMargin(0.05);
      p1->Draw();
      p1->SetGridy();

      p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
      p2->SetTopMargin(0);
      p2->SetBottomMargin(0.17);
      p2->SetRightMargin(0.05);
      p2->Draw();
      p2->SetGrid();
    } else if (vt == rms) {
      p1 = new TPad("p1", "p1", 0.0, 0.0, 1.0, 1.0);
      p1->SetBottomMargin(0.16);
      p1->SetRightMargin(0.05);
      p1->Draw();
      p1->SetGridy();
    }

    p1->cd();
    g->Draw("AP");
    p1->Update();
    st = (TPaveStats *) g->FindObject("stats");
    st->SetName("g_stats");
    double width = 0.7/(flips.size() + 1);
    st->SetX2NDC(0.95); 
    st->SetX1NDC(0.95-width); 
    st->SetY2NDC(0.9);
    st->SetY1NDC(0.8);

    g_bold->Draw("P same");
    g_bad->Draw("P same");

    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]]->Draw("P same");
      gPad->Update();
      if (flips.size() > 1) {
        sts[flips[i]] = (TPaveStats *) g_flips[flips[i]]->FindObject("stats");
        sts[flips[i]]->SetName(legends[flips[i]]);
        sts[flips[i]]->SetX2NDC(0.95-width*(i+1));
        sts[flips[i]]->SetX1NDC(0.95-width*(i+2));
        sts[flips[i]]->SetY2NDC(0.9);
        sts[flips[i]]->SetY1NDC(0.8);
        sts[flips[i]]->SetTextColor((4-i)*10+1);
      }
      l->AddEntry(g_flips[flips[i]], legends[flips[i]], "lep");
    }
    l->Draw();
    TAxis * ax = g->GetXaxis();
    TAxis * ay = g->GetYaxis();

    if (vt == mean) {
      g->GetXaxis()->SetLabelSize(0);
      g->GetXaxis()->SetNdivisions(-(nMiniruns+1));

      p2->cd();
      pull->SetFillColor(kGreen);
      pull->SetLineColor(kGreen);
      pull->Draw("AB");
      ax = pull->GetXaxis();
    }

    ax->SetNdivisions(-(nMiniruns+1));
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    // ax->SetLabelOffset(0.02);
    for (int i=0; i<=nMiniruns; i++) {
      ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    }

    double min = ay->GetXmin();
    double max = ay->GetXmax();
    ay->SetRangeUser(min, max+(max-min)/9);

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
  for (vector<pair<string, string>>::const_iterator it=fSlopePlots.cbegin(); it!=fSlopePlots.cend(); it++) {
    string unit = "ppb/nm";
    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double val, err;
      val = fSlopeValues[*it][i];
      err = fSlopeErrs[*it][i];
      g->SetPoint(i, i+1, val);
      g->SetPointError(i, 0, err);

      int ipoint = g_flips[fSigns[fMiniruns[i].first]]->GetN();
      g_flips[fSigns[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val);
      g_flips[fSigns[fMiniruns[i].first]]->SetPointError(ipoint, 0, err);
      
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
    if (sign)
      g->SetTitle((it->first + "_" + it->second + " (sign corrected);;" + unit).c_str());
    else 
      g->SetTitle((it->first + "_" + it->second + ";;" + unit).c_str());
    g_bold->SetMarkerStyle(20);
    g_bold->SetMarkerSize(1.3);
    g_bold->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerSize(1.2);
    g_bad->SetMarkerColor(kRed);

    g->Fit("pol0");
    TF1 * fit= g->GetFunction("pol0");
    double mean_value = fit->GetParameter(0);

    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]]->SetMarkerStyle(23-i);
      g_flips[flips[i]]->SetMarkerColor((4-i)*10+1);  // color: 41, 31, 21, 11
      if (flips.size() > 1) {
        g_flips[flips[i]]->Fit("pol0");
        g_flips[flips[i]]->GetFunction("pol0")->SetLineColor((4-i)*10+1);
        g_flips[flips[i]]->GetFunction("pol0")->SetLineWidth(1);
      }
    }

    TGraph * pull = new TGraph;
    for (int i=0; i<nMiniruns; i++) {
      double ratio = 0;
      if (fSlopeErrs[*it][i] != 0)
        ratio = (fSlopeValues[*it][i]-mean_value)/fSlopeErrs[*it][i];

      pull->SetPoint(i, i+1, ratio);
    }
    pull->GetXaxis()->SetRangeUser(0, nMiniruns+1);

    TLegend * l = new TLegend(0.1, 0.9-0.05*flips.size(), 0.25, 0.9);
    TPaveStats * st;
    map<int, TPaveStats *> sts;
    c->cd();
    TPad * p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
    p1->Draw();
    p1->SetGridy();
    p1->SetBottomMargin(0);
    p1->SetRightMargin(0.05);
    TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
    p2->Draw();
    p2->SetGrid();
    p2->SetTopMargin(0);
    p2->SetRightMargin(0.05);
    p2->SetBottomMargin(0.17);

    p1->cd();
    g->GetXaxis()->SetLabelSize(0);
    g->GetXaxis()->SetNdivisions(-(nMiniruns+1));
    g->Draw("AP");
    p1->Update();
    st = (TPaveStats *) g->FindObject("stats");
    st->SetName("g_stats");
    double width = 0.7/(flips.size() + 1);
    st->SetX2NDC(0.95);
    st->SetX1NDC(0.95-width);
    st->SetY2NDC(0.9);
    st->SetY1NDC(0.8);

    g_bold->Draw("P same");
    g_bad->Draw("P same");

    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]]->Draw("P same");
      p1->Update();
      if (flips.size() > 1) {
        sts[flips[i]] = (TPaveStats *) g_flips[flips[i]]->FindObject("stats");
        sts[flips[i]]->SetName(legends[flips[i]]);
        sts[flips[i]]->SetX2NDC(0.95-width*(i+1));
        sts[flips[i]]->SetX1NDC(0.95-width*(i+2));
        sts[flips[i]]->SetY2NDC(0.9);
        sts[flips[i]]->SetY1NDC(0.8);
        sts[flips[i]]->SetTextColor((4-i)*10+1);
      }
      l->AddEntry(g_flips[flips[i]], legends[flips[i]], "lep");
    }
    l->Draw();
    TAxis * ay = g->GetYaxis();
    double min = ay->GetXmin();
    double max = ay->GetXmax();
    ay->SetRangeUser(min, max+(max-min)/9);
    p1->Update();
    
    p2->cd();
    pull->SetFillColor(kGreen);
    pull->SetLineColor(kGreen);
    pull->Draw("AB");
    TAxis * ax = pull->GetXaxis();
    ax->SetNdivisions(-(nMiniruns+1));
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
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
  int MarkerStyles[] = {29, 33, 34, 31};
  for (vector<pair<string, string>>::const_iterator it=fCompPlots.cbegin(); it!=fCompPlots.cend(); it++) {
    string var1 = it->first;
    string var2 = it->second;
    string var_name1 = fVarNames[var1];
    string var_name2 = fVarNames[var2];
    string name1 = var_name1.substr(var_name1.find_last_of('_')+1);
    string name2 = var_name2.substr(var_name2.find_last_of('_')+1);

    StatsType vt = fStatsTypes[var1];
    string unit = GetUnit(var1);

    TGraphErrors * g1 = new TGraphErrors();
    TGraphErrors * g2 = new TGraphErrors();
    TGraphErrors * g_bold1 = new TGraphErrors();
    TGraphErrors * g_bold2 = new TGraphErrors();
    TGraphErrors * g_bad1  = new TGraphErrors();
    TGraphErrors * g_bad2  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips1;
    map<int, TGraphErrors *> g_flips2;
    vector<string> lnames1;
    vector<string> lnames2;
    for (int i=0; i<flips.size(); i++) {
      g_flips1[flips[i]] = new TGraphErrors();
      g_flips2[flips[i]] = new TGraphErrors();
      lnames1.push_back(legends[flips[i]] + ("--" + name1));
      lnames2.push_back(legends[flips[i]] + ("--" + name2));
      g_flips1[flips[i]]->SetName(lnames1[i].c_str());
      g_flips2[flips[i]]->SetName(lnames2[i].c_str());
    }
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
      int ipoint = g_flips1[fSigns[fMiniruns[i].first]]->GetN();
      g_flips1[fSigns[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val1);
      g_flips1[fSigns[fMiniruns[i].first]]->SetPointError(ipoint, 0, err1);
      g_flips2[fSigns[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val2);
      g_flips2[fSigns[fMiniruns[i].first]]->SetPointError(ipoint, 0, err2);
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

    if (sign)
      g1->SetTitle(Form("%s & %s (sign corrected);;%s", var1.c_str(), var2.c_str(), unit.c_str()));
    else 
      g1->SetTitle(Form("%s & %s;;%s", var1.c_str(), var2.c_str(), unit.c_str()));
    g_bold1->SetMarkerStyle(21);
    g_bold1->SetMarkerSize(1.3);
    g_bold1->SetMarkerColor(kBlue);
    g_bold2->SetMarkerStyle(21);
    g_bold2->SetMarkerSize(1.3);
    g_bold2->SetMarkerColor(kBlue);
    g_bad1->SetMarkerStyle(20);
    g_bad1->SetMarkerSize(1.2);
    g_bad1->SetMarkerColor(kRed);
    g_bad2->SetMarkerStyle(20);
    g_bad2->SetMarkerSize(1.2);
    g_bad2->SetMarkerColor(kRed);

    g1->Fit("pol0");
    g2->Fit("pol0");

    for (int i=0; i<flips.size(); i++) {
      g_flips1[flips[i]]->SetMarkerStyle(23-i);
      g_flips1[flips[i]]->SetMarkerColor((4-i)*10+1); 
      g_flips2[flips[i]]->SetMarkerStyle(MarkerStyles[i]);
      g_flips2[flips[i]]->SetMarkerColor((4-i)*10+8);  
      if (flips.size() > 1) {
        g_flips1[flips[i]]->Fit("pol0");
        g_flips1[flips[i]]->GetFunction("pol0")->SetLineColor((4-i)*10+1);
        g_flips1[flips[i]]->GetFunction("pol0")->SetLineWidth(1);
        g_flips2[flips[i]]->Fit("pol0");
        g_flips2[flips[i]]->GetFunction("pol0")->SetLineColor((4-i)*10+8);
        g_flips2[flips[i]]->GetFunction("pol0")->SetLineWidth(1);
      }
    }

    TLegend * l1 = new TLegend(0.1, 0.9-0.05*flips.size(), 0.25, 0.9);
    TLegend * l2 = new TLegend(0.1, 0.8-0.05*flips.size(), 0.25, 0.8);
    TPaveStats *st1, *st2;
    map<int, TPaveStats *> sts1, sts2;
    c->cd();
    TPad * p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
    p1->Draw();
    p1->SetGridy();
    p1->SetBottomMargin(0);
    p1->SetRightMargin(0.05);
    TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
    p2->Draw();
    p2->SetGrid();
    p2->SetTopMargin(0);
    p2->SetBottomMargin(0.17);
    p2->SetRightMargin(0.05);

    p1->cd();
    g1->GetXaxis()->SetLabelSize(0);
    g1->GetXaxis()->SetNdivisions(-(nMiniruns+1));
    g1->Draw("AP");
    p1->Update();
    st1 = (TPaveStats *) g1->FindObject("stats");
    st1->SetName("g1_stats");
    g2->Draw("P same");
    p1->Update();
    st2 = (TPaveStats *) g2->FindObject("stats");
    st2->SetName("g2_stats");
    g1->GetYaxis()->SetRangeUser(min, max+(max-min)/5);

    double width = 0.7/(flips.size() + 1);
    st1->SetX2NDC(0.95);
    st1->SetX1NDC(0.95-width);
    st1->SetY2NDC(0.9);
    st1->SetY1NDC(0.8);
    st2->SetX2NDC(0.95);
    st2->SetX1NDC(0.95-width);
    st2->SetY2NDC(0.8);
    st2->SetY1NDC(0.7);

    g_bold1->Draw("P same");
    g_bold2->Draw("P same");
    g_bad1->Draw("P same");
    g_bad2->Draw("P same");
    
    for (int i=0; i<flips.size(); i++) {
      g_flips1[flips[i]]->Draw("P same");
      g_flips2[flips[i]]->Draw("P same");
      p1->Update();
      if (flips.size() > 1) {
        sts1[flips[i]] = (TPaveStats *) g_flips1[flips[i]]->FindObject("stats");
        sts1[flips[i]]->SetName(lnames1[i].c_str());
        sts1[flips[i]]->SetX2NDC(0.95-width*(i+1));
        sts1[flips[i]]->SetX1NDC(0.95-width*(i+2));
        sts1[flips[i]]->SetY2NDC(0.9);
        sts1[flips[i]]->SetY1NDC(0.8);
        sts1[flips[i]]->SetTextColor((4-i)*10+1);

        sts2[flips[i]] = (TPaveStats *) g_flips2[flips[i]]->FindObject("stats");
        sts2[flips[i]]->SetName(lnames2[i].c_str());
        sts2[flips[i]]->SetX2NDC(0.95-width*(i+1));
        sts2[flips[i]]->SetX1NDC(0.95-width*(i+2));
        sts2[flips[i]]->SetY2NDC(0.8);
        sts2[flips[i]]->SetY1NDC(0.7);
        sts2[flips[i]]->SetTextColor((4-i)*10+8);
      }
      l1->AddEntry(g_flips1[flips[i]], lnames1[i].c_str(), "lep");
      l2->AddEntry(g_flips2[flips[i]], lnames2[i].c_str(), "lep");
    }
    l1->Draw();
    l2->Draw();

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
  for (vector<pair<string, string>>::const_iterator it=fCorPlots.cbegin(); it!=fCorPlots.cend(); it++) {
    string xvar = it->second;
    string yvar = it->first;
    string xvar_name = fVarNames[xvar];
    string yvar_name = fVarNames[yvar];
    StatsType xvt = fStatsTypes[xvar];
    StatsType yvt = fStatsTypes[yvar];
    string xunit = GetUnit(xvar);
    string yunit = GetUnit(yvar);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

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
      
      int ipoint = g_flips[fSigns[fMiniruns[i].first]]->GetN();
      g_flips[fSigns[fMiniruns[i].first]]->SetPoint(ipoint, xval, yval);
      g_flips[fSigns[fMiniruns[i].first]]->SetPointError(ipoint, xerr, yerr);

      if (fBoldRuns.find(fMiniruns[i].first) != fBoldRuns.cend()) {
        g_bold->SetPoint(ibold, xval, yval);
        g_bold->SetPointError(ibold, xerr, yerr);
        ibold++;
      }
      if (fCorBadMiniruns[*it].find(fMiniruns[i]) != fCorBadMiniruns[*it].cend()) {
        g_bad->SetPoint(ibad, xval, yval);
        g_bad->SetPointError(ibad, xerr, yerr);
        ibad++;
      }
    }
    // g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    if (sign)
      g->SetTitle((it->first + " vs " + it->second + " (sign corrected);" + xunit + ";" + yunit).c_str());
    else
      g->SetTitle((it->first + " vs " + it->second + ";" + xunit + ";" + yunit).c_str());
    g_bold->SetMarkerStyle(20);
    g_bold->SetMarkerSize(1.3);
    g_bold->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerSize(1.2);
    g_bad->SetMarkerColor(kRed);
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]]->SetMarkerStyle(23-i);
      g_flips[flips[i]]->SetMarkerColor((4-i)*10+1);
      if (flips.size() > 1) {
        g_flips[flips[i]]->Fit("pol1");
        g_flips[flips[i]]->GetFunction("pol1")->SetLineColor((4-i)*10+1);
        g_flips[flips[i]]->GetFunction("pol1")->SetLineWidth(1);
      }
    }

    g->Fit("pol1");

    TLegend * l = new TLegend(0.1, 0.9-0.05*flips.size(), 0.25, 0.9);
    TPaveStats * st;
    map<int, TPaveStats *> sts;
    c->cd();
    gPad->SetRightMargin(0.05);
    g->Draw("AP");
    gPad->Update();
    st = (TPaveStats *) g->FindObject("stats");
    st->SetName("g_stats");
    double width = 0.7/(flips.size() + 1);
    st->SetX2NDC(0.95); 
    st->SetX1NDC(0.95-width); 
    st->SetY2NDC(0.9);
    st->SetY1NDC(0.75);

    g_bold->Draw("P same");
    g_bad->Draw("P same");
    
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]]->Draw("P same");
      gPad->Update();
      if (flips.size() > 1) {
        sts[flips[i]] = (TPaveStats *) g_flips[flips[i]]->FindObject("stats");
        sts[flips[i]]->SetName(legends[flips[i]]);
        sts[flips[i]]->SetX2NDC(0.95-width*(i+1));
        sts[flips[i]]->SetX1NDC(0.95-width*(i+2));
        sts[flips[i]]->SetY2NDC(0.9);
        sts[flips[i]]->SetY1NDC(0.75);
        sts[flips[i]]->SetTextColor((4-i)*10+1);
      }
      l->AddEntry(g_flips[flips[i]], legends[flips[i]], "lep");
    }
    l->Draw();

    TAxis * ay = g->GetYaxis();
    double min = ay->GetXmin();
    double max = ay->GetXmax();
    ay->SetRangeUser(min, max+(max-min)/9);
    gPad->Update();

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s_vs_%s.png", out_name, xvar.c_str(), yvar.c_str()));
    c->Clear();
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Correlations.\n";
}

const char * TCheckStat::GetUnit (string var) {
  StatsType vt = fStatsTypes[var];
  if (var.find("asym") != string::npos) {
    if (vt == mean)
      return "ppb";
    else if (vt == rms)
      return "ppm";
  } else if (var.find("diff") != string::npos) {
    if (vt == mean)
      return "nm";
    else if (vt == rms)
      return "um";
  } else {
    return "";
  }
	return "";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
