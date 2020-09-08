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
#include "TH2F.h"
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
#include "TBase.h"



using namespace std;

class TMulPlot : public TBase {

    // ClassDe (TMulPlot, 0) // mul plots

  private:	// class specific variables besides those on base class
		bool logy = false;

		map<int, long> Entries;
		map<string, vector<double>> vals_buf;
    map<string, TH1F *>    fSoloHists;
    map<pair<string, string>, pair<TH1F *, TH1F *>>    fCompHists;
    // map<pair<string, string>, TH1F *>    fSlopeHists;
    map<pair<string, string>, TH2F *>    fCorHists;

  public:
     TMulPlot(const char* confif_file, const char* run_list = NULL);
     ~TMulPlot();
     void SetLogy(bool log) {logy = log;}
     void Draw();
     void GetValues();
		 double get_custom_value(Node * node);
     void DrawHistograms();
		 void GetOutliers();
};

// ClassImp(TMulPlot);

TMulPlot::TMulPlot(const char* config_file, const char* run_list) :
	TBase(config_file, run_list)
{
	TBase::out_name = "mulplot";
	// dir = "/adaqfs/home/apar/PREX/prompt/results/";
	// pattern = "prexPrompt_xxxx_???_regress_postpan.root";
	// tree = "reg";
	// cut = "ok_cut";
  logy  = fConf.GetLogy();
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
  DrawHistograms();
}

void TMulPlot::GetValues() {
  long total = 0;
  long ok = 0;
  map<string, double> maxes;
  double unit = 1;
  map<string, double> var_units;

  for (string var : fVars) {
		vals_buf[var].clear();	// Initialization

    if (var.find("asym") != string::npos)
      var_units[var] = ppm;
    else if (var.find("diff") != string::npos)
      var_units[var] = um/mm; // japan output has a unit of mm

    maxes[var] = 0;
  }
  for (string custom : fCustoms)
      maxes[custom] = 0;

  for (int run : fRuns) {
    const size_t sessions = fRootFiles[run].size();
    for (size_t session=0; session < sessions; session++) {
      const char *file_name = fRootFiles[run][session].c_str();
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << ERROR << "run-" << run << " ^^^^ Can't open root file: " << file_name << ENDL;
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d: ", run, session)
           << file_name << ENDL;
      TTree * tin = (TTree*) f_rootfile.Get(tree);
      if (! tin) {
        cerr << ERROR << "No such tree: " << tree << " in root file: " << file_name << ENDL;
        continue;
      }

			for (auto const ftree : ftrees) {
				string file_name = ftree.second;
				if (file_name.find("xxxx") != string::npos)
					file_name.replace(file_name.find("xxxx"), 4, to_string(run));
				if (file_name.size()) {
					glob_t globbuf;
					glob(file_name.c_str(), 0, NULL, &globbuf);
					if (globbuf.gl_pathc != sessions) {
						cerr << ERROR << "run-" << run << " ^^^^ unmatched friend tree root files: " << endl 
								 << "\t" << sessions << "main root files vs " 
								 << globbuf.gl_pathc << " friend tree root files" << ENDL; 
						continue;
					}
					file_name = globbuf.gl_pathv[session];
					globfree(&globbuf);
				}
				tin->AddFriend(ftree.first.c_str(), file_name.c_str());	// FIXME: what if the friend tree doesn't exist
			}

      bool error = false;
      for (string var : fVars) {
        string branch = fVarNames[var].first;
        string leaf   = fVarNames[var].second;
        TBranch * br = tin->GetBranch(branch.c_str());
        if (!br) {
					// special branches -- stupid
					if (branch.find("diff_bpm11X") != string::npos && run < 3390) {	
						// no bpm11X in early runs, replace with bpm12X
						cout << WARNING << "run-" << run << " ^^^^ No branch diff_bpm11X, replace with 0.6*diff_bpm12X" << ENDL;
						br = tin->GetBranch(branch.replace(branch.find("bpm11X"), 6, "bpm12X").c_str());
					} else if (branch.find("diff_bpmE") != string::npos && run < 3390) {	
						cout << WARNING << "run-" << run << " ^^^^ No branch diff_bpmE, replace with diff_bpm12X" << ENDL;
						br = tin->GetBranch(branch.replace(branch.find("bpmE"), 4, "bpm12X").c_str());
					} 
					if (!br) {
						cerr << ERROR << "no branch: " << branch << " in tree: " << tree
							<< " of file: " << file_name << ENDL;
						error = true;
						break;
					}
        }
        TLeaf * l = br->GetLeaf(leaf.c_str());
        if (!l) {
					if (leaf.find("diff_bpm11X") != string::npos && run < 3390) {		// lagrange tree
						l = br->GetLeaf(leaf.replace(leaf.find("bpm11X"), 6, "bpm12X").c_str());
					} else if (leaf.find("diff_bpmE") != string::npos && run < 3390) {		// reg tree
						l = br->GetLeaf(leaf.replace(leaf.find("bpmE"), 4, "bpm12X").c_str());
					} 
					if (!l) {
						cerr << ERROR << "no leaf: " << leaf << " in branch: " << branch  << "in var: " << var
							<< " in tree: " << tree << " of file: " << file_name << ENDL;
						error = true;
						break;
					}
        }
        fVarLeaves[var] = l;
      }

      if (error)
        continue;

      // if (nSlopes > 0) { // FIXME no slope now
      //   tin->SetBranchAddress("coeff", slopes_buf);
      //   tin->SetBranchAddress("err_coeff", slopes_err_buf);
      // }

      int N = tin->Draw(">>elist", cut, "entrylist");
			if (N < 4500) {
				cerr << WARNING << "run-" << run << " ^^^^ too short (< 4500 patterns), ignore it" <<ENDL;
				continue;
			}
      TEntryList *elist = (TEntryList*) gDirectory->Get("elist");
      // tin->SetEntryList(elist);
      for(int n=0; n<N; n++) {
        if (n%10000 == 0)
          cout << INFO << "read " << n << " event" << ENDL;

        const int en = elist->GetEntry(n);
        // if (CheckEntryCut(total+en))
        //   continue;

        ok++;
        for (string var : fVars) {
          fVarLeaves[var]->GetBranch()->GetEntry(en);
          double val = fVarLeaves[var]->GetValue();

          val *= (fSigns[run] > 0 ? 1 : -1); 
          val /= var_units[var];
					if (var.find("bpm11X") != string::npos && run < 3390)		// special treatment of bpmE = bpm11X + 0.4*bpm12X
						val *= 0.6;
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
		Entries[run] = ok;
  }

	if (ok == 0) {
		cout << ERROR << "no valid event" << ENDL;
		exit(44);
	}
  cout << INFO << "read " << ok << "/" << total << " ok events." << ENDL;

  // initialize histogram
  for (string solo : fSolos) {
    // long max = ceil(maxes[solo] * 1.05);
    long max = ceil(maxes[solo]);
    int power = floor(log(max)/log(10));
    int  a = max*10 / pow(10, power);
    max = (a+1) * pow(10, power) / 10.;

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

	for (pair<string, string> cor : fCors) {
    string xvar = cor.second;
    string yvar = cor.first;
    long xmax = ceil(maxes[xvar] * 1.05);
    long ymax = ceil(maxes[yvar] * 1.05);
    long power = floor(log(xmax)/log(10));
    int  a = xmax*10 / pow(10, power);
    xmax = a * pow(10, power) / 10.;
    power = floor(log(ymax)/log(10));
    a = ymax*10 / pow(10, power);
    ymax = a * pow(10, power) / 10.;
		long xmin = -xmax;
		long ymin = -ymax;

    const char * xunit = "", *yunit = "";
    if (xvar.find("asym") != string::npos)
      xunit = "ppm";
    else if (xvar.find("diff") != string::npos)
      xunit = "um";

    if (yvar.find("asym") != string::npos)
      yunit = "ppm";
    else if (yvar.find("diff") != string::npos)
      yunit = "um";

    // if (fCorCuts[cor].low != 1024)
    //   min = fCorCuts[cor].low/UNITS[unit];
    // if (fCompCuts[comp].high != 1024)
    //   min = fCompCuts[comp].high/UNITS[unit];

    fCorHists[cor] = new TH2F((yvar + xvar).c_str(), 
				Form("%s vs %s; %s/%s; %s/%s", yvar.c_str(), xvar.c_str(), xvar.c_str(), xunit, yvar.c_str(), yunit),
				100, xmin, xmax,
				100, ymin, ymax);

    for (int i=0; i<ok; i++) {
      fCorHists[cor]->Fill(vals_buf[xvar][i], vals_buf[yvar][i]);
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
					|| find(fCustoms.cbegin(), fCustoms.cend(), val) != fCustoms.cend())
				return vars_buf[val];
		default:
			cerr << ERROR << "unkonw token type: " << TypeName[node->token.type] << ENDL;
			return -999999;
	}
	return -999999;
}

// bool TMulPlot::CheckEntryCut(const long entry) {
//   if (ecuts.size() == 0) return false;
// 
//   for (pair<long, long> cut : ecuts) {
//     long start = cut.first;
//     long end = cut.second;
//     if (entry >= start) {
//       if (end == -1)
//         return true;
//       else if (entry < end)
//         return true;
//     }
//   }
//   return false;
// }

void TMulPlot::DrawHistograms() {
  TCanvas c("c", "c", 800, 600);
  c.SetGridy();
  if (logy)
    c.SetLogy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(112210);
  gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  if (format == pdf)
    c.Print(Form("%s.pdf[", out_name));

  for (string custom : fCustoms)
    fSolos.push_back(custom);
  for (string solo : fSolos) {
    c.cd();
    fSoloHists[solo]->Fit("gaus");
    TH1F * hc = (TH1F*) fSoloHists[solo]->DrawClone();
		cout << OUTPUT << solo << "--mean:\t" << fSoloHists[solo]->GetMean() << " ± " << fSoloHists[solo]->GetMeanError() << ENDL;
		cout << OUTPUT << solo << "--rms:\t" << fSoloHists[solo]->GetRMS() << " ± " << fSoloHists[solo]->GetRMSError() << ENDL;

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

	c.SetLogy(false);
  gStyle->SetOptFit(0);
  gStyle->SetOptStat(0);
	for (pair<string, string> cor : fCors) {
    c.cd();
		fCorHists[cor]->SetStats(false);
		fCorHists[cor]->Draw("colorz");

    if (format == pdf) 
      c.Print(Form("%s.pdf", out_name));
    else if (format == png)
      c.Print(Form("%s_%s_vs_%s.png", out_name, cor.first.c_str(), cor.second.c_str()));
    c.Clear();
	}

  if (format == pdf)
    c.Print(Form("%s.pdf]", out_name));

  cout << INFO << "done with drawing histograms" << ENDL;
}

void TMulPlot::GetOutliers() {
	map<string, pair<double, double>> fSoloOutliers;
	double ratio = 4;
	for (string solo : fSolos) {
		fSoloOutliers[solo].first = 9999;
		fSoloOutliers[solo].second = 9999;
		const int nBins = fSoloHists[solo]->GetNbinsX();
		TF1 * func = fSoloHists[solo]->GetFunction("gaus");
		// get fit parameters, define range of outliers
		for (int i=nBins/2; i>0; i--) {
			double hVal = fSoloHists[solo]->GetBinContent(i);
			double fVal = func->Eval(fSoloHists[solo]->GetBinCenter(i));
			if (abs(hVal/fVal) > ratio || abs(fVal/hVal) > ratio) {	// test next 2 bins
				double hv1 = fSoloHists[solo]->GetBinContent(i-1);
				double fv1 = func->Eval(fSoloHists[solo]->GetBinCenter(i-1));
				double hv2 = fSoloHists[solo]->GetBinContent(i-2);
				double fv2 = func->Eval(fSoloHists[solo]->GetBinCenter(i-2));
				if (i > 2) {
					if ((abs(hv1/fv1) > ratio || abs(fv1/hv1) > ratio) && (abs(hv2/fv2) > ratio || abs(fv2/hv2) > ratio)) {
						fSoloOutliers[solo].first = fSoloHists[solo]->GetBinLowEdge(i+1);
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i) << "\t" << hVal << "\t" << fVal << ENDL;
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i-1) << "\t" << hv1 << "\t" << fv1 << ENDL;
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i-2) << "\t" << hv2 << "\t" << fv2 << ENDL;
						break;
					}
				} else if (i > 1) {
					if ((abs(hv1/fv1) > ratio || abs(fv1/hv1) > ratio)) {
						fSoloOutliers[solo].first = fSoloHists[solo]->GetBinLowEdge(i+1);
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i) << "\t" << hVal << "\t" << fVal << ENDL;
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i-1) << "\t" << hv1 << "\t" << fv1 << ENDL;
						break;
					}
				} else {
					fSoloOutliers[solo].first = fSoloHists[solo]->GetBinLowEdge(i+1);
					cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i) << "\t" << hVal << "\t" << fVal << ENDL;
					break;
				}
			}
		}
		for (int i=nBins/2; i<=nBins; i++) {
			double hVal = fSoloHists[solo]->GetBinContent(i);
			double fVal = func->Eval(fSoloHists[solo]->GetBinCenter(i));
			if (abs(hVal/fVal) > ratio || abs(fVal/hVal) > ratio) {	// test next 2 bins
				double hv1 = fSoloHists[solo]->GetBinContent(i+1);
				double fv1 = func->Eval(fSoloHists[solo]->GetBinCenter(i+1));
				double hv2 = fSoloHists[solo]->GetBinContent(i+2);
				double fv2 = func->Eval(fSoloHists[solo]->GetBinCenter(i+2));
				if (i < nBins-1) {
					if ((abs(hv1/fv1) > ratio || abs(fv1/hv1) > ratio) && (abs(hv2/fv2) > ratio || abs(fv2/hv2) > ratio)) {
						fSoloOutliers[solo].second = fSoloHists[solo]->GetBinLowEdge(i);
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i) << "\t" << hVal << "\t" << fVal << ENDL;
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i+1) << "\t" << hv1 << "\t" << fv1 << ENDL;
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i+2) << "\t" << hv2 << "\t" << fv2 << ENDL;
						break;
					}
				} else if (i < nBins) {
					if ((abs(hv1/fv1) > ratio || abs(fv1/hv1) > ratio)) {
						fSoloOutliers[solo].second = fSoloHists[solo]->GetBinLowEdge(i);
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i) << "\t" << hVal << "\t" << fVal << ENDL;
						cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i+1) << "\t" << hv1 << "\t" << fv1 << ENDL;
						break;
					}
				} else {
					fSoloOutliers[solo].second = fSoloHists[solo]->GetBinLowEdge(i);
					cout << INFO << "outlier values at: " << fSoloHists[solo]->GetBinCenter(i) << "\t" << hVal << "\t" << fVal << ENDL;
					break;
				}
			}
		}
		if (fSoloOutliers[solo].first != 9999 || fSoloOutliers[solo].second != 9999) {
			cout << INFO << "find outlier in variable: " << solo << " with cut values: " 
					 << fSoloOutliers[solo].first << "\t" << fSoloOutliers[solo].second << ENDL;
			bool lout = false, hout = false;
			double lval, hval;
			if (fSoloOutliers[solo].first != 9999) {
				lout = true;
				lval = fSoloOutliers[solo].first;
			}
			if (fSoloOutliers[solo].second != 9999) {
				hout = true;
				hval = fSoloOutliers[solo].second;
			}
			long iok = 0;
			for (int run : fRuns) {
				const long N = Entries[run];
				for (; iok<N; iok++) {
					double val = vals_buf[solo][iok];
					if ((lout && val < lval) || (hout && val > hval)) {
						cout << ALERT << "Outlier in run: " << run << "\tvariable: " << solo << "\tvalue: " << val << ENDL;
					}
				}
			}
		}
	}
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
