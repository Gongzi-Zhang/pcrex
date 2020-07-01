#ifndef TMULPLOT_H
#define TMULPLOT_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <glob.h>
#include <libgen.h>

#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TCut.h"
#include "TEntryList.h"
#include "TCollection.h"
#include "TGraphErrors.h"
#include "TH1F.h"
#include "TPaveStats.h"
#include "TList.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TColor.h"

#include "const.h"
#include "line.h"
#include "rcdb.h"
#include "TConfig.h"


enum Format {pdf, png};

using namespace std;

class TMulPlot {

    // ClassDe (TMulPlot, 0) // mul plots

  private:
    Format format     = pdf; // default pdf output
    const char *out_name = "mulplot";
    // root file: $dir/$pattern
    const char *dir   = "/adaqfs/home/apar/PREX/prompt/results/";
    string pattern    = "prexPrompt_xxxx_???_regress_postpan.root";
    const char *tree  = "reg";
    TCut cut          = "ok_cut";
    bool logy         = false;
    vector<pair<long, long>> ecuts;

    TConfig fConf;
    int     nSlugs;
    int	    nRuns;
    int	    nVars;
    int	    nSolos;
		int		  nCustoms;
    int	    nComps;
    int	    nSlopes;
    int	    nCors;

    set<int>    fRuns;
    set<int>    fSlugs;
    set<string> fVars;

    set<string>         fSolos;
    map<string, VarCut> fSoloCuts;	// use the low and high cut as x range

    set<string>					fCustoms;
    map<string, VarCut>	fCustomCuts;
		map<string, Node *>	fCustomDefs;

    set<pair<string, string>>         fComps;
    map<pair<string, string>, VarCut>	fCompCuts;

    set<pair<string, string>>         fSlopes;
    map<pair<string, string>, VarCut> fSlopeCuts;

    set<pair<string, string>>         fCors;
    map<pair<string, string>, VarCut> fCorCuts;

    map<int, vector<string>> fRootFiles;
    map<int, int> fSigns;
    map<pair<string, string>, pair<int, int>>	  fSlopeIndexes;
    map<string, pair<string, string>> fVarNames;
    map<string, TLeaf *> fVarLeaves;
    map<string, double>  vars_buf;
    // double slopes_buf[ROWS][COLS];
    // double slopes_err_buf[ROWS][COLS];
    int      rows, cols;
    double * slopes_buf;
    double * slopes_err_buf;

    map<string, TH1F *>    fSoloHists;
    map<pair<string, string>, pair<TH1F *, TH1F *>>    fCompHists;
    // map<pair<string, string>, TH1F *>    fSlopeHists;
    // map<pair<string, string>, TH1F *>    fCorHists;

  public:
     TMulPlot(const char* confif_file, const char* run_list = NULL);
     ~TMulPlot();
     void Draw();
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutFormat(const char * f);
     void SetDir(const char * d);
     void SetLogy(bool log) {logy = log;}
     void SetSlugs(set<int> slugs);
     void SetRuns(set<int> runs);
     void CheckRuns();
     void CheckVars();
     bool CheckVar(string var);
     bool CheckCustomVar(Node * node);
     void GetValues();
     bool CheckEntryCut(const long entry);
		 double get_custom_value(Node * node);
     void DrawHistogram();
};

// ClassImp(TMulPlot);

TMulPlot::TMulPlot(const char* config_file, const char* run_list) :
  fConf(config_file, run_list)
{
  fConf.ParseConfFile();
  fRuns = fConf.GetRuns();
  nRuns = fRuns.size();
  fVars	= fConf.GetVars();
  nVars = fVars.size();

  fSolos    = fConf.GetSolos();
  fSoloCuts = fConf.GetSoloCuts();
  nSolos    = fSolos.size();

  fCustoms    = fConf.GetCustoms();
  fCustomDefs = fConf.GetCustomDefs();
  fCustomCuts = fConf.GetCustomCuts();
  nCustoms    = fCustoms.size();

  fComps    = fConf.GetComps();
  fCompCuts = fConf.GetCompCuts();
  nComps    = fComps.size();

  fSlopes = fConf.GetSlopes();
  nSlopes = fSlopes.size();
  fSlopeCuts  = fConf.GetSlopeCuts();

  fCors	    = fConf.GetCors();
  fCorCuts  = fConf.GetCorCuts();

  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetPattern())   pattern = fConf.GetPattern();
  if (fConf.GetTreeName())  tree    = fConf.GetTreeName();
  if (fConf.GetTreeCut())   cut     = fConf.GetTreeCut();
  logy  = fConf.GetLogy();
  ecuts = fConf.GetEntryCuts();

  gROOT->SetBatch(1);
}

TMulPlot::~TMulPlot() {
  cout << INFO << "End of TMulPlot" << ENDL;
}

void TMulPlot::Draw() {
  cout << INFO << "draw mul plots of" << endl
       << "\tin directory: " << dir << endl
       << "\tfrom files: " << pattern << endl
       << "\tuse tree: " << tree << ENDL;

  CheckRuns();
  CheckVars();
  GetValues();
  DrawHistogram();
}

void TMulPlot::SetDir(const char * d) {
  struct stat info;
  if (stat(d, &info) != 0) {
    cerr << FATAL << "can't access specified dir: " << dir << ENDL;
    exit(30);
  } else if ( !(info.st_mode & S_IFDIR)) {
    cerr << FATAL << "not a dir: " << dir << ENDL;
    exit(31);
  }
  dir = d;
}

void TMulPlot::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << FATAL << "Unknow output format: " << f << ENDL;
    exit(40);
  }
}

void TMulPlot::SetSlugs(set<int> slugs) {
  for(int slug : slugs) {
    if (   (CREX_AT_START_SLUG <= slug && slug <= CREX_AT_END_SLUG)
        || (PREX_AT_START_SLUG <= slug && slug <= PREX_AT_END_SLUG)
        || (        START_SLUG <= slug && slug <= END_SLUG) ) { 
      fSlugs.insert(slug);
    } else {
      cerr << ERROR << "Invalid slug number (" << START_SLUG << "-" << END_SLUG << "): " << slug << ENDL;
      continue;
    }
  }
  nSlugs = fSlugs.size();
}

void TMulPlot::SetRuns(set<int> runs) {
  for(int run : runs) {
    if (run < START_RUN || run > END_RUN) {
      cerr << ERROR << "Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << ENDL;
      continue;
    }
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
}

void TMulPlot::CheckRuns() {
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
  GetValidRuns(fRuns);
  nRuns = fRuns.size();

  for (set<int>::const_iterator it = fRuns.cbegin(); it != fRuns.cend(); ) {
    int run = *it;
    string p_buf(pattern);
    p_buf.replace(p_buf.find("xxxx"), 4, to_string(run));
    const char * p = Form("%s/%s", dir, p_buf.c_str());

    glob_t globbuf;
    glob(p, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << WARNING << "no root file for run " << run << ". Ignore it." << ENDL;
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
    cerr << FATAL << "No valid runs specified!" << ENDL;
    EndConnection();
    exit(10);
  }

  cout << INFO << "" << nRuns << " valid runs specified:" << ENDL;
  for(int run : fRuns) {
    cout << "\t" << run << endl;
  }

  fSigns = GetSign(fRuns); // sign corrected
  EndConnection();
}

void TMulPlot::CheckVars() {
  srand(time(NULL));
  int s = rand() % nRuns;
  set<int>::const_iterator it_r=fRuns.cbegin();
  for(int i=0; i<s; i++)
    it_r++;

  while (it_r != fRuns.cend()) {
    int run = *it_r;
    const char *file_name = fRootFiles[run][0].c_str();
    TFile * f_rootfile = new TFile(file_name, "read");
    if (f_rootfile->IsOpen()) {
      if (!f_rootfile->GetListOfKeys()->Contains(tree)) {
        cerr << FATAL << "no tree: " << tree << " in root file: " << file_name;
        exit(22);
      }
      TTree * tin = (TTree*) f_rootfile->Get(tree);
      if (tin != NULL) {
        cout << INFO << "use file to check vars: " << file_name << ENDL;

        TObjArray * l_var = tin->GetListOfBranches();
        for (string var : fVars) {
          size_t dot_pos = var.find_first_of('.');
          string branch = (dot_pos == string::npos) ? var : var.substr(0, dot_pos);
          string leaf = (dot_pos == string::npos) ? "" : var.substr(dot_pos+1);

          TBranch * bbuf = (TBranch *) l_var->FindObject(branch.c_str());
          if (!bbuf) {
            cerr << WARNING << "No such branch: " << branch << " in var: " << var << ENDL;
            cout << DEBUG << "List of valid branches:" << ENDL;
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
            cerr << WARNING << "No such leaf: " << leaf << " in var: " << var << ENDL;
            cout << DEBUG << "List of valid leaves:" << ENDL;
            TIter next(l_leaf);
            TLeaf *l;
            while (l = (TLeaf*) next()) {
              cout << "\t" << l->GetName() << endl;
            }
            // FIXME: clear up before exit
            exit(24);
          }

          fVarNames[var] = make_pair(branch, leaf);
        }

        /*  FIXME: no slope now
        if (nSlopes>0) {
          rows = l_dv->size();
          cols = l_iv->size();
          slopes_buf = new double[rows*cols];
          slopes_err_buf = new double[rows*cols];
          // if (rows != ROWS || cols != COLS) {
          //   cerr << FATAL << "Unmatched slope array size: " << rows << "x" << cols << " in run: " << run << ENDL;
          //   exit(20);
          // }
          bool error_dv_flag = false;
          bool error_iv_flag = false;
          for (set<pair<string, string>>::const_iterator it=fSlopes.cbegin(); it != fSlopes.cend(); ) {
            string dv = it->first;
            string iv = it->second;
            vector<TString>::const_iterator it_dv = find(l_dv->cbegin(), l_dv->cend(), dv);
            vector<TString>::const_iterator it_iv = find(l_iv->cbegin(), l_iv->cend(), iv);
            if (it_dv == l_dv->cend()) {
              cerr << WARNING << "Invalid dv name for slope: " << dv << ENDL;
              it = fSlopes.erase(it);
              error_dv_flag = true;
              continue;
            }
            if (it_iv == l_iv->cend()) {
              cerr << WARNING << "Invalid iv name for slope: " << iv << ENDL;
              it = fSlopes.erase(it);
              error_iv_flag = true;
              continue;
            }
            fSlopeIndexes[*it] = make_pair(it_dv-l_dv->cbegin(), it_iv-l_iv->cbegin());
            it++;
          }
          if (error_dv_flag) {
            cout << DEBUG << "List of valid dv names:" << ENDL;
            for (vector<TString>::const_iterator it = l_dv->cbegin(); it != l_dv->cend(); it++) 
              cout << "\t" << (*it).Data() << endl;
          }
          if (error_iv_flag) {
            cout << DEBUG << "List of valid dv names:" << ENDL;
            for (vector<TString>::const_iterator it = l_iv->cbegin(); it != l_iv->cend(); it++) 
              cout << "\t" << (*it).Data() << endl;
          }
        }
        */
        tin->Delete();
        f_rootfile->Close();
        break;
      }
    } 
      
    cerr << WARNING << "root file of run: " << run << " is broken, ignore it." << ENDL;
    it_r = fRuns.erase(it_r);

    if (it_r == fRuns.cend())
      it_r = fRuns.cbegin();
  }

  nRuns = fRuns.size();
  if (nRuns == 0) {
    cerr << FATAL << "no valid runs, aborting." << ENDL;
    exit(10);
  }

  nVars = fVars.size();
  nSlopes = fSlopes.size();
  if (nVars == 0 && nSlopes == 0) {
    cerr << FATAL << "no valid variables specified, aborting." << ENDL;
    exit(11);
  }

  for (set<string>::const_iterator it=fSolos.cbegin(); it!=fSolos.cend();) {
    if (fVars.find(*it) == fVars.cend()) {
			map<string, VarCut>::const_iterator it_c = fSoloCuts.find(*it);
			if (it_c != fSoloCuts.cend())
				fSoloCuts.erase(it_c);

			cerr << WARNING << "Invalid solo variable: " << *it << ENDL;
      it = fSolos.erase(it);
		} else
      it++;
  }

	for (set<string>::const_iterator it=fCustoms.cbegin(); it!=fCustoms.cend(); ) {
		if (!CheckCustomVar(fCustomDefs[*it])) {
			map<string, VarCut>::const_iterator it_c = fCustomCuts.find(*it);
			if (it_c != fCustomCuts.cend())
				fCustomCuts.erase(it_c);

			cerr << WARNING << "Invalid custom variable: " << *it << ENDL;
      it = fCustoms.erase(it);
		} else
      it++;
	}

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
				// || strcmp(GetUnit(it->first), GetUnit(it->second)) != 0 ) {
			map<pair<string, string>, VarCut>::const_iterator it_c = fCompCuts.find(*it);
			if (it_c != fCompCuts.cend())
				fCompCuts.erase(it_c);

			cerr << WARNING << "Invalid Comp variable: " << it->first << "\t" << it->second << ENDL;
      it = fComps.erase(it);
		} else 
			it++;
  }

  for (set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
			map<pair<string, string>, VarCut>::const_iterator it_c = fCorCuts.find(*it);
			if (it_c != fCorCuts.cend())
				fCorCuts.erase(it_c);

			cerr << WARNING << "Invalid Cor variable: " << it->first << "\t" << it->second << ENDL;
      it = fCors.erase(it);
		} else
      it++;
  }

  nSolos = fSolos.size();
  nComps = fComps.size();
  nCors  = fCors.size();
  nCustoms = fCustoms.size();

  cout << INFO << "" << nSolos << " valid solo variables specified:" << ENDL;
  for(string solo : fSolos) {
    cout << "\t" << solo << endl;
  }
  cout << INFO << "" << nComps << " valid comparisons specified:" << ENDL;
  for(pair<string, string> comp : fComps) {
    cout << "\t" << comp.first << " , " << comp.second << endl;
  }
  cout << INFO << "" << nSlopes << " valid slopes specified:" << ENDL;
  for(pair<string, string> slope : fSlopes) {
    cout << "\t" << slope.first << " : " << slope.second << endl;
  }
  cout << INFO << "" << nCors << " valid correlations specified:" << ENDL;
  for(pair<string, string> cor : fCors) {
    cout << "\t" << cor.first << " : " << cor.second << endl;
  }
  cout << INFO << "" << nCustoms << " valid customs specified:" << ENDL;
  for(string custom : fCustoms) {
    cout << "\t" << custom << endl;
  }
}

bool TMulPlot::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend() && fCustoms.find(var) == fCustoms.cend()) {
    cerr << WARNING << "Unknown variable: " << var << ENDL;
    return false;
  }
  return true;
}

bool TMulPlot::CheckCustomVar(Node * node) {
  if (node) {
		if (	 node->token.type == variable 
				&& fCustoms.find(node->token.value) == fCustoms.cend()
				&& fVars.find(node->token.value) == fVars.cend() ) {
			cerr << WARNING << "Unknown variable: " << node->token.value << ENDL;
			return false;
		}

		bool l = CheckCustomVar(node->lchild);
		bool r = CheckCustomVar(node->rchild);
		bool s = CheckCustomVar(node->sibling);
		return l && r && s;
	}
	return true;
}

void TMulPlot::GetValues() {
  long total = 0;
  long ok = 0;
  map<string, vector<double>> vals_buf;
  map<string, double> maxes;
  double unit = 1;
  map<string, double> var_units;

  for (string var : fVars) {
    if (var.find("asym") != string::npos)
      var_units[var] = ppm;
    else if (var.find("diff") != string::npos)
      var_units[var] = um/mm; // japan output has a unit of mm

    maxes[var] = 0;
  }
  for (string custom : fCustoms)
      maxes[custom] = 0;

  for (int run : fRuns) {
    size_t sessions = fRootFiles[run].size();
    for (size_t session=0; session < sessions; session++) {
      const char *file_name = fRootFiles[run][session].c_str();
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << WARNING << "Can't open root file: " << file_name << ENDL;
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d: ", run, session)
           << file_name << ENDL;
      TTree * tin = (TTree*) f_rootfile.Get(tree);
      if (! tin) {
        cerr << WARNING << "No " << tree << " tree in root file: " << file_name << ENDL;
        continue;
      }

      bool error = false;
      for (string var : fVars) {
        string branch = fVarNames[var].first;
        string leaf   = fVarNames[var].second;
        TBranch * br = tin->GetBranch(branch.c_str());
        if (!br) {
          cerr << ERROR << "no branch: " << branch << " in tree: " << tree
            << " of file: " << file_name << ENDL;
          error = true;
          break;
        }
        TLeaf * l = br->GetLeaf(leaf.c_str());
        if (!l) {
          cerr << ERROR << "no leaf: " << leaf << " in branch: " << branch 
            << " in tree: " << tree << " of file: " << file_name << ENDL;
          error = true;
          break;
        }
        fVarLeaves[var] = l;
      }

      if (error)
        continue;

      // if (nSlopes > 0) { // FIXME no slope now
      //   tin->SetBranchAddress("coeff", slopes_buf);
      //   tin->SetBranchAddress("err_coeff", slopes_err_buf);
      // }

      const int N = tin->Draw(">>elist", cut, "entrylist");
      TEntryList *elist = (TEntryList*) gDirectory->Get("elist");
      // tin->SetEntryList(elist);
      for(int n=0; n<N; n++) {
        if (n%10000 == 0)
          cout << INFO << "read " << n << " event" << ENDL;

        const int en = elist->GetEntry(n);
        if (CheckEntryCut(total+en))
          continue;

        ok++;
        for (string var : fVars) {
          fVarLeaves[var]->GetBranch()->GetEntry(en);
          double val = fVarLeaves[var]->GetValue();

          val *= (fSigns[run] > 0 ? 1 : -1); 
          val /= var_units[var];
          vars_buf[var] = val;
          vals_buf[var].push_back(val);
          if (abs(val) > maxes[var])
            maxes[var] = abs(val);
        }
        for (string custom : fCustoms) {
          double val = get_custom_value(fCustomDefs[custom]);
          vars_buf[custom] = val;
          vals_buf[custom].push_back(val);
          if (abs(val) > maxes[custom]) 
            maxes[custom] = abs(val);
        }

        // for (pair<string, string> slope : fSlopes) {
        // }
      }
      total += tin->GetEntries();

      tin->Delete();
      f_rootfile.Close();
    }
  }

  cout << INFO << "read " << ok << "/" << total << " ok events." << ENDL;

  // initialize histogram
  for (string solo : fSolos) {
    long max = ceil(maxes[solo] * 1.05);
    long power = floor(log(max)/log(10));
    int  a = max*10 / pow(10, power);
    max = a * pow(10, power) / 10.;

    long min = -max;
    const char * unit = "";
    if (solo.find("asym") != string::npos)
      unit = "ppm";
    else if (solo.find("diff") != string::npos)
      unit = "um";

    if (fSoloCuts[solo].low != 1024)
      min = fSoloCuts[solo].low/UNITS[unit];
    if (fSoloCuts[solo].high != 1024)
      max = fSoloCuts[solo].high/UNITS[unit];
    
    fSoloHists[solo] = new TH1F(solo.c_str(), Form("%s;%s", solo.c_str(), unit), 100, min, max);
    for (int i=0; i<ok; i++)
      fSoloHists[solo]->Fill(vals_buf[solo][i]);
  }

  for (string custom : fCustoms) {
    long max = ceil(maxes[custom] * 1.05);
    long power = floor(log(max)/log(10));
    int  a = max*10 / pow(10, power);
    max = a * pow(10, power) / 10.;

    long min = -max;
    const char * unit = "";
    if (custom.find("asym") != string::npos)
      unit = "ppm";
    else if (custom.find("diff") != string::npos)
      unit = "um";

    if (fCustomCuts[custom].low != 1024)
      min = fCustomCuts[custom].low/UNITS[unit];
    if (fCustomCuts[custom].high != 1024)
      max = fCustomCuts[custom].high/UNITS[unit];
    
    // !!! add it to solo histogram
    fSoloHists[custom] = new TH1F(custom.c_str(), Form("%s;%s", custom.c_str(), unit), 100, min, max);
    for (int i=0; i<ok; i++)
      fSoloHists[custom]->Fill(vals_buf[custom][i]);
  }

  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
    double max1 = maxes[var1] * 1.2;
    double max2 = maxes[var2] * 1.2;
    long max  = ceil((max1 > max2 ? max1 : max2) * 1.05);
    long power = floor(log(max)/log(10));
    int  a = max*10 / pow(10, power);
    max = a * pow(10, power) / 10.;

    long min  = -max;
    const char * unit = "";
    if (var1.find("asym") != string::npos)
      unit = "ppm";
    else if (var1.find("diff") != string::npos)
      unit = "um";

    if (fCompCuts[comp].low != 1024)
      min = fCompCuts[comp].low/UNITS[unit];
    if (fCompCuts[comp].high != 1024)
      min = fCompCuts[comp].high/UNITS[unit];

    size_t h = hash<string>{}(var1+var2);
    fCompHists[comp].first  = new TH1F(Form("%s_%ld", var1.c_str(), h), Form("%s;%s", var1.c_str(), unit), 100, min, max);
    fCompHists[comp].second = new TH1F(Form("%s_%ld", var2.c_str(), h), Form("%s;%s", var2.c_str(), unit), 100, min, max);

    for (int i=0; i<ok; i++) {
      fCompHists[comp].first->Fill(vals_buf[var1][i]);
      fCompHists[comp].second->Fill(vals_buf[var2][i]);
    }
  }
}

double TMulPlot::get_custom_value(Node *node) {
	if (!node) {
		cerr << ERROR << "Null node" << ENDL;
		return -999999;
	}

	const char * val = node->token.value;
	double v=0, vl=0, vr=0;
	if (node->lchild) vl = get_custom_value(node->lchild);
	if (node->rchild) vr = get_custom_value(node->rchild);
	if (vl == -999999 || vr == -999999)
		return -999999;

	switch (node->token.type) {
		case opt:
			switch(val[0]) {
				case '+':
					return vl + vr;
				case '-':
					return vl - vr;
				case '*':
					return vl * vr;
				case '/':
					return vl / vr;
				case '%':
					return ((int)vl) % ((int)vr);
			}
		case function1:
			return f1[val](vl);
		case function2:
			return f2[val](vl, get_custom_value(node->lchild->sibling));
		case number:
			return atof(val);
		case variable:
			if (	 fVars.find(val) != fVars.cend()
					|| fCustoms.find(val) != fCustoms.cend())
				return vars_buf[val];
		default:
			cerr << ERROR << "unkonw token type: " << TypeName[node->token.type] << ENDL;
			return -999999;
	}
	return -999999;
}

bool TMulPlot::CheckEntryCut(const long entry) {
  if (ecuts.size() == 0) return false;

  for (pair<long, long> cut : ecuts) {
    long start = cut.first;
    long end = cut.second;
    if (entry >= start) {
      if (end == -1)
        return true;
      else if (entry < end)
        return true;
    }
  }
  return false;
}

void TMulPlot::DrawHistogram() {
  TCanvas c("c", "c", 800, 600);
  c.SetGridy();
  if (logy)
    c.SetLogy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(111110);
  gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  if (format == pdf)
    c.Print(Form("%s.pdf[", out_name));


  for (string custom : fCustoms)
    fSolos.insert(custom);
  for (string solo : fSolos) {
    c.cd();
    fSoloHists[solo]->Fit("gaus");
    TH1F * hc = (TH1F*) fSoloHists[solo]->DrawClone();

    gPad->Update();
    TPaveStats * st = (TPaveStats*) gPad->GetPrimitive("stats");
    st->SetName("myStats");
    TList* l_line = st->GetListOfLines();
    l_line->Remove(st->GetLineWith("Constant"));
    hc->SetStats(0);
    gPad->Modified();
    fSoloHists[solo]->SetStats(0);
    fSoloHists[solo]->Draw("same");

    if (format == pdf) 
      c.Print(Form("%s.pdf", out_name));
    else if (format == png)
      c.Print(Form("%s_%s.png", out_name, solo.c_str()));

    c.Clear();
  }
  for (pair<string, string> comp : fComps) {
    c.cd();
    TH1F * h1 = fCompHists[comp].first;
    TH1F * h2 = fCompHists[comp].second;
    Color_t color1 = h1->GetLineColor();
    Color_t color2 = 28;
    h1->SetTitle(Form("#color[%d]{%s} & #color[%d]{%s}", color1, comp.first.c_str(), color2, comp.second.c_str()));
    h1->Fit("gaus");
    h2->Fit("gaus");
    TH1F * hc1 = (TH1F*) h1->DrawClone();
    h2->SetLineColor(color2);
    TH1F * hc2 = (TH1F*) h2->DrawClone("sames");
    double max1 = hc1->GetMaximum();
    double min1 = hc1->GetMinimum();
    double max2 = hc2->GetMaximum();
    double min2 = hc2->GetMinimum();
    double min  = min1 < min2 ? min1 : min2;
    if (logy && min <= 0) min = 0.1;
    double max  = max1 > max2 ? max1 : max2;

    c.Update();
    TPaveStats * st1 = (TPaveStats*) hc1->FindObject("stats");
    st1->SetName("stats1");
    double width = st1->GetX2NDC() - st1->GetX1NDC();
    st1->SetX1NDC(0.1);
    st1->SetX2NDC(0.1 + width);
    st1->SetTextColor(color1);
    st1->GetListOfLines()->Remove(st1->GetLineWith("Constant"));
    hc1->SetStats(0);

    TPaveStats * st2 = (TPaveStats*) hc2->FindObject("stats");
    st2->SetName("stats2");
    st2->SetTextColor(color2);
    st2->GetListOfLines()->Remove(st2->GetLineWith("Constant"));
    hc2->SetStats(0);

    hc1->GetYaxis()->SetRangeUser(min, max);
    h1->SetStats(0);
    h1->Draw("same");
    h2->SetStats(0);
    h2->Draw("same");
    c.Modified();

    if (format == pdf) 
      c.Print(Form("%s.pdf", out_name));
    else if (format == png)
      c.Print(Form("%s_%s-%s.png", out_name, comp.first.c_str(), comp.second.c_str()));
    c.Clear();
  }

  if (format == pdf)
    c.Print(Form("%s.pdf]", out_name));

  cout << INFO << "done with drawing plots" << ENDL;
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
