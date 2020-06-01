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


typedef struct {double hw_sum, block0, block1, block2, block3, num_samples, Device_Error_Code;} japan_data;
typedef struct {double hw_sum, num_samples, Device_Error_Code;} dithering_data;

enum FileType { japan, postpan, dithering }; 
enum Format {pdf, png};

using namespace std;

class TMulPlot {

    // ClassDe (TMulPlot, 0) // mul plots

  private:
    Format      format = pdf; // default pdf output
    const char *out_name = "mulplot";
    // root file: $dir/${prefix}xxxx${midfix}000${suffix}.root
    const char *dir    = "/adaqfs/home/apar/PREX/prompt/results/";
    const char *prefix = "prexPrompt_";
    const char *suffix = "_regress_postpan";
          char  midfix = '_';
    const char *tree   = "reg";
    const char *filetype = "postpan";
    FileType    ft = postpan;
    const char *postpan_type = "prexPrompt"; // exclude quick postpan result
    const char *japan_pass = "pass1";
    const char *dit_bpm    = NULL;
          bool  logy = false;

    TConfig fConf;
    int     nSlugs;
    int	    nRuns;
    int	    nVars;
    int	    nSolos;
    int	    nComps;
    int	    nSlopes;
    int	    nCors;
    set<int>    fRuns;
    set<int>    fSlugs;
    set<string>   fVars;
    set<string>   fSolos;
    set<pair<string, string>>   fComps;
    set<pair<string, string>>   fSlopes;
    set<pair<string, string>>   fCors;
    map<string, VarCut>         fSoloCuts;	// use the low and high cut as x range
    map<pair<string, string>, VarCut>		fCompCuts;
    map<pair<string, string>, VarCut>   fSlopeCuts;
    map<pair<string, string>, VarCut>   fCorCuts;

    map<int, int> fSessions;
    map<int, int> fSigns;
    map<pair<string, string>, pair<int, int>>	  fSlopeIndexes;
    map<string, double>  vars_postpan_buf;    // read postpan output
    map<string, japan_data>  vars_japan_buf;  // read japan output
    map<string, dithering_data>  vars_dithering_buf;  // read dithering output
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
     void SetFileType(const char * ftype);
     void SetJapanPass(const char * pass);
     void SetLogy(bool log) {logy = log;}
     void SetSlugs(set<int> slugs);
     void SetRuns(set<int> runs);
     void CheckRuns();
     void CheckVars();
     void GetValues();
     // void CheckValues();
     void DrawHistogram();
};

// ClassImp(TMulPlot);

TMulPlot::TMulPlot(const char* config_file, const char* run_list) :
  fConf(config_file, run_list)
{
  fConf.ParseConfFile();
  fRuns   = fConf.GetRuns();
  fVars	  = fConf.GetVars();
  fSolos  = fConf.GetSolos();
  fComps  = fConf.GetComps();
  fSlopes = fConf.GetSlopes();
  fCors	  = fConf.GetCors();
  nSlopes = fSlopes.size();

  fSoloCuts   = fConf.GetSoloCuts();
  fCompCuts   = fConf.GetCompCuts();
  fSlopeCuts  = fConf.GetSlopeCuts();
  fCorCuts    = fConf.GetCorCuts();

  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetFileType())  SetFileType(fConf.GetFileType());
  if (fConf.GetTreeName())  tree    = fConf.GetTreeName();
  if (fConf.GetJapanPass()) SetJapanPass(fConf.GetJapanPass());
  if (fConf.GetDitBPM())    dit_bpm = fConf.GetDitBPM();
  logy = fConf.GetLogy();

  gROOT->SetBatch(1);
}

TMulPlot::~TMulPlot() {
  cout << __PRETTY_FUNCTION__ << ":INFO\t End of TMulPlot\n";
}

void TMulPlot::Draw() {
  cout << __PRETTY_FUNCTION__ << ":INFO\t draw mul plots of" << endl
       << "\troot file type: " << filetype << endl 
       << "\tin directory: " << dir << endl
       << "\tuse tree: " << tree << endl;

  CheckRuns();
  CheckVars();
  GetValues();
  DrawHistogram();
}

void TMulPlot::SetFileType(const char * ftype) {
  if (ftype == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no filetype specified" << endl;
    exit(10);
  }

  char * type = Sub(ftype, 0);
  StripSpaces(type);
  if (strcmp(type, "postpan") == 0) {
    filetype = "postpan";
    ft = postpan;
    tree = "reg"; // default tree
  } else if (strcmp(type, "japan") == 0) {
    filetype = "japan";
    ft = japan;
    tree = "mul";
  } else if (strcmp(type, "dithering") == 0) {
    filetype = "dithering";
    ft = dithering;
    tree = "dit";
  } else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t unknown root file type: " << type << ".\t Allowed type: " << endl
         << "\t postpan" << endl
         << "\t japan" << endl
         << "\t dithering" << endl;
    exit(10);
  }
}

void TMulPlot::SetDir(const char * d) {
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

void TMulPlot::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unknow output format: " << f << endl;
    exit(40);
  }
}
void TMulPlot::SetJapanPass(const char * pass) {
  if (pass == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t no pass specified, use default value: " << japan_pass << endl;
    return;
  }

  if (strcmp(pass, "pass1") == 0)
    japan_pass = pass;
  else if (strcmp(pass, "pass2") == 0)
    japan_pass = pass;
  else {
    cerr << ":FATAL\t unknow japan pass: " << pass << endl;
    exit(24);
  }
}

void TMulPlot::SetSlugs(set<int> slugs) {
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

void TMulPlot::SetRuns(set<int> runs) {
  for(set<int>::const_iterator it=runs.cbegin(); it != runs.cend(); it++) {
    int run = *it;
    if (run < START_RUN || run > END_RUN) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << endl;
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
    for(set<int>::const_iterator it=fSlugs.cbegin(); it != fSlugs.cend(); it++) {
      runs = GetRunsFromSlug(*it);
      for (set<int>::const_iterator it_r=runs.cbegin(); it_r != runs.cend(); it_r++) {
        fRuns.insert(*it_r);
      }
    }
  }
  nRuns = fRuns.size();

  // check runs against database
  GetValidRuns(fRuns);
  nRuns = fRuns.size();

  bool flag = false;
  for (set<int>::const_iterator it = fRuns.cbegin(); it != fRuns.cend(); ) {
    int run = *it;
    const char * pattern  = Form("%s/*%s*%d[_.]???*.root", dir, postpan_type, run);
    if (ft == japan)
      pattern = Form("%s/*%s*%d[_.]???*.root", dir, japan_pass, run);
    else if (ft == dithering) {
      if (dit_bpm)
        pattern = Form("%s/*%s*%d[_.]???*.root", dir, dit_bpm, run);
      else
        pattern = Form("%s/*dither_%d[_.]???*.root", dir, run); // FIXME
    }

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
  for(set<int>::const_iterator it=fRuns.cbegin(); it!=fRuns.cend(); it++) {
    cout << "\t" << *it << endl;
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
    const char * file_name = Form("%s/%s%d%c000%s.root", dir, prefix, run, midfix, suffix);
    TFile * f_rootfile = new TFile(file_name, "read");
    if (f_rootfile->IsOpen()) {
      if (!f_rootfile->GetListOfKeys()->Contains(tree)) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t no tree: " << tree << " in root file: " << file_name;
        exit(22);
      }
      TTree * tin = (TTree*) f_rootfile->Get(tree);
      if (tin != NULL) {
        TObjArray * l_var = tin->GetListOfBranches();
        bool error_var_flag = false;
        cout << __PRETTY_FUNCTION__ << ":INFO\t use file to check vars: " << file_name << endl;
        for (set<string>::const_iterator it_v=fVars.cbegin(); it_v != fVars.cend(); ) {
          if (!l_var->FindObject(it_v->c_str())) {
            cerr << __PRETTY_FUNCTION__ << ":WARNING\t Variable not found: " << *it_v << endl;
            it_v = fVars.erase(it_v);
            error_var_flag = true;
          } else
            it_v++;
        }
        if (error_var_flag) {
          TIter next(l_var);
          TBranch *br;
          cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid variables:\n";
          while (br = (TBranch*) next()) {
            cout << "\t" << br->GetName() << endl;
          }
        }

        /*  FIXME: no slope now
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
          for (set<pair<string, string>>::const_iterator it=fSlopes.cbegin(); it != fSlopes.cend(); ) {
            string dv = it->first;
            string iv = it->second;
            vector<TString>::const_iterator it_dv = find(l_dv->cbegin(), l_dv->cend(), dv);
            vector<TString>::const_iterator it_iv = find(l_iv->cbegin(), l_iv->cend(), iv);
            if (it_dv == l_dv->cend()) {
              cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid dv name for slope: " << dv << endl;
              it = fSlopes.erase(it);
              error_dv_flag = true;
              continue;
            }
            if (it_iv == l_iv->cend()) {
              cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid iv name for slope: " << iv << endl;
              it = fSlopes.erase(it);
              error_iv_flag = true;
              continue;
            }
            fSlopeIndexes[*it] = make_pair(it_dv-l_dv->cbegin(), it_iv-l_iv->cbegin());
            it++;
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
        */
        f_rootfile->Close();
        break;
      }
    } 
      
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t root file of run: " << run << " is broken, ignore it.\n";
    it_r = fRuns.erase(it_r);

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
    if (fVars.find(*it) == fVars.cend()) {
			map<string, VarCut>::const_iterator it_c = fSoloCuts.find(*it);
			if (it_c != fSoloCuts.cend())
				fSoloCuts.erase(it_c);

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid solo variable: " << *it << endl;
      it = fSolos.erase(it);
		} else
      it++;
  }

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); ) {
    if (fVars.find(it->first) == fVars.cend() || fVars.find(it->second) == fVars.cend()) {
			map<pair<string, string>, VarCut>::const_iterator it_c = fCompCuts.find(*it);
			if (it_c != fCompCuts.cend())
				fCompCuts.erase(it_c);

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid Comp variable: " << it->first << "\t" << it->second << endl;
      it = fComps.erase(it);
		} else 
			it++;
  }

  for (set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); ) {
    if (fVars.find(it->first) == fVars.cend() || fVars.find(it->second) == fVars.cend()) {
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

void TMulPlot::GetValues() {
  // initialize histogram
  for (set<string>::const_iterator it=fSolos.cbegin(); it != fSolos.cend(); it++) {
    string var = *it;
    double min = -100;
    double max = 100;
    const char * unit = "";
    if (var.find("asym") != string::npos) {
      unit = "ppm";
      min  = -3000;
      max  = 3000;
    }
    else if (var.find("diff") != string::npos) {
      unit = "um";
      min  = -50;
      max  = 50;
    } 

    if (fSoloCuts[*it].low != 1024)
      min = fSoloCuts[*it].low;
    if (fSoloCuts[*it].high != 1024)
      max = fSoloCuts[*it].high;
    
    fSoloHists[var] = new TH1F(var.c_str(), Form("%s;%s", var.c_str(), unit), 100, min, max);
  }

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it != fComps.cend(); it++) {
    string var1 = it->first;
    string var2 = it->second;
    double min = -100;
    double max = 100;
    const char * unit = "";
    if (var1.find("asym") != string::npos) {
      unit = "ppm";
      min  = -3000;
      max  = 3000;
    }
    else if (var1.find("diff") != string::npos) {
      unit = "um";
      min  = -50;
      max  = 50;
    } 

    if (fCompCuts[*it].low != 1024)
      min = fCompCuts[*it].low;
    if (fCompCuts[*it].high != 1024)
      max = fCompCuts[*it].high;

    size_t h = hash<string>{}(var1+var2);
    fCompHists[*it].first  = new TH1F(Form("%s_%ld", var1.c_str(), h), Form("%s;%s", var1.c_str(), unit), 100, min, max);
    fCompHists[*it].second = new TH1F(Form("%s_%ld", var2.c_str(), h), Form("%s;%s", var2.c_str(), unit), 100, min, max);
  }

  for (set<string>::const_iterator it=fVars.cbegin(); it!=fVars.cend(); it++) {
    vars_postpan_buf[*it] = 1024;
    vars_japan_buf[*it] = {1024, 1024, 1024, 1024, 1024, 1024, 1024};
    vars_dithering_buf[*it] = {1024, 1024, 1024};
  }

  int total = 0;
  for (set<int>::const_iterator it_r=fRuns.cbegin(); it_r!=fRuns.cend(); it_r++) {
    int run = *it_r;
    for (int session=0; session<fSessions[run]; session++) {
      const char * file_name = Form("%s/%s%d%c%03d%s.root", dir, prefix, run, midfix, session, suffix);
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d: ", run, session)
           << Form("%s%d%c%03d%s.root", prefix, run, midfix, session, suffix) << endl;
      TTree * tin = (TTree*) f_rootfile.Get(tree);
      if (! tin) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t No " << tree << " tree in root file: " << file_name << endl;
        continue;
      }

      double ErrorFlag = 1024, ok_cut = 0;
      for (set<string>::const_iterator it_v=fVars.cbegin(); it_v!=fVars.cend(); it_v++) {
        if (ft == postpan) {
          tin->SetBranchAddress(it_v->c_str(), &(vars_postpan_buf[*it_v]));
          tin->SetBranchAddress("ok_cut", &ok_cut);
        } else if (ft == japan) {
          tin->SetBranchAddress(it_v->c_str(), &(vars_japan_buf[*it_v]));
          tin->SetBranchAddress("ErrorFlag", &ErrorFlag);
        } else if (ft == dithering) {
          tin->SetBranchAddress(it_v->c_str(), &(vars_dithering_buf[*it_v]));
          tin->SetBranchAddress("ErrorFlag", &ErrorFlag);
        }
      }

      // if (nSlopes > 0) { // FIXME no slope now
      //   tin->SetBranchAddress("coeff", slopes_buf);
      //   tin->SetBranchAddress("err_coeff", slopes_err_buf);
      // }

      const int nentries = tin->GetEntries();
      bool ok = false;
      for(int n=0; n<nentries; n++) {
        tin->GetEntry(n);
        if (n%10000 == 0)
          cout << __PRETTY_FUNCTION__ << ":INFO\t processing " << n << " event\n";

        if (ft == postpan)
          ok  = ok_cut ? true : false;
        else if (ft == japan)
          ok  = !ErrorFlag ? true : false;
        else if (ft == dithering)
          ok  = !ErrorFlag ? true : false;

        if (!ok)  continue;

        for (set<string>::const_iterator it_v=fSolos.cbegin(); it_v!=fSolos.cend(); it_v++) {
          double val;
          if (ft == postpan)
            val = vars_postpan_buf[*it_v];
          else if (ft == japan)
            val = vars_japan_buf[*it_v].hw_sum;
          else if (ft == dithering)
            val = vars_dithering_buf[*it_v].hw_sum;

          double unit = 1;
          if (it_v->find("asym") != string::npos)
            unit = ppm;
          else if (it_v->find("diff") != string::npos)
            unit = um/mm; // japan output has a unit of mm

          val *= fSigns[run];
          val /= unit;
          fSoloHists[*it_v]->Fill(val);
        }
        for (set<pair<string, string>>::const_iterator it_v=fComps.cbegin(); it_v!=fComps.cend(); it_v++) {
          double val1, val2;
          if (ft == postpan) {
            val1 = vars_postpan_buf[it_v->first];
            val2 = vars_postpan_buf[it_v->second];
          } else if (ft == japan) {
            val1 = vars_japan_buf[it_v->first].hw_sum;
            val2 = vars_japan_buf[it_v->second].hw_sum;
          } else if (ft == dithering) {
            val1 = vars_dithering_buf[it_v->first].hw_sum;
            val2 = vars_dithering_buf[it_v->second].hw_sum;
          }

          double unit = 1;
          if ((it_v->first).find("asym") != string::npos)
            unit = ppm;
          else if ((it_v->first).find("diff") != string::npos)
            unit = mm;

          val1 *= fSigns[run];
          val2 *= fSigns[run];
          val1 /= unit;
          val2 /= unit;
          fCompHists[*it_v].first->Fill(val1);
          fCompHists[*it_v].second->Fill(val2);
        }

        // for (set<pair<string, string>>::const_iterator it=fSlopes.cbegin(); it!=fSlopes.cend(); it++) {
        // }
      }
      total += nentries;

      tin->Delete();
      f_rootfile.Close();
    }
  }

  cout << __PRETTY_FUNCTION__ << ":INFO\t Read " << total << " entries in total\n";
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


  for (set<string>::const_iterator it=fSolos.cbegin(); it!=fSolos.cend(); it++) {
    c.cd();
    fSoloHists[*it]->Fit("gaus");
    TH1F * hc = (TH1F*) fSoloHists[*it]->DrawClone();

    gPad->Update();
    TPaveStats * st = (TPaveStats*) gPad->GetPrimitive("stats");
    st->SetName("myStats");
    TList* l_line = st->GetListOfLines();
    l_line->Remove(st->GetLineWith("Constant"));
    hc->SetStats(0);
    gPad->Modified();
    fSoloHists[*it]->SetStats(0);
    fSoloHists[*it]->Draw("same");

    if (format == pdf) 
      c.Print(Form("%s.pdf", out_name));
    else if (format == png)
      c.Print(Form("%s_%s.png", out_name, it->c_str()));

    c.Clear();
  }
  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); it++) {
    c.cd();
    TH1F * h1 = fCompHists[*it].first;
    TH1F * h2 = fCompHists[*it].second;
    Color_t color1 = h1->GetLineColor();
    Color_t color2 = 28;
    h1->SetTitle(Form("#color[%d]{%s} & #color[%d]{%s}", color1, it->first.c_str(), color2, it->second.c_str()));
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
      c.Print(Form("%s_%s-%s.png", out_name, it->first.c_str(), it->second.c_str()));
    c.Clear();
  }

  if (format == pdf)
    c.Print(Form("%s.pdf]", out_name));

  cout << __PRETTY_FUNCTION__ << ":INFO\t done with drawing plots\n";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
