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
#include "TLeaf.h"
#include "TCut.h"
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


enum Format {pdf, png};

map<int, const char *> legends = {
  {1,   "left in"},
  {-1,  "left out"},
  {2,   "right in"},
  {-2,  "right out"},
  {3,   "up in"},
  {-3,  "up out"},
  {4,   "down in"},
  {-4,  "down out"},
};

using namespace std;

class TCheckStat {

    // ClassDe (TCheckStat, 0) // check statistics

  private:
    TConfig fConf;
    Format format         = pdf;
    const char *out_name  = "check";
    const char *dir	      = "/chafs2/work1/apar/postpan-outputs/";
    string pattern        = "prexPrompt_xxxx_???_regress_postpan.root"; 
    const char *tree      = "mini";
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

    map<int, vector<string>> fRootFiles;
    map<int, int> fSigns;
    map<string, string> fUnits;
    map<pair<string, string>, pair<int, int>>	  fSlopeIndexes;
    // map<string, StatsType> fStatsTypes;
    // map<string, string> fVarNames;
    map<string, pair<string, string>> fVarNames;
    map<string, TLeaf *> fVarLeaves;
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
    map<string, vector<double>> fVarValues;
    map<pair<string, string>, vector<double>> fSlopeValues;
    map<pair<string, string>, vector<double>> fSlopeErrs;

    TCanvas * c;
  public:
     TCheckStat(const char*);
     ~TCheckStat();
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutFormat(const char * f);
     // void SetFileType(const char * f);
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

  // if (fConf.GetFileType())  SetFileType(fConf.GetFileType()); // must precede the following statements
  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetPattern())   pattern = fConf.GetPattern();
  if (fConf.GetTreeName())  tree  = fConf.GetTreeName();

  gROOT->SetBatch(1);
}

TCheckStat::~TCheckStat() {
  cerr << __PRETTY_FUNCTION__ << ":INFO\t Release TCheckStat\n";
}

/*
void TCheckStat::SetFileType(const char * ftype) {
  if (ftype == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no filetype specified" << endl;
    exit(10);
  }

  char * type = Sub(ftype, 0);
  StripSpaces(type);
  if (strcmp(type, "postpan") == 0) {
    filetype = "postpan";
    ft = postpan;
    dir	= "/chafs2/work1/apar/postpan-outputs";
    pattern = "prexPrompt_xxxx_???_regress_postpan.root";
    tree = "mini"; // default tree
  } else if (strcmp(type, "dithering") == 0) {
    filetype = "dithering";
    ft = dithering;
    dir = "/adaqfs/home/apar/weibin/check/dithering/";  // default dir.
    pattern = "prexPrompt_dit_agg_xxxx.root";
    tree = "mini";
  } else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t unknown root file type: " << type << ".\t Allowed type: " << endl
         << "\t postpan" << endl
         << "\t dithering" << endl;
    exit(10);
  }
}
*/

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
  for(int run : runs) {
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
  for(int run : runs) {
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
  for(int slug : slugs) {
    if (   (CREX_AT_START_SLUG <= slug && slug <= CREX_AT_END_SLUG)
        || (PREX_AT_START_SLUG <= slug && slug <= PREX_AT_END_SLUG)
        || (        START_SLUG <= slug && slug <= END_SLUG) ) { 
      fSlugs.insert(slug);
    } else {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid slug number (" << START_SLUG << "-" << END_SLUG << "): " << slug << endl;
      continue;
    }
  }
  nSlugs = fSlugs.size();
}

void TCheckStat::CheckRuns() {
  StartConnection();

  if (nSlugs > 0) {
    set<int> runs;
    for(int slug : fSlugs) {
      runs = GetRunsFromSlug(slug);
      for (int run : runs) {
        fRuns.insert(run);
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

  for (set<int>::const_iterator it = fRuns.cbegin(); it != fRuns.cend(); ) {
    int run = *it;
    string p_buf(pattern);
    p_buf.replace(p_buf.find("xxxx"), 4, to_string(run));
    const char * p = Form("%s/%s", dir, p_buf.c_str());
    glob_t globbuf;
    glob(p, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << __PRETTY_FUNCTION__ << ":WARNING\t no root file for run " << run << ". Ignore it.\n";
      it = fRuns.erase(it);
      continue;
    }
    for (int i=0; i<globbuf.gl_pathc; i++)
      fRootFiles[run].push_back(globbuf.gl_pathv[i]);

    globfree(&globbuf);
    it++;
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
    if (find(flips.cbegin(), flips.cend(), it->second) == flips.cend())
      flips.push_back(it->second);
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
    const char * file_name = fRootFiles[run][0].c_str();
    TFile * f_rootfile = new TFile(file_name, "read");
    if (f_rootfile->IsOpen()) {
      TTree * tin = (TTree*) f_rootfile->Get(tree); // receive minitree
      vector<TString> * l_iv = (vector<TString>*) f_rootfile->Get("IVNames");
      vector<TString> * l_dv = (vector<TString>*) f_rootfile->Get("DVNames");
      bool sflag = true;
      if (nSlopes > 0)
        sflag = (l_iv != NULL && l_dv != NULL);

      if (tin != NULL && sflag) {
        cout << __PRETTY_FUNCTION__ << ":INFO\t use file to check vars: " << file_name << endl;

        TObjArray * l_var = tin->GetListOfBranches();
        for (string var : fVars) {
          size_t dot_pos = var.find_first_of('.');
          string branch = (dot_pos == string::npos) ? var : var.substr(0, dot_pos);
          string leaf = (dot_pos == string::npos) ? "" : var.substr(dot_pos+1);

          TBranch * bbuf = (TBranch *) l_var->FindObject(branch.c_str());
          if (!bbuf) {
            cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such branch: " << branch << " in var: " << var << endl;
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid branches:\n";
            TIter next(l_var);
            TBranch *br;
            while (br = (TBranch*) next()) {
              cout << "\t" << br->GetName() << endl;
            }
            // FIXME: clear up before exit
            exit(24);
          }

          TObjArray * l_leaf = bbuf->GetListOfLeaves();
          if (leaf.size() == 0) {
            leaf = l_leaf->At(0)->GetName();  // use the first leaf
          }
          TLeaf * lbuf = (TLeaf *) l_leaf->FindObject(leaf.c_str());
          if (!lbuf) {
            cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such leaf: " << leaf << " in var: " << var << endl;
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid leaves:" << endl;
            TIter next(l_leaf);
            TLeaf *l;
            while (l = (TLeaf*) next()) {
              cout << "\t" << l->GetName() << endl;
            }
            // FIXME: clear up before exit
            exit(24);
          }

          fVarNames[var] = make_pair(branch, leaf);

          if (leaf == "mean") {
            string err_var = branch + ".err";
            lbuf = (TLeaf *) l_leaf->FindObject("err");
            if (lbuf) {
              fVars.insert(err_var);
              fVarNames[err_var] = make_pair(branch, "err");
            } else {
              cerr << __PRETTY_FUNCTION__ << ":WARNING\t No err leaf for mean var: " << var << endl;
            }
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
          for (pair<string, string> slope : fSlopes) {
            string dv = slope.first;
            string iv = slope.second;
            vector<TString>::const_iterator it_dv = find(l_dv->cbegin(), l_dv->cend(), dv);
            vector<TString>::const_iterator it_iv = find(l_iv->cbegin(), l_iv->cend(), iv);

            if (it_dv == l_dv->cend()) {
              cerr << __PRETTY_FUNCTION__ << ":FATAL\t Invalid dv name for slope: " << dv << endl;
              error_dv_flag = true;
              break;
            } 
            if (it_iv == l_iv->cend()) {
              cerr << __PRETTY_FUNCTION__ << ":FATAL\t Invalid iv name for slope: " << iv << endl;
              error_iv_flag = true;
              break;
            }

            fSlopeIndexes[slope] = make_pair(it_dv-l_dv->cbegin(), it_iv-l_iv->cbegin());
          }
          if (error_dv_flag) {
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid dv names:\n";
            for (TString dv : (*l_dv)) 
              cout << "\t" << dv.Data() << endl;

            exit(24);
          }
          if (error_iv_flag) {
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid dv names:\n";
            for (TString iv : (*l_iv)) 
              cout << "\t" << iv.Data() << endl;

            exit(24);
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
				&& fVarNames[it->first].second == fVarNames[it->second].second) {
			it++;
			continue;
		} 
			
    if (fVarNames[it->first].second != fVarNames[it->second].second)
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

  // cout << __PRETTY_FUNCTION__ << ":DEBUG\t " << " internal variables:\n";
  // for (string var : fVars) {
  //   cout << "\t" << var << endl;
  // }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSolos << " valid solo variables specified:\n";
  for(string solo : fSolos) {
    cout << "\t" << solo << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nComps << " valid comparisons specified:\n";
  for(pair<string, string> comp : fComps) {
    cout << "\t" << comp.first << " , " << comp.second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSlopes << " valid slopes specified:\n";
  for(pair<string, string> slope : fSlopes) {
    cout << "\t" << slope.first << " vs " << slope.second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nCors << " valid correlations specified:\n";
  for(pair<string, string> cor : fCors) {
    cout << "\t" << cor.first << " : " << cor.second << endl;
  }
}

bool TCheckStat::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t Unknown variable in: " << var << endl;
    return false;
  }

  return true;
}

void TCheckStat::GetValues() {
  for (int run : fRuns) {
    const size_t sessions = fRootFiles[run].size();
    for (size_t session=0; session<sessions; session++) {
      const char * file_name = fRootFiles[run][session].c_str();
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d\t", run, session)
           << file_name << endl;
      TTree * tin = (TTree*) f_rootfile.Get(tree); // receive minitree
      if (! tin) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such tree: " << tree << " in root file: " << file_name << endl;
        continue;
      }

      bool error = false;
      // minirun
      vector<const char *> mini_names = {"minirun", "mini", "miniruns"};
      TBranch * b_minirun = NULL;
      for (const char * mini_name : mini_names) {
        if (!b_minirun)
          b_minirun = tin->GetBranch(mini_name);
        if (b_minirun) 
          break;
      }
      if (!b_minirun) {
        cerr << __PRETTY_FUNCTION__ << ":ERROR\t no minirun branch in tree: " << tree 
          << " of file: " << file_name << endl;
        continue;
      }
      TLeaf *l_minirun = (TLeaf *)b_minirun->GetListOfLeaves()->At(0);

      for (string var : fVars) {
        string branch = fVarNames[var].first;
        string leaf   = fVarNames[var].second;
        TBranch * br = tin->GetBranch(branch.c_str());
        if (!br) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t no branch: " << branch << " in tree: " << tree
            << " of file: " << file_name << endl;
          error = true;
          break;
        }
        TLeaf * l = br->GetLeaf(leaf.c_str());
        if (!l) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t no leaf: " << leaf << " in branch: " << branch 
            << " in tree: " << tree << " of file: " << file_name << endl;
          error = true;
          break;
        }
        fVarLeaves[var] = l;
      }

      if (error)
        continue;

      if (nSlopes > 0) {
        tin->SetBranchAddress("coeff", slopes_buf);
        tin->SetBranchAddress("err_coeff", slopes_err_buf);
      }

      const int nentries = tin->GetEntries();  // number of miniruns
      for(int n=0; n<nentries; n++) { // loop through the miniruns
        tin->GetEntry(n);

        l_minirun->GetBranch()->GetEntry(n);
        fMiniruns.push_back(make_pair(run, l_minirun->GetValue()));
        for (string var : fVars) {
          double unit = 1;
          double value;
          string leaf = fVarNames[var].second;
          if (var.find("asym") != string::npos) {
            if (leaf == "mean" || leaf == "err")
              unit = ppb;
            else if (leaf == "rms")
              unit = ppm;
          }
          else if (var.find("diff")) {
            if (leaf == "mean" || leaf == "err")
              unit = um/mm;
            else if (leaf == "rms")
              unit = mm/mm;
          }

          fVarLeaves[var]->GetBranch()->GetEntry(n);
          value = fVarLeaves[var]->GetValue() / unit;
          if (sign) {
            if (fVarNames[var].second == "mean") {
              if (fSigns[run] == 0)
                value = 0;
              else 
                value *= (fSigns[run] > 0 ? 1 : -1);
            }
          }
          fVarValues[var].push_back(value);
        }
        for (pair<string, string> slope : fSlopes) {
          double unit = ppm/(um/mm);
          fSlopeValues[slope].push_back(slopes_buf[fSlopeIndexes[slope].first*cols+fSlopeIndexes[slope].second]/unit);
          fSlopeErrs[slope].push_back(slopes_err_buf[fSlopeIndexes[slope].first*cols+fSlopeIndexes[slope].second]/unit);
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
  for (string solo : fSolos) {
    const double low_cut  = fSoloCuts[solo].low;
    const double high_cut = fSoloCuts[solo].high;
    const double stat_cut = fSoloCuts[solo].stability;
    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean, sigma = 0;
    for (int i=0; i<nMiniruns; i++) {
      double val = fVarValues[solo][i];

      if (i == 0) {
        mean = val;
        sigma = 0;
      }

      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (stat_cut != 1024 && abs(val-mean) > stat_cut*sigma)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in " << solo
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        if (find(fSoloPlots.cbegin(), fSoloPlots.cend(), solo) == fSoloPlots.cend())
          fSoloPlots.push_back(solo);
        fSoloBadMiniruns[solo].insert(fMiniruns[i]);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
  }

  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
    const double low_cut  = fCompCuts[comp].low;
    const double high_cut = fCompCuts[comp].high;
    for (int i=0; i<nMiniruns; i++) {
      double val1 = fVarValues[var1][i];
      double val2 = fVarValues[var1][i];
			double diff = abs(val1 - val2);

      if ( (low_cut  != 1024 && diff < low_cut)
        || (high_cut != 1024 && diff > high_cut)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Comp: " << var1 << " vs " << var2 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        if (find(fCompPlots.cbegin(), fCompPlots.cend(), comp) == fCompPlots.cend())
          fCompPlots.push_back(comp);
        fCompBadMiniruns[comp].insert(fMiniruns[i]);
      }
    }
  }

  for (pair<string, string> slope : fSlopes) {
    // string dv = slope.first;
    // string iv = slope.second;
    const double low_cut  = fSlopeCuts[slope].low;
    const double high_cut = fSlopeCuts[slope].high;
    const double stat_cut = fSlopeCuts[slope].stability;

    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean = fSlopeValues[slope][0];
    double sigma = fSlopeErrs[slope][0];
    for (int i=0; i<nMiniruns; i++) {
      double val = fSlopeValues[slope][i];
      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (stat_cut != 1024 && abs(val-mean) > stat_cut*sigma)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in slope: " << slope.first << " vs " << slope.second 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << endl;
        if (find(fSlopePlots.cbegin(), fSlopePlots.cend(), slope) == fSlopePlots.cend())
          fSlopePlots.push_back(slope);
        fSlopeBadMiniruns[slope].insert(fMiniruns[i]);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
  }

  for (pair<string, string> cor : fCors) {
    string yvar = cor.first;
    string xvar = cor.second;
    const double low_cut   = fCorCuts[cor].low;
    const double high_cut  = fCorCuts[cor].high;
    // const double 
    for (int i=0; i<nMiniruns; i++) {
      double xval = fVarValues[xvar][i];
      double yval = fVarValues[yvar][i];

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
  for (string solo : fSoloPlots) {
    string unit = GetUnit(solo);
    string err_var;
    bool mean = (fVarNames[solo].second == "mean");
    if (mean)
      err_var = fVarNames[solo].first + ".err";

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double val, err=0;
      val = fVarValues[solo][i];
      if (mean) {
        err = fVarValues[err_var][i];
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
      if (fSoloBadMiniruns[solo].find(fMiniruns[i]) != fSoloBadMiniruns[solo].cend()) {
        g_bad->SetPoint(ibad, i+1, val);
        g_bad->SetPointError(ibad, 0, err);
        ibad++;
      }
    }
    g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    if (sign)
      g->SetTitle((solo + " (sign corrected);;" + unit).c_str());
    else
      g->SetTitle((solo + ";;" + unit).c_str());
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
    if (mean) {
      pull = new TGraph();

      for (int i=0; i<nMiniruns; i++) {
        double ratio = 0;
        if (fVarValues[err_var][i]!= 0)
          ratio = (fVarValues[solo][i]-mean_value)/fVarValues[err_var][i];

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
    if (mean) {
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
    } else {
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

    if (mean) {
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
      c->Print(Form("%s_%s.png", out_name, solo.c_str()));

    c->Clear();
    if (pull) {
      pull->Delete();
      pull = NULL;
    }
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Solos.\n";
}

void TCheckStat::DrawSlopes() {
  for (pair<string, string> slope : fSlopePlots) {
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
      val = fSlopeValues[slope][i];
      err = fSlopeErrs[slope][i];
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
      if (fSlopeBadMiniruns[slope].find(fMiniruns[i]) != fSlopeBadMiniruns[slope].cend()) {
        g_bad->SetPoint(ibad, i+1, val);
        g_bad->SetPointError(ibad, 0, err);
        ibad++;
      }
    }
    g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    if (sign)
      g->SetTitle((slope.first + "_" + slope.second + " (sign corrected);;" + unit).c_str());
    else 
      g->SetTitle((slope.first + "_" + slope.second + ";;" + unit).c_str());
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
      if (fSlopeErrs[slope][i] != 0)
        ratio = (fSlopeValues[slope][i]-mean_value)/fSlopeErrs[slope][i];

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
      c->Print(Form("%s_%s_%s.png", out_name, slope.first.c_str(), slope.second.c_str()));
    c->Clear();
    pull->Delete();
    pull = NULL;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Slopes.\n";
}

void TCheckStat::DrawComps() {
  int MarkerStyles[] = {29, 33, 34, 31};
  for (pair<string, string> comp : fCompPlots) {
    string var1 = comp.first;
    string var2 = comp.second;
    string branch1 = fVarNames[var1].first;
    string branch2 = fVarNames[var2].first;
    string name1 = branch1.substr(branch1.find_last_of('_')+1);
    string name2 = branch2.substr(branch2.find_last_of('_')+1);

    const char * err_var1 = Form("%s.err", branch1.c_str());
    const char * err_var2 = Form("%s.err", branch2.c_str());
    bool mean = (fVarNames[var1].second == "mean");

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
      lnames1.push_back(Form("%s--%s", legends[flips[i]], name1.c_str()));
      lnames2.push_back(Form("%s--%s", legends[flips[i]], name2.c_str()));
      g_flips1[flips[i]]->SetName(lnames1[i].c_str());
      g_flips2[flips[i]]->SetName(lnames2[i].c_str());
    }
    TH1F * h_diff = new TH1F("diff", "", nMiniruns, 0, nMiniruns);

    double min, max;
    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double val1, err1=0;
      double val2, err2=0;
      val1 = fVarValues[var1][i];
      val2 = fVarValues[var2][i];
      if (mean) {
        err1 = fVarValues[err_var1][i];
        err2 = fVarValues[err_var2][i];
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
      if (fCompBadMiniruns[comp].find(fMiniruns[i]) != fCompBadMiniruns[comp].cend()) {
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
  for (pair<string, string> cor : fCorPlots) {
    string xvar = cor.second;
    string yvar = cor.first;
    string xbranch = fVarNames[xvar].first;
    string ybranch = fVarNames[yvar].first;
    string xunit = GetUnit(xvar);
    string yunit = GetUnit(yvar);

    bool xmean = (fVarNames[xvar].second == "mean");
    bool ymean = (fVarNames[yvar].second == "mean");
    const char * xerr_var = Form("%s.err", xbranch.c_str());
    const char * yerr_var = Form("%s.err", ybranch.c_str());

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double xval, xerr=0;
      double yval, yerr=0;
      xval = fVarValues[xvar][i];
      yval = fVarValues[yvar][i];
      if (xmean) {
        xerr = fVarValues[xerr_var][i];
      }
      if (ymean) {
        yerr = fVarValues[yerr_var][i];
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
      if (fCorBadMiniruns[cor].find(fMiniruns[i]) != fCorBadMiniruns[cor].cend()) {
        g_bad->SetPoint(ibad, xval, yval);
        g_bad->SetPointError(ibad, xerr, yerr);
        ibad++;
      }
    }
    // g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    if (sign)
      g->SetTitle((cor.first + " vs " + cor.second + " (sign corrected);" + xunit + ";" + yunit).c_str());
    else
      g->SetTitle((cor.first + " vs " + cor.second + ";" + xunit + ";" + yunit).c_str());
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
  string branch = fVarNames[var].first;
  string leaf   = fVarNames[var].second;
  if (branch.find("asym") != string::npos) {
    if (leaf == "mean")
      return "ppb";
    else if (leaf == "rms")
      return "ppm";
  } else if (branch.find("diff") != string::npos) {
    if (leaf == "mean")
      return "nm";
    else if (leaf == "rms")
      return "um";
  } else {
    return "";
  }
	return "";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
