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
#include "TRunBase.h"



using namespace std;

class TMulPlot : public TRunBase {

    // ClassDe (TMulPlot, 0) // mul plots

  private:	// class specific variables besides those on base class
		bool logy = false;

    map<string, TH1F *>    fSoloHists;
    map<pair<string, string>, pair<TH1F *, TH1F *>>    fCompHists;
    // map<pair<string, string>, TH1F *>    fSlopeHists;
    map<pair<string, string>, TH2F *>    fCorHists;

  public:
     TMulPlot();
     ~TMulPlot();
     void SetLogy(bool log) {logy = log;}
     void Draw();
     void FillHistograms();
     void DrawHistograms();
		 void GetOutliers();
};

// ClassImp(TMulPlot);

TMulPlot::TMulPlot() :
	TRunBase()
{
	out_name = "mulplot";
	// dir = "/adaqfs/home/apar/PREX/prompt/results/";
	// pattern = "prexPrompt_xxxx_???_regress_postpan.root";
	// tree = "reg";
	// cut = "ok_cut";
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

	for (string var : fCustoms)	{		// merge customs into solos/vars
		fSolos.push_back(var);
		fVars.insert(var);
	}

	FillHistograms();
  DrawHistograms();
}

void TMulPlot::FillHistograms() {
	map<string, double> up;
	for (string var : fVars) {
		fVarUnit[var] = GetUnit(var);
		fVarMax[var] /= UNITS[fVarUnit[var]];
    long max = ceil(fVarMax[var]);
    int power = floor(log(max)/log(10));
    int  a = max*10 / pow(10, power);
    up[var] = (a+1) * pow(10, power) / 10.;

		// default sign correction
		int iok = 0;
		for (int run : fRuns) {
      int s = fRunSign[run] > 0 ? 1 : (fRunSign[run] < 0 ? -1 : 0);
      const size_t sessions = fRootFile[run].size();
      for (size_t session=0; session < sessions; session++) {
        for (int i = 0; i < fEntryNumber[run][session].size(); i++, iok++)
        fVarValue[var][iok] *= s;
      }
		}
	}

  // initialize histogram
	for (string var : fVars) {
		double unit = UNITS[fVarUnit[var]];
    for (int i=0; i<nOk; i++)
      fVarValue[var][i] /= unit;
	}

  for (string solo : fSolos) {
		double unit = UNITS[fVarUnit[solo]];
		double high = up[solo];
		double low  = -high;
    if (fSoloCut[solo].low != 1024)
      low = fSoloCut[solo].low/unit;
    if (fSoloCut[solo].high != 1024)
      high = fSoloCut[solo].high/unit;
    
    fSoloHists[solo] = new TH1F(solo.c_str(), Form("%s;%s", solo.c_str(), fVarUnit[solo]), 100, low, high);
    for (int i=0; i<nOk; i++)
      fSoloHists[solo]->Fill(fVarValue[solo][i]);
  }

  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
		double unit = UNITS[fVarUnit[var1]];
		double high = (up[var1] > up[var2] ? up[var1] : up[var2]) * 1.05;
		double low  = -high;
    if (fCompCut[comp].low != 1024)
      low = fCompCut[comp].low/unit;
    if (fCompCut[comp].high != 1024)
      high = fCompCut[comp].high/unit;

    size_t h = hash<string>{}(var1+var2);
    fCompHists[comp].first  = new TH1F(Form("%s_%ld", var1.c_str(), h), Form("%s;%s", var1.c_str(), fVarUnit[var1]), 100, low, high);
    fCompHists[comp].second = new TH1F(Form("%s_%ld", var2.c_str(), h), Form("%s;%s", var2.c_str(), fVarUnit[var2]), 100, low, high);

    for (int i=0; i<nOk; i++) {
      fCompHists[comp].first->Fill(fVarValue[var1][i]);
      fCompHists[comp].second->Fill(fVarValue[var2][i]);
    }
  }

	for (pair<string, string> cor : fCors) {
    string xvar = cor.second;
    string yvar = cor.first;
		double xunit = UNITS[fVarUnit[xvar]];
		double yunit = UNITS[fVarUnit[yvar]];
		double xhigh = up[xvar] * 1.05;
		double xlow = -xhigh;
		double yhigh = up[yvar] * 1.05;
		double ylow = -yhigh;

    // if (fCorCut[cor].low != 1024)
    //   min = fCorCut[cor].low/UNITS[unit];
    // if (fCompCut[comp].high != 1024)
    //   min = fCompCut[comp].high/UNITS[unit];

    fCorHists[cor] = new TH2F((yvar + xvar).c_str(), 
				Form("%s vs %s; %s/%s; %s/%s", yvar.c_str(), xvar.c_str(), xvar.c_str(), fVarUnit[xvar], yvar.c_str(), fVarUnit[yvar]),
				100, xlow, xhigh,
				100, ylow, yhigh);

    for (int i=0; i<nOk; i++)
      fCorHists[cor]->Fill(fVarValue[xvar][i], fVarValue[yvar][i]);
	}
}

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
        const size_t sessions = fRootFile[run].size();
        for (size_t session=0; session < sessions; session++) {
          for (int i = 0; i < fEntryNumber[run][session].size(); i++, iok++) {
            double val = fVarValue[solo][iok];
            if ((lout && val < lval) || (hout && val > hval)) {
              cout << ALERT << "Outlier in run: " << run << "\tvariable: " << solo << "\tvalue: " << val << ENDL;
            }
          }
				}
			}
		}
	}
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
