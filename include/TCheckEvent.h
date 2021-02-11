#ifndef TCHECKRUN_H
#define TCHECKRUN_H

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <glob.h>
#include <libgen.h>

#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TEntryList.h"
#include "TCut.h"
#include "TCollection.h"
#include "TGraphErrors.h"
#include "TH1F.h"
#include "TPaveStats.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TAxis.h"
#include "TText.h"
#include "TLine.h"

#include "rcdb.h"
#include "const.h"
#include "line.h"
#include "TConfig.h"
#include "TRSbase.h"
#include "draw.h"

using namespace std;

class TCheckEvent : public TRSbase {

    // ClassDe (TCheckEvent, 0) // check statistics

  private:
		// const char *mCut;
    // vector<TCut> allCuts; // all cuts with mCut begin the first, highlight cuts follow
		vector<long> fEntryNumber_buf;

    map<string, set<int>>									fSoloBadPoints;
    map<string, set<int>>									fCustomBadPoints;
    map<pair<string, string>, set<int>>	  fCompBadPoints;
    map<pair<string, string>, set<int>>	  fCorBadPoints;

  public:
     TCheckEvent();
     ~TCheckEvent();
		 void GetConfig(const TConfig fConf);
		 void ProcessValues();
     void CheckValues();
     void Draw();
     void DrawSolos();
     // void DrawCustoms();
     void DrawComps();
     void DrawCors();
};

// ClassImp(TCheckEvent);

TCheckEvent::TCheckEvent() :
	TRSbase()
{
	if (!out_name)
		out_name = "checkruns";
	// program = checkruns;
	// dir = "/chafs2/work1/apar/japanOutput/";
	// pattern = "prexPrompt_pass2_xxxx.???.root";
	// tree = "evt";
}

void TCheckEvent::GetConfig(const TConfig fConf) {
	TBase::GetConfig(fConf);
	/*
	mCut = tcut;
	allCuts.push_back(mCut);
	vector<const char *> hCuts = fConf.GetHighlightCut();
	for (const char *c : hCuts) {
		allCuts.push_back(c);
		Node *node = ParseExpression(c);
		for (string var : GetVariables(node))
			fCutVars.insert(var);
		DeleteNode(node);
	}
	*/
}

TCheckEvent::~TCheckEvent() {
  cerr << INFO << "Release TCheckEvent" << ENDL;
}

void TCheckEvent::ProcessValues() {
	set<string> vars;
	map<string, double> inUnit, outUnit;
	for (string var : fVars)
		vars.insert(var);
	for (string var : fCustoms)
		vars.insert(var);
	for (string var : vars)
	{
		inUnit[var] = UNITS[GetInUnit(fVarName[var].first, fVarName[var].second)];
		outUnit[var] = UNITS[GetOutUnit(fVarName[var].first, fVarName[var].second)];
	}
	for (int run : fRuns)
		for (int s=0; s<fRootFile[run].size(); s++)
		{
			const int n = fEntryNumber[run][s].size();
			for (int i=0; i<n; i++)
			{
				fEntryNumber_buf.push_back(fEntryNumber[run][s][i]);
				for (string var : vars)
					fVarValue[var][i] *= inUnit[var]/outUnit[var];
			}
		}
}

void TCheckEvent::CheckValues() {
	set<string> vars;
	for (string var : fSolos)
		vars.insert(var);
	for (string var : fCustoms)
		vars.insert(var); // assumes (and should be) no same name between solos and customs
	for (string solo : vars) {
		double sum = 0, sum2 = 0;
		double mean = 0, sigma = 0;
		long discontinuity = 120;
		long pre_entry = 0;
		long entry = 0;
		double val = 0;

		// first loop: find out the mean value of largest consecutive events segments 
		// (with large 1s = 120 events discontinuity)
		long start_entry = fEntryNumber_buf[0];
		pre_entry = start_entry;
		double length = 1;  // length of consecutive segments
		for (int i=0; i<nOk; i++) {
			entry = fEntryNumber_buf[i];
			val = fVarValue.at(solo)[i];

			if (entry - pre_entry > discontinuity || (sigma != 0 && abs(val - mean) > 10*sigma)) {  // end previous segment, start a new segment  
				if (pre_entry - start_entry > length) {
					length = pre_entry - start_entry + 1;
					mean = sum/length;  // length initial value can't be 0
					sigma = sqrt(sum2/length - mean*mean);
				}
				start_entry = entry;
				sum = 0;
				sum2 = 0;
			}

			sum += val;
			sum2 += val*val;
			pre_entry = entry;
		}
		if (pre_entry - start_entry > length) {
			length = pre_entry - start_entry + 1;
			mean = sum/length;  // length initial value can't be 0
			sigma = sqrt(sum2/length - mean*mean);
		}
		cout << INFO << "variable: " << solo << "\t start entry: " << start_entry 
				 << "\t end_enry: " << pre_entry << "\t length: " << length 
				 << "\tmean: " << mean << "\tsigma: " << sigma << ENDL;

		double low_cut  = fSoloCut[solo].low;
		double high_cut = fSoloCut[solo].high;
		double burp_cut = fSoloCut[solo].burplevel;
		// if (low_cut == 1024 && high_cut != 1024)
		//   low_cut = high_cut;
		// else if (high_cut == 1024 && low_cut != 1024)
		//   high_cut = low_cut;

		double unit = UNITS[GetInUnit(solo)];
		if (low_cut != 1024) {
			low_cut /= unit;
			low_cut = mean - low_cut;
		}
		if (high_cut != 1024) {
			high_cut /= unit;
			high_cut = mean + high_cut;
		}
		if (burp_cut != 1024)
			burp_cut /= unit;
		else 
			burp_cut = 10*sigma;

		pre_entry = fEntryNumber_buf[0];
		const int burp_length = 120;  // compare with the average value of previous 120 events
		double burp_ring[burp_length] = {0};
		int burp_index = 0;
		int burp_events = 0;  // how many events in the burp ring now
		mean = fVarValue[solo][0];
		sum = 0;
		bool outlier = false;
		long start_outlier;
		bool has_outlier = false;
		bool has_glitch = false;
		for (int i=0; i<nOk; i++) {
			entry = fEntryNumber_buf[i];
			val = fVarValue[solo][i];

			if (   (low_cut  != 1024 && val < low_cut)
					|| (high_cut != 1024 && val > high_cut) ) {  // outlier
				if (!outlier) {
					start_outlier = entry;
					outlier = true;
					has_outlier = true;
				}
				fSoloBadPoints[solo].insert(fEntryNumber_buf[i]);
			} else {
				if (outlier) {
					cerr << OUTLIER << "in variable: " << solo << "\tfrom entry: " << start_outlier << " to entry: " << fEntryNumber_buf[i-1] << ENDL;
					// FIXME should I add run info in the OUTLIER output?
					outlier = false;
				}
			}

			if (entry - pre_entry > discontinuity) {  // start a new count
				burp_events = 0;
				sum = 0;
				mean = val;
			}

			if (abs(val - mean) > burp_cut) {
				cerr << GLITCH << "glitch in variable: " << solo << " in entry " << entry << "\tmean: " << mean << "\tvalue: " << val << ENDL;
				has_glitch = true;
				fSoloBadPoints[solo].insert(fEntryNumber_buf[i]);
			}

			burp_ring[burp_index] = val;
			burp_index++;
			burp_index %= burp_length;
			if (burp_events == burp_length) {
				sum -= burp_ring[burp_index];
			} else {
				burp_events++;
			}
			sum += val;
			mean = sum/burp_events;
			pre_entry = entry;
		}
		if (outlier)
			cerr << OUTLIER << "in variable: " << solo << "\tfrom entry: " << start_outlier << " to entry: " << fEntryNumber_buf[nOk-1] << ENDL;
	}

	for (pair<string, string> comp : fComps) {
		string var1 = comp.first;
		string var2 = comp.second;
		double low_cut  = fCompCut[comp].low;
		double high_cut = fCompCut[comp].high;
		double burp_cut = fCompCut[comp].burplevel;
		double unit = UNITS[GetInUnit(var1)];
		if (low_cut != 1024)	low_cut /= unit;
		if (high_cut != 1024) high_cut /= unit;
		if (burp_cut != 1024) burp_cut /= unit;

		bool outlier = false;
		long start_outlier;
		bool has_outlier = false;
		for (int i=0; i<nOk; i++) {
			double val1, val2;
			val1 = fVarValue[var1][i];
			val2 = fVarValue[var2][i];
			double diff = abs(val1 - val2);

			if (  (low_cut  != 1024 && diff < low_cut)
				 || (high_cut != 1024 && diff > high_cut) ) {
				// || (stat != 1024 && abs(diff-mean) > stat*sigma
				if (!outlier) {
					start_outlier = fEntryNumber_buf[i];
					outlier = true;
					has_outlier = true;
				}
				fCompBadPoints[comp].insert(fEntryNumber_buf[i]);
			} else {
				if (outlier) {
					cerr << OUTLIER << "in variable: " << var1 << " vs " << var2 << "\tfrom entry: " << start_outlier << " to entry: " << fEntryNumber_buf[i-1] << ENDL;
					// FIXME should I add run info in the OUTLIER output?
					outlier = false;
				}
			}
		}
		if (outlier)
			cerr << OUTLIER << "in variable: " << var1 << " vs " << var2 << "\tfrom entry: " << start_outlier << " to entry: " << fEntryNumber_buf[nOk-1] << ENDL;
	}

	/*
	for (pair<string, string> cor : fCors) {
		string yvar = cor.first;
		string xvar = cor.second;
		const double low_cut  = fCorCut[cor].low;
		const double high_cut = fCorCut[cor].high;
		const double burp_cut = fCompCut[cor].burplevel;
		for (int i=0; i<nOk; i++) {
			double xval, yval;
			xval = values.at(xvar)[i];
			yval = values.at(yvar)[i];

			if ( 1 ) {	// FIXME
				cout << ALERT << "bad datapoint in Cor: " << yvar << " vs " << xvar << ENDL;
				fCorBadPoints[*it].insert(i);
			}
		}
	}
	*/

	cout << INFO << "done with checking values" << ENDL;
}

void TCheckEvent::Draw() {
  c = new TCanvas("c", "c", 1800, 600);
  c->SetGridy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(1110);
  gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  gStyle->SetBarWidth(1.05);

  if (format == pdf)
    c->Print(Form("%s.pdf[", out_name));

	// for (TCut cut : allCuts)
	// {
	//	 SetTreeCut(cut);
		GetValues();
		ProcessValues();
		CheckValues();
	// }
  DrawSolos();
  // DrawComps();
  // DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << INFO << "done with drawing plots" << ENDL;
}

void TCheckEvent::DrawSolos() {
  vector<string> vars;
  for (string var : fSolos)
    vars.push_back(var);
  for (string var : fCustoms)
    vars.push_back(var); // assumes (and should be) no same name between solos and customs
  for (string var : vars) {
    string unit = GetOutUnit(var);

		// draw for the main cut
    TGraphErrors * g = new TGraphErrors();      // all data points
    // TGraphErrors * g_err = new TGraphErrors();  // ErrorFlag != 0
    // TGraphErrors * g_bad = new TGraphErrors();  // ok data points that don't pass check

		for(int i=0, ibad=0; i<nOk; i++) {
			double val;
			val = fVarValue[var][i];

			// g->SetPoint(i, i+1, val);

			// g_err->SetPoint(ierr, i+1, val);
			// ierr++;

			g->SetPoint(i, fEntryNumber_buf[i], val);
			// if (fSoloBadPoints[var].find(i) != fSoloBadPoints[var].cend()) {
			//   g_bad->SetPoint(ibad, fEntryNumber_buf[i], val);
			//   ibad++;
			// }
		}
    g->SetTitle((var + ";;" + unit).c_str());
    // g_err->SetMarkerStyle(1.2);
    // g_err->SetMarkerColor(kBlue);
    // g_bad->SetMarkerStyle(1.2);
    // g_bad->SetMarkerSize(1.5);
    // g_bad->SetMarkerColor(kRed);

    c->cd();
    g->Draw("AP");
    // g_err->Draw("P same");
    // g_bad->Draw("P same");

    TAxis * ay = g->GetYaxis();
    double ymin = ay->GetXmin();
    double ymax = ay->GetXmax();

    if (nRuns > 1) {
      for (int run : fRuns) {
				const int s = fRootFile[run].size();
        TLine *l = new TLine(fEntries[run][s-1], ymin, fEntries[run][s-1], ymax);
        l->SetLineStyle(2);
        l->SetLineColor(kRed);
        l->Draw("same");
        TText *t = new TText(fEntries[run][s-1]-nTotal/(2*nRuns), ymin+(ymax-ymin)/30*(run%5 + 1), Form("%d", run));
        t->SetTextSize((t->GetTextSize())/(nRuns/7+1));
        t->SetTextColor(kRed);
        t->Draw("same");
      }
    }

    c->Modified();

		c->Update();
		c->Modified();

    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, var.c_str()));

    c->Clear();
  }
  cout << INFO << "Done with drawing Solos and customized variables." << ENDL;
}

// it looks like not a good idea to draw diff plots with a few hundred thousands points
void TCheckEvent::DrawComps() {
  int MarkerStyles[] = {29, 33, 34, 31};
  for (pair<string, string> var : fComps) {
    string var1 = var.first;
    string var2 = var.second;

    string unit = GetOutUnit(var1);

    TGraphErrors *g1 = new TGraphErrors();
    TGraphErrors *g2 = new TGraphErrors();
    TGraphErrors *g_err1 = new TGraphErrors();
    TGraphErrors *g_err2 = new TGraphErrors();
    TGraphErrors *g_bad1 = new TGraphErrors();
    TGraphErrors *g_bad2 = new TGraphErrors();
    TH1F * h_diff = new TH1F("diff", "", nOk, 0, nOk);

    double ymin = fVarMin[var1] < fVarMin[var2] ? fVarMin[var1] : fVarMin[var2];
    double ymax = fVarMax[var1] < fVarMax[var2] ? fVarMax[var1] : fVarMax[var2];
		for(int i=0, ibad=0; i<nOk; i++) {
			double val1;
			double val2;
			val1 = fVarValue[var1][i];
			val2 = fVarValue[var2][i];

			g1->SetPoint(i, fEntryNumber_buf[i], val1);
			g2->SetPoint(i, fEntryNumber_buf[i], val2);
			h_diff->SetBinContent(i+1, val1-val2);
			
			//  g_err1->SetPoint(ierr, i+1, val1);
			//  g_err2->SetPoint(ierr, i+1, val2);
			//  ierr++;
			if (fCompBadPoints[var].find(i) != fCompBadPoints[var].cend()) {
				g_bad1->SetPoint(ibad, fEntryNumber_buf[i], val1);
				g_bad2->SetPoint(ibad, fEntryNumber_buf[i], val2);
				ibad++;
			}
		}
    double margin = (ymax-ymin)/10;
    ymin -= margin;
    ymax += margin;

		g1->SetTitle(Form("%s & %s;;%s", var1.c_str(), var2.c_str(), unit.c_str()));
		g1->SetMarkerColor(8);
		g2->SetMarkerColor(9);
    g_err1->SetMarkerStyle(1.2);
    g_err1->SetMarkerColor(kBlue);
    g_err2->SetMarkerStyle(1.2);
    g_err2->SetMarkerColor(kBlue);
    g_bad1->SetMarkerStyle(1.2);
    g_bad1->SetMarkerSize(1.5);
    g_bad1->SetMarkerColor(kRed);
    g_bad2->SetMarkerStyle(1.2);
    g_bad2->SetMarkerSize(1.5);
    g_bad2->SetMarkerColor(kRed);

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
    g1->Draw("AP");
    g2->Draw("P same");
    g1->GetYaxis()->SetRangeUser(ymin, ymax+(ymax-ymin)/5);

    g_bad1->Draw("P same");
    g_bad2->Draw("P same");
    g_err1->Draw("P same");
    g_err2->Draw("P same");
    
    p2->cd();
    h_diff->SetStats(kFALSE);
    h_diff->SetFillColor(kGreen);
    h_diff->SetBarOffset(0.5);
    h_diff->SetBarWidth(1);
    h_diff->Draw("B");
    p2->Update();

    if (nRuns > 1) {
      p1->cd();
      for (int run : fRuns) {
				const int s = fRootFile[run].size();
        TLine *l = new TLine(fEntries[run][s-1], ymin, fEntries[run][s-1], ymax);
        l->SetLineStyle(2);
        l->SetLineColor(kRed);
        l->Draw("same");
        TText *t = new TText(fEntries[run][s-1]-nTotal/(5*nRuns), ymin+(ymax-ymin)/30, Form("%d", run));
        t->Draw("same");
      }
    }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s-%s.png", out_name, var1.c_str(), var2.c_str()));
    c->Clear();
    h_diff->Delete();
    h_diff = NULL;
  }
  cout << INFO << "Done with drawing Comparisons." << ENDL;
}

void TCheckEvent::DrawCors() {
  for (pair<string, string> var : fCors) {
    string xvar = var.second;
    string yvar = var.first;
    string xunit = GetOutUnit(xvar);
    string yunit = GetOutUnit(yvar);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    // TGraphErrors * g_good = new TGraphErrors();

    for(int i=0, ibad=0; i<nOk; i++) {
      double xval;
      double yval;
			xval = fVarValue[xvar][i];
			yval = fVarValue[yvar][i];

      g->SetPoint(i, xval, yval);
      
      // g_good->SetPoint(i, xval, yval);
      if (fCorBadPoints[var].find(i) != fCorBadPoints[var].cend()) {
        g_bad->SetPoint(ibad, xval, yval);
        ibad++;
      }
    }
		g->SetTitle((yvar + " vs " + xvar + ";" + xunit + ";" + yunit).c_str());
    g->SetMarkerStyle(1.2);
    g->SetMarkerColor(kBlue);
    // g_good->SetMarkerColor(kBlack);
    g_bad->SetMarkerStyle(1.2);
    g_bad->SetMarkerColor(kRed);

    // g_good->Fit("pol1");

    // TPaveStats * st;
    c->cd();
    gPad->SetRightMargin(0.05);
    g->Draw("AP");
    gPad->Update();
    // st = (TPaveStats *) g->FindObject("stats");
    // st->SetName("g_stats");
    // st->SetX2NDC(0.95); 
    // st->SetX1NDC(0.95-0.3); 
    // st->SetY2NDC(0.9);
    // st->SetY1NDC(0.75);

    // g_good->Draw("P same");
    g_bad->Draw("P same");
    
    // TAxis * ay = g->GetYaxis();
    // double min = ay->GetXmin();
    // double max = ay->GetXmax();
    // ay->SetRangeUser(min, max+(max-min)/9);
    // gPad->Update();

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s_vs_%s.png", out_name, xvar.c_str(), yvar.c_str()));
    c->Clear();
  }
  cout << INFO << "Done with drawing Correlations." << ENDL;
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
