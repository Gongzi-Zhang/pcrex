#ifndef TCHECKMINI_H
#define TCHECKMINI_H

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
#include "TRSbase.h"
#include "draw.h"

using namespace std;

class TCheckMini : public TRSbase {

    // ClassDe (TCheckMini, 0) // check statistics

  private:
    bool  sign = false;
    map<string, const char *> fVarInUnit;
    map<string, const char *> fVarOutUnit;

    vector<pair<int, int>> fMiniruns;
		int nMiniruns;

  public:
     TCheckMini();
     ~TCheckMini();
     void SetSign() {sign = true;}
		 void ProcessValues();
     void CheckValues();
     void Draw();
     void DrawSolos();
     void DrawComps();
     void DrawCors();
};

// ClassImp(TCheckMini);

TCheckMini::TCheckMini() :
  TRSbase()
{
	if (!out_name)
		out_name	= "checkmini";
	// dir       = "/chafs2/work1/apar/postpan-outputs/";
	// pattern   = "prexPrompt_xxxx_???_regress_postpan.root"; 
	// tree      = "mini";
}

TCheckMini::~TCheckMini() {
  cout << INFO << "Release TCheckMini" << ENDL;
}

void TCheckMini::ProcessValues() {
  GetRunInfo();
	for (map<int, int>::const_iterator it = fRunSign.cbegin(); it!=fRunSign.cend(); it++) {
		if (find(flips.cbegin(), flips.cend(), it->second) == flips.cend())
			flips.push_back(it->second);
	}

	nMiniruns = nOk;
  for (int run : fRuns) {
    const size_t sessions = fRootFile[run].size();
    for (size_t session=0; session < sessions; session++) {
      for (int m=0; m<fEntryNumber[run][session].size(); m++)
        fMiniruns.push_back(make_pair(run, fEntryNumber[run][session][m]));
    }
  }
	for (string var : fVars) {
		// unit correction
    fVarInUnit[var] = GetInUnit(fVarName[var].first, fVarName[var].second);
    fVarOutUnit[var] = GetOutUnit(fVarName[var].first, fVarName[var].second);
		for (int m=0; m<nMiniruns; m++) {
			fVarValue[var][m] *= (UNITS[fVarInUnit[var]]/UNITS[fVarOutUnit[var]]);
		}

		// sign correction: only for mean value
		if (sign && fVarName[var].second == "mean" 
				&& (var.find("asym") != string::npos ^ var.find("diff") != string::npos)
				// slope don't need sign correction
				)
		{
      int m = 0;
			for (int run : fRuns) {
        const size_t sessions = fRootFile[run].size();
        for (size_t session=0; session < sessions; session++) {
          for (int i = 0; i < fEntryNumber[run][session].size(); i++, m++)
            fVarValue[var][m] *= fRunSign[run] > 0 ? 1 : (fRunSign[run] < 0 ? -1 : 0);
        }
      }
		}
	}
  GetCustomValues();
  for (string var : fCustoms) {
    fVarInUnit[var] = GetInUnit(fVarName[var].first, fVarName[var].second);
    fVarOutUnit[var] = GetOutUnit(fVarName[var].first, fVarName[var].second);
    // looks like no need for unit correction for custom, should be taken care of in the expression
		// for (int m=0; m<nMiniruns; m++) {
		// 	fVarValue[var][m] *= (UNITS[fVarInUnit[var]]/UNITS[fVarOutUnit[var]]);
		// }
  }
}

void TCheckMini::CheckValues() {  // looks like Chicken ribs
  /*
  for (string solo : fSolos) {
    double low_cut  = fSoloCut[solo].low;
    double high_cut = fSoloCut[solo].high;
    double burp_cut = fSoloCut[solo].burplevel;
    double unit = UNITS[fVarOutUnit[solo]];
    if (low_cut != 1024)
      low_cut /= unit;
    if (high_cut != 1024)
      high_cut /= unit;
    if (burp_cut != 1024)
      burp_cut /= unit;

    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean, sigma = 0;
    for (int i=0; i<nMiniruns; i++) {
      double val = fVarValue[solo][i];

      if (i == 0) {
        mean = val;
        sigma = 0;
      }

      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (burp_cut != 1024 && abs(val-mean) > burp_cut)) {
        cout << ALERT << "bad datapoint in " << solo
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << ENDL;
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
    double low_cut  = fCompCut[comp].low;
    double high_cut = fCompCut[comp].high;
    double unit = UNITS[GetOutUnit(var1)];
    if (low_cut != 1024)
      low_cut /= unit;
    if (high_cut != 1024)
      high_cut /= unit;

    for (int i=0; i<nMiniruns; i++) {
      double val1 = fVarValue[var1][i];
      double val2 = fVarValue[var1][i];
			double diff = abs(val1 - val2);

      if ( (low_cut  != 1024 && diff < low_cut)
        || (high_cut != 1024 && diff > high_cut)) {
        cout << ALERT << "bad datapoint in Comp: " << var1 << " vs " << var2 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << ENDL;
      }
    }
  }

  for (pair<string, string> cor : fCors) {
    string yvar = cor.first;
    string xvar = cor.second;
    double low_cut   = fCorCut[cor].low;
    double high_cut  = fCorCut[cor].high;
    double xunit = UNITS[GetOutUnit(xvar)];
    double yunit = UNITS[GetOutUnit(yvar)];
    if (low_cut != 1024)
      low_cut /= (yunit/xunit);
    if (high_cut != 1024)
      high_cut /= (yunit/xunit);
    // const double 
    for (int i=0; i<nMiniruns; i++) {
      double xval = fVarValue[xvar][i];
      double yval = fVarValue[yvar][i];
    }
  }
*/
  cout << INFO << "done with checking values" << ENDL;
}

void TCheckMini::Draw() {
	// make sure get err for mean values
	for (string var : fVars) {
		if (var.find(".mean") != string::npos) {
			fVars.insert(var.substr(0, var.find(".mean")) + ".err");	// add new elements while looping the set
		}
	}

	CheckRuns();
	CheckVars();
	GetValues();
	ProcessValues();
	// CheckValues();

	if (nMiniruns > 100)
		c = new TCanvas("c", "c", 1800, 600);
	else 
		c = new TCanvas("c", "c", 1200, 900);
  c->SetGridy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(1110);
  gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  gStyle->SetTitleSize(0.07, "Y");
  gStyle->SetTitleYOffset(0.55);

  if (format == pdf)
    c->Print(Form("%s.pdf[", out_name));

  DrawSolos();
  DrawComps();
  DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << INFO << "done with drawing plots" << ENDL;
}

void TCheckMini::DrawSolos() {
  for (string var : fCustoms)
    fSolos.push_back(var);
  for (string solo : fSolos) {
		string branch = fVarName[solo].first;
		string leaf = fVarName[solo].second;
    string err_var;
		bool mean = leaf == "mean" ? true : false;
    if (mean)
      err_var = branch + ".err";

    TH1F *h = new TH1F(solo.c_str(), "", nMiniruns, 0.5, nMiniruns+0.5);
    TGraphErrors *g = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

    for(int i=0; i<nMiniruns; i++) {
      double val, err=0;
      val = fVarValue[solo][i];
			if (std::isnan(val))
				continue;
      if (mean) 
        err = fVarValue[err_var][i];

      h->SetBinContent(i+1, val);
			int ipoint = g->GetN();
      g->SetPoint(ipoint, i+1, val);
      g->SetPointError(ipoint, 0, err);

      ipoint = g_flips[fRunSign[fMiniruns[i].first]]->GetN();
      g_flips[fRunSign[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val);
      g_flips[fRunSign[fMiniruns[i].first]]->SetPointError(ipoint, 0, err);
    }

    // style
    double min = g->GetYaxis()->GetXmin();
    double max = g->GetYaxis()->GetXmax();
    h->GetYaxis()->SetRangeUser(min, max+(max-min)/10);
    h->GetXaxis()->SetNdivisions(nMiniruns, 0, 0, kTRUE);
    h->SetStats(kFALSE);
		string title = solo;
		if (count(title.begin(), title.end(), '.') == 2)
			title = title.substr(title.find('.')+1);
    if (sign)
      title += " (sign corrected)";
    h->SetTitle((title + ";;" + fVarOutUnit[solo]).c_str());

    if (flips.size() == 1)
      g->SetMarkerStyle(20);
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]]->SetMarkerStyle(mstyles[i]);
      g_flips[flips[i]]->SetMarkerColor(mcolors[i]);
      if (flips.size() > 1) {
        g_flips[flips[i]]->Fit("pol0");
        g_flips[flips[i]]->GetFunction("pol0")->SetLineColor(mcolors[i]);
        g_flips[flips[i]]->GetFunction("pol0")->SetLineWidth(1);
      }
    }

    g->Fit("pol0");
    TF1 * fit= g->GetFunction("pol0");
    double mean_value = fit->GetParameter(0);
		cout << OUTPUT << solo << "\t" << mean_value << " ± " << fit->GetParError(0) << ENDL;

    TAxis *ax = h->GetXaxis();
    TH1F *pull = NULL; 
    if (mean) {
      pull = new TH1F("pull", "", nMiniruns, 0.5, nMiniruns+0.5);
      for (int i=0; i<nMiniruns; i++) {
        double ratio = 0;
        if (fVarValue[err_var][i]!= 0 && !std::isnan(fVarValue[solo][i]))
          ratio = (fVarValue[solo][i]-mean_value)/fVarValue[err_var][i];

        pull->Fill(i+1, ratio);
      }
      pull->GetXaxis()->SetNdivisions(nMiniruns, 0, 0, kTRUE);
      ax = pull->GetXaxis();
      ax->SetLabelSize(0.06);
      pull->GetYaxis()->SetLabelSize(0.12);
      h->GetYaxis()->SetLabelSize(0.08);
    }
    for (int i=0; i<nMiniruns; i++) {
      ax->SetBinLabel(i+1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    }
    // ax->SetTitle("Minirun");

    TLegend *l = new TLegend(0.1, 0.9-0.05*flips.size(), 0.25, 0.9);
    TPaveStats *st;
    map<int, TPaveStats *> sts;
    TPad *p1;
    TPad *p2;
    c->cd();
    if (mean) {
      p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
      p1->SetBottomMargin(0);
      p1->SetRightMargin(0.05);
      // p1->SetLeftMargin(0.08);
      p1->Draw();
      p1->SetGridy();

      p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
      p2->SetTopMargin(0);
      p2->SetBottomMargin(0.17);
      // p2->SetLeftMargin(0.08);
      p2->SetRightMargin(0.05);
      p2->Draw();
      p2->SetGrid();
    } else {
      p1 = new TPad("p1", "p1", 0.0, 0.0, 1.0, 1.0);
      p1->SetBottomMargin(0.16);
      // p1->SetLeftMargin(0.08);
      p1->SetRightMargin(0.05);
      p1->Draw();
      p1->SetGridy();
    }

    p1->cd();
    h->Draw("*");
    g->Draw("P SAME");
    p1->Update();
    ((TPaveText *)p1->GetPrimitive("title"))->SetTextSize(0.08);
    st = (TPaveStats *) g->FindObject("stats");
    st->SetName("g_stats");
    double width = 0.7/(flips.size() + 1);
    st->SetX2NDC(0.95); 
    st->SetX1NDC(0.95-width); 
    st->SetY2NDC(0.9);
    st->SetY1NDC(0.8);

    if (flips.size()>1) {
      for (int i=0; i<flips.size(); i++) {
        g_flips[flips[i]]->Draw("P same");
        gPad->Update();
        if (flips.size() > 1) {
          sts[flips[i]] = (TPaveStats *) g_flips[flips[i]]->FindObject("stats");
          sts[flips[i]]->SetName(legends[flips[i]]);
          sts[flips[i]]->SetX2NDC(0.25+width*i);
          sts[flips[i]]->SetX1NDC(0.25+width*(i+1));
          sts[flips[i]]->SetY2NDC(0.9);
          sts[flips[i]]->SetY1NDC(0.8);
          sts[flips[i]]->SetTextColor(mcolors[i]);
        }
        l->AddEntry(g_flips[flips[i]], legends[flips[i]], "lep");
      }
      l->Draw();
    }

    p1->Modified();
    if (mean) {
      p2->cd();
      p2->SetGrid();
      pull->SetFillColor(kGreen);
      pull->SetLineColor(kGreen);
      pull->SetStats(0);
      pull->Draw("HIST");
      p2->Update();
    }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, solo.c_str()));

    c->Clear();
    if (pull) 
      pull->Delete();
    h->Delete();
    g->Delete();
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]]->Delete();
    }
  }
  cout << INFO << "Done with drawing Solos." << ENDL;
}

void TCheckMini::DrawComps() {
  for (pair<string, string> comp : fComps) {
    string var[2] = {comp.first, comp.second};
    string branch[2];
    string name[2];

    bool mean = fVarName[var[0]].second == "mean" ? true : false;
    string err_var[2];

    TH1F *h = new TH1F("h", "", nMiniruns, 0.5, nMiniruns+0.5);
    TGraphErrors * g[2];
    TH1F * h_diff = new TH1F("diff", "", nMiniruns, 0.5, nMiniruns+0.5);
    
    double min, max;
    for (int i=0; i<2; i++) {
      branch[i] = fVarName[var[i]].first;
      name[i] = branch[i].substr(branch[i].find_last_of('_')+1);
      if (mean)
        err_var[i] = branch[i] + ".err";

      g[i] = new TGraphErrors();
      g[i]->SetMarkerStyle(mstyles[i]);
      g[i]->SetMarkerColor(mcolors[i]);
      for(int m=0; m<nMiniruns; m++) {
        double val, err=0;
        val = fVarValue[var[i]][m];
				if (std::isnan(val))
					continue;
        if (mean) 
          err = fVarValue[err_var[i]][m];
        if (m==0) 
          min = max = val;

        if ((val-err) < min) min = val-err;
        if ((val+err) > max) max = val+err;

        g[i]->SetPoint(m, m+1, val);
        g[i]->SetPointError(m, 0, err);
        h_diff->Fill(m+1, val*(1-2*i));
      }

      g[i]->Fit("pol0");
      g[i]->GetFunction("pol0")->SetLineColor(mcolors[i]);
    }
    double margin = (max-min)/10;
    min -= margin;
    max += margin;

    h->SetStats(kFALSE);
    h->GetYaxis()->SetRangeUser(min, max+(max-min)/5);
    h->GetYaxis()->SetLabelSize(0.08);
    h->GetXaxis()->SetNdivisions(nMiniruns, 0, 0, kTRUE);
    h_diff->GetXaxis()->SetNdivisions(nMiniruns, 0, 0, kTRUE);
    h_diff->GetYaxis()->SetLabelSize(0.12);

    string title = Form("#color[%d]{%s} & #color[%d]{%s}",
        mcolors[0], var[0].c_str(), mcolors[1], var[1].c_str());
    if (sign)
      title += " (sign corrected)";
    h->SetTitle(Form("%s;;%s", title.c_str(), fVarOutUnit[var[0]]));

    TPaveStats *st[2];
    c->cd();
    TPad * p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
    p1->Draw();
    p1->SetGridy();
    p1->SetBottomMargin(0);
    // p1->SetLeftMargin(0.08);
    p1->SetRightMargin(0.05);
    TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
    p2->Draw();
    p2->SetGrid();
    p2->SetTopMargin(0);
    p2->SetBottomMargin(0.17);
    // p2->SetLeftMargin(0.08);
    p2->SetRightMargin(0.05);

    p1->cd();
    h->Draw("*");
    p1->Update();
    ((TPaveText *)p1->GetPrimitive("title"))->SetTextSize(0.08);
    for (int i=0; i<2; i++) {
      g[i]->Draw("P SAME");
      p1->Update();
      st[i] = (TPaveStats *) g[i]->FindObject("stats");
      st[i]->SetName(Form("g%d_stats", i));
      st[i]->SetTextColor(mcolors[i]);
      st[i]->SetX1NDC(0.1 + 0.65*i);
      st[i]->SetX2NDC(0.3 + 0.65*i);
      st[i]->SetY1NDC(0.75);
      st[i]->SetY2NDC(0.9);
    }

    p2->cd();
    h_diff->SetStats(kFALSE);
    h_diff->SetFillColor(kGreen);
    h_diff->SetLineColor(kGreen);
    h_diff->Draw("HIST");
    TAxis *ax = h_diff->GetXaxis();
    for (int i=0; i<nMiniruns; i++)
      ax->SetBinLabel(i+1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    ax->SetLabelSize(0.06);
    p2->Update();

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s-%s.png", out_name, var[0].c_str(), var[1].c_str()));
    c->Clear();
    for (int i=0; i<2; i++) {
      g[i]->Delete();
    }
    h_diff->Delete();
    h_diff = NULL;
  }
  cout << INFO << "Done with drawing Comparisons." << ENDL;
}

void TCheckMini::DrawCors() {
  for (pair<string, string> cor : fCors) {
    string var[2] = {cor.first, cor.second};
    string branch[2];
    string name[2];
    bool mean[2];
    string err_var[2];
    TH1F *h[2];
    TGraphErrors * g[2];
    
    for (int i=0; i<2; i++) {
      branch[i] = fVarName[var[i]].first;
      name[i] = branch[i].substr(branch[i].find_last_of('_')+1);
      mean[i] = fVarName[var[i]].second == "mean" ? true : false;
      if (mean[i])
        err_var[i] = branch[i] + ".err";

      h[i] = new TH1F(Form("h%d", i), "", nMiniruns, 0.5, nMiniruns+0.5);
      g[i] = new TGraphErrors();
      g[i]->SetMarkerStyle(20);

      for(int m=0; m<nMiniruns; m++) {
        double val, err=0;
        val = fVarValue[var[i]][m];
				if (std::isnan(val))
					continue;
        if (mean[i]) 
          err = fVarValue[err_var[i]][m];

        g[i]->SetPoint(m, m+1, val);
        g[i]->SetPointError(m, 0, err);
      }

      g[i]->Fit("pol0");

      TAxis *ay = g[i]->GetYaxis();
      double min = ay->GetXmin();
      double max = ay->GetXmax();
      h[i]->GetYaxis()->SetRangeUser(min, max+(max-min)/10);
      h[i]->GetYaxis()->SetLabelSize(0.08);
      h[i]->SetStats(kFALSE);
      h[i]->GetXaxis()->SetNdivisions(nMiniruns, 0, 0, kTRUE);
    }

    string title = Form("%s vs %s", var[0].c_str(), var[1].c_str());
    if (sign)
      title += " (sign corrected)";
    h[0]->SetTitle(Form("%s;;%s", title.c_str(), fVarOutUnit[var[0]]));
    h[1]->SetTitle(Form(";;%s", fVarOutUnit[var[1]]));

    TPaveStats *st[2];
    c->cd();
    TPad *p[2];
    p[0] = new TPad("p1", "p1", 0.0, 0.53, 1.0, 1.0);
    p[0]->Draw();
    p[0]->SetGridy();
    p[0]->SetBottomMargin(0);
    // p[0]->SetLeftMargin(0.08);
    p[0]->SetRightMargin(0.05);
    p[1] = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.53);
    p[1]->Draw();
    p[1]->SetGridy();
    p[1]->SetTopMargin(0);
    p[1]->SetBottomMargin(0.2);
    // p[1]->SetLeftMargin(0.08);
    p[1]->SetRightMargin(0.05);

    for (int i=0; i<2; i++) {
      p[i]->cd();
      h[i]->Draw("*");
      g[i]->Draw("P SAME");
      p[i]->Update();
      st[i] = (TPaveStats *) g[i]->FindObject("stats");
      st[i]->SetName(Form("g%d_stats", i));
      st[i]->SetX1NDC(0.75);
      st[i]->SetX2NDC(0.95);
      st[i]->SetY1NDC(0.75+0.1*i);
      st[i]->SetY2NDC(0.9+0.1*i);
    }
    ((TPaveText *)p[0]->GetPrimitive("title"))->SetTextSize(0.08);

    TAxis *ax = h[1]->GetXaxis();
    for (int i=0; i<nMiniruns; i++)
      ax->SetBinLabel(i+1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
    ax->SetLabelSize(0.06);
    p[1]->Update();

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s-%s.png", out_name, var[0].c_str(), var[1].c_str()));
    c->Clear();
    for (int i=0; i<2; i++) {
      h[i]->Delete();
      g[i]->Delete();
    }
  }
  cout << INFO << "Done with drawing corarisons." << ENDL;
}
/*
void TCheckMini::DrawCors() {
  for (pair<string, string> cor : fCors) {
    string xvar = cor.second;
    string yvar = cor.first;
    string xbranch = fVarName[xvar].first;
    string ybranch = fVarName[yvar].first;

    bool xmeanflag = fVarName[xvar].first == "mean";
    bool ymeanflag = fVarName[yvar].first == "mean";
    const char * xerr_var = Form("%s.err", xbranch.c_str());
    const char * yerr_var = Form("%s.err", ybranch.c_str());

    TGraphErrors * g = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

    for(int i=0; i<nMiniruns; i++) {
      double xval, xerr=0;
      double yval, yerr=0;
      xval = fVarValue[xvar][i];
      yval = fVarValue[yvar][i];
      if (xmeanflag) {
        xerr = fVarValue[xerr_var][i];
      }
      if (ymeanflag) {
        yerr = fVarValue[yerr_var][i];
      }

      g->SetPoint(i, xval, yval);
      g->SetPointError(i, xerr, yerr);
      
      int ipoint = g_flips[fRunSign[fMiniruns[i].first]]->GetN();
      g_flips[fRunSign[fMiniruns[i].first]]->SetPoint(ipoint, xval, yval);
      g_flips[fRunSign[fMiniruns[i].first]]->SetPointError(ipoint, xerr, yerr);
    }
    // g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    if (sign)
      g->SetTitle((cor.first + " vs " + cor.second + " (sign corrected);" + fVarOutUnit[xvar] + ";" + fVarOutUnit[yvar]).c_str());
    else
      g->SetTitle((cor.first + " vs " + cor.second + ";" + fVarOutUnit[xvar] + ";" + fVarOutUnit[yvar]).c_str());
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
    ay->SetRangeUser(min, max+(max-min)/10);
    gPad->Update();

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s_vs_%s.png", out_name, xvar.c_str(), yvar.c_str()));
    c->Clear();
  }
  cout << INFO << "Done with drawing Correlations." << ENDL;
}
*/
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
