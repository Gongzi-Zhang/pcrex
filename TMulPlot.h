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


typedef struct {double hw_sum, block0, block1, block2, block3, num_samples, Device_Error_Code;} japan_data;
typedef struct {double hw_sum, num_samples, Device_Error_Code;} dithering_data;

enum FileType { japan, postpan, dithering }; 
enum Format {pdf, png};

using namespace std;

class TMulPlot {

    // ClassDe (TMulPlot, 0) // mul plots

  private:
    Format format     = pdf; // default pdf output
    const char *out_name = "mulplot";
    // root file: $dir/$pattern
    const char *filetype = "postpan";
    FileType ft       = postpan;
    const char *dir   = "/adaqfs/home/apar/PREX/prompt/results/";
    string pattern    = "prexPrompt_xxxx_???_regress_postpan.root";
    const char *tree  = "reg";
    const char *cut   = "ok_cut";
    bool logy         = false;

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

    map<int, vector<string>> fRootFiles;
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
     void SetLogy(bool log) {logy = log;}
     void SetSlugs(set<int> slugs);
     void SetRuns(set<int> runs);
     void CheckRuns();
     void CheckVars();
     void GetValues();
     void CheckValues();
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

  if (fConf.GetFileType())  SetFileType(fConf.GetFileType()); // must precede the following statements
  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetPattern())   pattern = fConf.GetPattern();
  if (fConf.GetTreeName())  tree    = fConf.GetTreeName();
  if (fConf.GetTreeCut())   cut     = fConf.GetTreeCut();
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
       << "\tfrom files: " << pattern << endl
       << "\tuse tree: " << tree << endl;

  CheckRuns();
  CheckVars();
  // CheckValues();
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
    dir = "/chafs2/work1/apar/postpan-outputs/";
    pattern = "prexPrompt_xxxx_???_regress_postpan.root";
    tree = "reg"; // default tree
    cut  = "ok_cut";  // default cut
  } else if (strcmp(type, "japan") == 0) {
    filetype = "japan";
    ft = japan;
    dir = "/chafs2/work1/apar/japanOutput/";
    pattern = "prexPrompt_pass2_xxxx.???.root";
    tree = "mul";
    cut  = "ErrorFlag == 0";  // default cut
  } else if (strcmp(type, "dithering") == 0) {
    filetype = "dithering";
    ft = dithering;
    dir = "/chafs2/work1/apar/DitherCorrection/";
    pattern = "prexPrompt_dither_1X_xxxx_???.root";
    tree = "dit";
    cut  = "ErrorFlag == 0";  // default cut
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
  if (stat(d, &info) != 0) {
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

void TMulPlot::SetSlugs(set<int> slugs) {
  for(int slug : slugs) {
    if (slug < START_SLUG || slug > END_SLUG) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid slug number (" << START_SLUG << "-" << END_SLUG << "): " << slug << endl;
      continue;
    }
    fSlugs.insert(slug);
  }
  nSlugs = fSlugs.size();
}

void TMulPlot::SetRuns(set<int> runs) {
  for(int run : runs) {
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
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t no tree: " << tree << " in root file: " << file_name;
        exit(22);
      }
      TTree * tin = (TTree*) f_rootfile->Get(tree);
      if (tin != NULL) {
        TObjArray * l_var = tin->GetListOfBranches();
        bool error_var_flag = false;
        cout << __PRETTY_FUNCTION__ << ":INFO\t use file to check vars: " << file_name << endl;
        for (string var : fVars) {
          if (!l_var->FindObject(var.c_str())) {
            cerr << __PRETTY_FUNCTION__ << ":FATAL\t Variable not found: " << var << endl;
            error_var_flag = true;
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
          exit(23);
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
        tin->Delete();
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
  for(string solo : fSolos) {
    cout << "\t" << solo << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nComps << " valid comparisons specified:\n";
  for(pair<string, string> comp : fComps) {
    cout << "\t" << comp.first << " , " << comp.second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSlopes << " valid slopes specified:\n";
  for(pair<string, string> slope : fSlopes) {
    cout << "\t" << slope.first << " : " << slope.second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nCors << " valid correlations specified:\n";
  for(pair<string, string> cor : fCors) {
    cout << "\t" << cor.first << " : " << cor.second << endl;
  }
}

void TMulPlot::GetValues() {
  // initialize histogram
  for (string solo : fSolos) {
    double min = -100;
    double max = 100;
    const char * unit = "";
    if (solo.find("asym") != string::npos) {
      unit = "ppm";
      min  = -3000;
      max  = 3000;
    }
    else if (solo.find("diff") != string::npos) {
      unit = "um";
      min  = -50;
      max  = 50;
    } 

    if (fSoloCuts[solo].low != 1024)
      min = fSoloCuts[solo].low;
    if (fSoloCuts[solo].high != 1024)
      max = fSoloCuts[solo].high;
    
    fSoloHists[solo] = new TH1F(solo.c_str(), Form("%s;%s", solo.c_str(), unit), 100, min, max);
  }

  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
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

    if (fCompCuts[comp].low != 1024)
      min = fCompCuts[comp].low;
    if (fCompCuts[comp].high != 1024)
      max = fCompCuts[comp].high;

    size_t h = hash<string>{}(var1+var2);
    fCompHists[comp].first  = new TH1F(Form("%s_%ld", var1.c_str(), h), Form("%s;%s", var1.c_str(), unit), 100, min, max);
    fCompHists[comp].second = new TH1F(Form("%s_%ld", var2.c_str(), h), Form("%s;%s", var2.c_str(), unit), 100, min, max);
  }

  for (string var : fVars) {
    vars_postpan_buf[var] = 1024;
    vars_japan_buf[var] = {1024, 1024, 1024, 1024, 1024, 1024, 1024};
    vars_dithering_buf[var] = {1024, 1024, 1024};
  }

  int total = 0;
  for (int run : fRuns) {
    size_t sessions = fRootFiles[run].size();
    for (size_t session=0; session < sessions; session++) {
      const char *file_name = fRootFiles[run][session].c_str();
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d: ", run, session)
           << file_name << endl;
      TTree * tin = (TTree*) f_rootfile.Get(tree);
      if (! tin) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t No " << tree << " tree in root file: " << file_name << endl;
        continue;
      }

      for (string var : fVars) {
        if (ft == postpan) {
          tin->SetBranchAddress(var.c_str(), &(vars_postpan_buf[var]));
        } else if (ft == japan) {
          tin->SetBranchAddress(var.c_str(), &(vars_japan_buf[var]));
        } else if (ft == dithering) {
          tin->SetBranchAddress(var.c_str(), &(vars_dithering_buf[var]));
        }
      }

      // if (nSlopes > 0) { // FIXME no slope now
      //   tin->SetBranchAddress("coeff", slopes_buf);
      //   tin->SetBranchAddress("err_coeff", slopes_err_buf);
      // }

      const int nentries = tin->Draw(">>elist", cut, "entrylist");
      TEntryList *elist = (TEntryList*) gDirectory->Get("elist");
      // tin->SetEntryList(elist);
      for(int n=0; n<nentries; n++) {
        tin->GetEntry(elist->GetEntry(n));
        if (n%10000 == 0)
          cout << __PRETTY_FUNCTION__ << ":INFO\t processing " << n << " event\n";

        for (string solo : fSolos) {
          double val;
          if (ft == postpan)
            val = vars_postpan_buf[solo];
          else if (ft == japan)
            val = vars_japan_buf[solo].hw_sum;
          else if (ft == dithering)
            val = vars_dithering_buf[solo].hw_sum;

          double unit = 1;
          if (solo.find("asym") != string::npos)
            unit = ppm;
          else if (solo.find("diff") != string::npos)
            unit = um/mm; // japan output has a unit of mm

          val *= fSigns[run];
          val /= unit;
          fSoloHists[solo]->Fill(val);
        }
        for (pair<string, string> vars : fComps) {
          double val1, val2;
          if (ft == postpan) {
            val1 = vars_postpan_buf[vars.first];
            val2 = vars_postpan_buf[vars.second];
          } else if (ft == japan) {
            val1 = vars_japan_buf[vars.first].hw_sum;
            val2 = vars_japan_buf[vars.second].hw_sum;
          } else if (ft == dithering) {
            val1 = vars_dithering_buf[vars.first].hw_sum;
            val2 = vars_dithering_buf[vars.second].hw_sum;
          }

          double unit = 1;
          if ((vars.first).find("asym") != string::npos)
            unit = ppm;
          else if ((vars.first).find("diff") != string::npos)
            unit = mm;

          val1 *= fSigns[run];
          val2 *= fSigns[run];
          val1 /= unit;
          val2 /= unit;
          fCompHists[vars].first->Fill(val1);
          fCompHists[vars].second->Fill(val2);
        }

        // for (pair<string, string> vars : fSlopes) {
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


  for (string var : fSolos) {
    c.cd();
    fSoloHists[var]->Fit("gaus");
    TH1F * hc = (TH1F*) fSoloHists[var]->DrawClone();

    gPad->Update();
    TPaveStats * st = (TPaveStats*) gPad->GetPrimitive("stats");
    st->SetName("myStats");
    TList* l_line = st->GetListOfLines();
    l_line->Remove(st->GetLineWith("Constant"));
    hc->SetStats(0);
    gPad->Modified();
    fSoloHists[var]->SetStats(0);
    fSoloHists[var]->Draw("same");

    if (format == pdf) 
      c.Print(Form("%s.pdf", out_name));
    else if (format == png)
      c.Print(Form("%s_%s.png", out_name, var.c_str()));

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

  cout << __PRETTY_FUNCTION__ << ":INFO\t done with drawing plots\n";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
