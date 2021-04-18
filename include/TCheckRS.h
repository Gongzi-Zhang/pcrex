#ifndef TCHECKRS_H
#define TCHECKRS_H

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

enum Ptype {slug, run, event, other};

class TCheckRS : public TRSbase {

    // ClassDe (TCheckRS, 0) // check statistics

  private:
    Ptype pt = slug;
		bool sign = false;
    map<int, int> fSign;
    map<string, string> fVarInUnit;
    map<string, string> fVarOutUnit;
    double xmin, xmax;
    // TCanvas *c;

  public:
     TCheckRS(const char *t);
     ~TCheckRS();
     // void GetConfig(const TConfig &fConf);
     // void CheckVars();
     void SetSign(bool s) { sign = s; }
		 void ProcessValues();
     void CheckValues();
     void Draw();
     void DrawSolos();
     void DrawComps();
     void DrawCors();
};
// ClassImp(TCheckRS);

TCheckRS::TCheckRS(const char *t) :
  TRSbase()
{
  if (strcmp(t, "slug") == 0) {
    pt = slug;
    granularity = "slug";
		if (!out_name)
			out_name	= "checkslug";
    dir       = "rootfiles/";
    pattern   = "agg_slug_xxxx.root"; 
    tree      = "slug";
  } else if (strcmp(t, "run") == 0) {
    pt = run;
    granularity = "run";
		if (!out_name)
			out_name	= "checkrun";
    dir       = "rootfiles/";
    pattern   = "agg_minirun_xxxx.root"; 
    tree      = "run";
  } 
}

TCheckRS::~TCheckRS() {
  cout << INFO << "Release TCheckRS" << ENDL;
}

/*
void TCheckRS::GetConfig(const TConfig &fConf) {
  TBase::GetConfig(fConf);
  for (string var : fVars) {
    if (var.find(".mean") != string::npos)
      fVars.insert(var.substr(0, var.find(".mean")) + ".err");
  }
}
*/

/*
void TCheckRS::CheckVars() {
  TBase::CheckVars();
  for (string var : fVars) {
    if (fVarName[var].second == "mean") {
      string errvar = fVarName[var].first + ".err";
      fVars.insert(errvar);
      fVarName[errvar] = make_pair(fVarName[var].first, "err");
    }
  }
}
*/

void TCheckRS::ProcessValues() {
	for (pair<int, int> p : fSign) {
		if (find(flips.begin(), flips.end(), p.second) == flips.end())
			flips.push_back(p.second);
	}
  for (string var : fVars) {
    fVarInUnit[var] = GetInUnit(fVarName[var].first, fVarName[var].second);
    fVarOutUnit[var] = GetOutUnit(fVarName[var].first, fVarName[var].second);
		double unit = UNITS[fVarInUnit[var]]/UNITS[fVarOutUnit[var]];
		bool sflag = sign 
								 && (var.find("asym") != string::npos ^ var.find("diff") != string::npos)  
								 && fVarName[var].second == "mean"; // sign correction, only for mean value

    int i = 0;
    for (int g : fGrans) {
      const int sessions = fRootFile[g].size();
      for (int s=0; s<sessions; s++) {
        if (sflag) 
          fVarValue[var][i] *= fSign[g] > 0 ? 1 : (fSign[g] < 0 ? -1 : 0);
        fVarValue[var][i] *= unit;
        i++;  
      }
    }
	}
  GetCustomValues();
  set<string> vars = fVars;
  for (string var : fCustoms) {
    fVarInUnit[var] = GetInUnit(fVarName[var].first, fVarName[var].second);
    fVarOutUnit[var] = GetOutUnit(fVarName[var].first, fVarName[var].second);

    double sum = 0;
    for (int i=0; i<nGrans; i++) { // for slug and run, only one entry per slug/run
      // fVarValue[var][i] *= (UNITS[fVarInUnit[var]] / UNITS[fVarOutUnit[var]]);
      sum += fVarValue[var][i];
    }
    if (var.find("charge") != string::npos)
      cout << OUTPUT << var << ": " << sum << fVarOutUnit[var] << ENDL;
  }
}

void TCheckRS::CheckValues() {  // FIXME: do i need it?
  for (string solo : fSolos) {  // FIXME: solo variable may not contain leaf type
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
    for (int i=0; i<nOk; i++) {
      double val = fVarValue[solo][i];

      if (i == 0) {
        mean = val;
        sigma = 0;
      }

      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (burp_cut != 1024 && abs(val-mean) > burp_cut)) {
        cout << ALERT << "bad datapoint in " << solo
             << " in slug: " << ENDL;
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
    double unit = UNITS[fVarOutUnit[var1]];
    if (low_cut != 1024)
      low_cut /= unit;
    if (high_cut != 1024)
      high_cut /= unit;

    for (int i=0; i<nOk; i++) {
      double val1 = fVarValue[var1][i];
      double val2 = fVarValue[var1][i];
			double diff = abs(val1 - val2);

      if ( (low_cut  != 1024 && diff < low_cut)
        || (high_cut != 1024 && diff > high_cut)) {
        cout << ALERT << "bad datapoint in Comp: " << var1 << " vs " << var2 
             << " in slug: " << ENDL;
      }
    }
  }

  for (pair<string, string> cor : fCors) {
    string yvar = cor.first;
    string xvar = cor.second;
    double low_cut   = fCorCut[cor].low;
    double high_cut  = fCorCut[cor].high;
    double xunit = UNITS[fVarOutUnit[xvar]];
    double yunit = UNITS[fVarOutUnit[yvar]];
    if (low_cut != 1024)
      low_cut /= (yunit/xunit);
    if (high_cut != 1024)
      high_cut /= (yunit/xunit);
    // const double 
    for (int i=0; i<nOk; i++) {
      double xval = fVarValue[xvar][i];
      double yval = fVarValue[yvar][i];
    }
  }
  cout << INFO << "done with checking values" << ENDL;
}

void TCheckRS::Draw() {
	// make sure get err for mean values

  if (pt == slug) {
    CheckSlugs();
    GetSlugInfo();
    fSign = fSlugSign;
  } else if (pt == run) {
    CheckRuns();
    GetRunInfo();
    fSign = fRunSign;
  }
  CheckVars();
	GetValues();
	ProcessValues();
	// CheckValues();

	if (nOk > 100)
		c = new TCanvas("c", "c", 1800, 1200);
	else 
		c = new TCanvas("c", "c", 1200, 800);
  // c->SetGridy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(112200);
  // gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  // gStyle->SetBarWidth(1.05);
  gStyle->SetLabelSize(0.05, "XY");

  if (format == pdf)
    c->Print(Form("%s.pdf[", out_name));

  xmin = 10000;
  xmax = 0;
  for(int rs : fGrans) {
    if (rs < xmin) xmin=rs;
    if (rs > xmax) xmax=rs;
  }

  DrawSolos();
  // DrawSlopes();
  // DrawComps();
  DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << INFO << "done with drawing plots" << ENDL;
}

void TCheckRS::DrawSolos() {
  gStyle->SetTitleSize(0.05, "XY");
  gStyle->SetTitleXOffset(0.75);
  gStyle->SetTitleYOffset(0.7);
  vector<string> solos = fSolos;
  for (string var : fCustoms)
    solos.push_back(var);
  for (string solo : solos) {
    string branch = fVarName[solo].first;
    bool mean = false;
    string errvar;
    if (fVarName[solo].second == "mean") {
      mean = true;
      errvar = branch + ".err";
    }

    TH1F *h = new TH1F(solo.c_str(), "", xmax-xmin+1, xmin-0.5, xmax+0.5);
    TGraphErrors *g = new TGraphErrors();
    TH1F *h_pull = NULL;
    TH1F *pull = NULL;
    h->SetStats(kFALSE);
    g->SetMarkerStyle(20);
    if (mean) {
      h_pull = new TH1F("h_pull", "", xmax-xmin+1, xmin-0.5, xmax+0.5);
      h_pull->SetFillColor(kGreen);
      h_pull->SetLineColor(kGreen);
      h_pull->SetStats(kFALSE);
      h_pull->GetYaxis()->SetLabelSize(0.1);
      h_pull->GetXaxis()->SetLabelSize(0.1);
      pull = new TH1F("pull", "", 16, -8, 8);
      // pull->SetLineColor(kGreen);
    }

    map<int, TGraphErrors *> gflip;
    bool flip = false;
    // bool asym = solo.find("asym_us") != string::npos;  // asym var, only for det.
    if ((solo.find("asym_us") != string::npos && solo.find("diff") == string::npos) // not slope
         && flips.size() > 1) {
      flip = true;
      for (int i=0; i<flips.size(); i++) {
        gflip[flips[i]] = new TGraphErrors();
        gflip[flips[i]]->SetMarkerStyle(mstyles[i]);
        gflip[flips[i]]->SetMarkerColor(mcolors[i]);
      }
      g->SetMarkerStyle(1);
    } 

    bool alt = false;
    TGraphErrors *galt = NULL;
    vector<int> &alt_rs = fVarUseAlt[branch];
    if (alt_rs.size()) {
      alt = true;
      galt = new TGraphErrors();
      
      g->SetMarkerColor(kBlue);
      galt->SetMarkerStyle(20);
      galt->SetMarkerColor(kRed);
    }

    int i=0;
    double ymin = fVarValue[solo][0];
    double ymax = ymin;
    for(int rs : fGrans) {
      const int sessions = fRootFile[rs].size();
      for (int s=0; s<sessions; s++) {
        double val, err=0;
        val = fVarValue[solo][i];
        if (std::isnan(val)) {
          i++;
          continue;
        }
        if (mean)
          err = fVarValue[errvar][i];
        if (val + err > ymax) ymax = val + err;
        if (val - err < ymin) ymin = val - err;
        h->Fill(rs, val);
        g->SetPoint(i, rs, val);
        g->SetPointError(i, 0, err);
        i++;

        if (alt && find(alt_rs.begin(), alt_rs.end(), rs) != alt_rs.end()) {
          int j = galt->GetN();
          galt->SetPoint(j, rs, val);
          galt->SetPointError(j, 0, err);
        }

        if (flip) {
          int k = gflip[fSign[rs]]->GetN();
          gflip[fSign[rs]]->SetPoint(k, rs, val);
          gflip[fSign[rs]]->SetPointError(k, 0, err);
        }
      }
    }

    g->Fit("pol0");
    TF1 *fit= g->GetFunction("pol0");
    double mean_value = fit->GetParameter(0);
    cout << OUTPUT << solo << "\t" << mean_value << " ± " << fit->GetParError(0) << ENDL;
    if (mean) {
      int i=0;
      for (int rs : fGrans) {
        const int sessions = fRootFile[rs].size();
        for (int s=0; s<sessions; s++) {
          double val = fVarValue[solo][i];
          if (std::isnan(val)) {
            i++;
            continue;
          }
					double ratio = 0;
          double err = fVarValue[errvar][i];
					if (!std::isnan(err) && 0 != err)
						ratio = (val - mean_value)/err;
          h_pull->Fill(rs, ratio);
          pull->Fill(ratio);
          i++;
        }
      }
      pull->Fit("gaus");
    }

		string title = solo;
    if (alt)
      title = Form("#color[4]{%s} (#color[2]{%s})", solo.c_str(), fBrAlt[branch].c_str());
    if (sign)
      title = title + " (sign corrected)";
		h->SetTitle((title + ";" + granularity + ";" + fVarOutUnit[solo]).c_str());
    h->GetYaxis()->SetRangeUser(ymin, ymax+(ymax-ymin)/10);
    h->GetXaxis()->SetTitle(granularity);

    c->cd();
		// c->SetGridx();
    TPad *p1, *p2, *p3;
    // if (mean) {
    //   p1 = new TPad("p1", "p1", 0.0, 0.3, 0.7, 1.0);
    //   p2 = new TPad("p2", "p2", 0.0, 0.0, 0.7, 0.3);
    //   p3 = new TPad("p3", "p3", 0.7, 0.0, 1.0, 1.0);
    //   p1->SetBottomMargin(0);
    //   p1->SetRightMargin(0.05);
    //   p1->Draw();
    //   p1->SetGridy();
    //   p2->SetTopMargin(0);
    //   p2->SetRightMargin(0.05);
    //   p2->SetBottomMargin(0.16);
    //   p2->Draw();
    //   p2->SetGrid();
    //   p3->SetRightMargin(0.05);
    //   p3->Draw();
    // } else {
      p1 = new TPad("p1", "p1", 0.0, 0.0, 1.0, 1.0);
      p1->Draw();
      p1->SetGridy();
    // }

    p1->cd();
    h->Sumw2(kFALSE);
    h->Draw("*");
    g->Draw("P SAME");
    if (alt)
      galt->Draw("P SAME");
    p1->Update();
    TPaveStats *st = (TPaveStats *) g->FindObject("stats");
    if (st) {
      st->SetName(solo.c_str());
      st->SetX1NDC(0.65+0.05*(mean ? 1 : 0));
      st->SetX2NDC(0.9+0.05*(mean ? 1 : 0));
      st->SetY1NDC(0.75);
      st->SetY2NDC(0.9);
    }
    if (flip) {
      TLegend *l = new TLegend(0.1, 0.9-0.05*flips.size(), 0.2, 0.9);
      for (int i=0; i<flips.size(); i++) {
        gflip[flips[i]]->Draw("P SAME"); 
        l->AddEntry(gflip[flips[i]], legends[flips[i]], "p");
      }
      l->Draw();
    }
    TPaveText *t = (TPaveText *) p1->GetPrimitive("title");
    if (t)
      t->SetTextSize(0.07);

    // if (mean) {
    //   p2->cd();
    //   h_pull->Draw("HIST");
    //   h_pull->GetXaxis()->SetTitle(granularity);
    //   h_pull->GetXaxis()->SetTitleSize(0.1);
    //   p3->cd();
    //   pull->Draw();
    // }

    if (std::isnan(g->GetYaxis()->GetXmax())) {
      c->Clear();
      c->cd();
      TText *t = new TText(0.2, 0.45, Form("Blank page: no valid value for var: %s", solo.c_str())); 
      t->SetNDC();
      t->SetTextSize(0.15);
      c->Clear();
      t->Draw();
    }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, solo.c_str()));

    if (t)
      t->Delete();
		g->Delete();
    if (galt)  galt->Delete();
    if (mean) {
      h_pull->Delete();
      pull->Delete();
    }
    // if (st) st->Delete();
    if (flip) {
      for(int i=0; i<flips.size(); i++)
        gflip[flips[i]]->Delete();
    }
    c->Clear();
  }
  cout << INFO << "Done with drawing Solos." << ENDL;
}

void TCheckRS::DrawCors() {
  gStyle->SetTitleSize(0.09, "XY");
  // gStyle->SetLabelSize(0.09, "XY");
  gStyle->SetTitleXOffset(0.4);
  gStyle->SetTitleYOffset(0.4);
  for (pair<string, string> cor : fCors) {
    string var[2] = {cor.first, cor.second};
    string branch[2];
    TGraphErrors *g[2];
    TGraphErrors *galt[2];
    bool alt[2];
    vector<int> alt_rs[2];
    bool mean[2];
    string errvar[2];
    bool asym[2];

    for (int i=0; i<2; i++) {
      branch[i] = fVarName[var[i]].first;
      asym[i] = (branch[i].find("asym") != string::npos ^ branch[i].find("diff") != string::npos);
      g[i] = new TGraphErrors();
      galt[i] = NULL;
      alt[i] = false;
      alt_rs[i] = fVarUseAlt[branch[i]];
      mean[i] = false;
      if (fVarName[var[i]].second == "mean") {
        mean[i] = true;
        errvar[i] = branch[i] + ".err";
      }

      if (alt_rs[i].size()) {
        alt[i] = true;
        galt[i] = new TGraphErrors();
        
        // style
        g[i]->SetMarkerColor(kBlue);
        galt[i]->SetMarkerStyle(20);
        galt[i]->SetMarkerColor(kRed);
      }
      g[i]->SetMarkerStyle(20);

      int j=0, k=0;
      for(int rs : fGrans) {
        const int sessions = fRootFile[rs].size();
        for (int s=0; s<sessions; s++) {
          double val, err = 0;
          val = fVarValue[var[i]][j];
          if (mean[i])
            err = fVarValue[errvar[i]][j];
          g[i]->SetPoint(j, rs, val);
          g[i]->SetPointError(j, 0, err);
          j++;

          if (alt[i] && find(alt_rs[i].begin(), alt_rs[i].end(), rs) != alt_rs[i].end()) {
            galt[i]->SetPoint(k, rs, val);
            galt[i]->SetPointError(k, 0, err);
            k++;
          }
        }
      }
    }

    // title
		string title = var[0], ytitle = fVarOutUnit[var[0]];
    string title1=var[1], xtitle1(granularity), ytitle1 = fVarOutUnit[var[1]];
    if (alt[1]) 
      title1 = Form("#color[4]{%s} (#color[2]{%s})", var[1].c_str(), fBrAlt[branch[1]].c_str());
    if (sign && asym[1])
      title1 += " (sign corrected)";

    if (branch[0] == branch[1]) { // same branch
      title = fVarName[var[0]].first;
      title1 = "";
      ytitle = fVarName[var[0]].second + " / " + ytitle;
      ytitle1 = fVarName[var[1]].second + " / " + ytitle1;

      if (branch[0].find("asym") != string::npos ^ branch[0].find("diff") != string::npos) {
        g[0]->Fit("pol0");
        TF1 *fit = g[0]->GetFunction("pol0");
        double mean_value = fit->GetParameter(0);
        cout << OUTPUT << var[0] << "\t" << mean_value << " ± " << fit->GetParError(0) << ENDL;
        g[1]->Fit("pol0");
        fit = g[1]->GetFunction("pol0");
        mean_value = fit->GetParameter(0);
        cout << OUTPUT << var[1] << "\t" << mean_value << " ± " << fit->GetParError(0) << ENDL;
      }

    }
    if (alt[0])
      title = Form("#color[4]{%s} (#color[2]{%s})", title.c_str(), fBrAlt[branch[0]].c_str());
    if (sign && asym[0])
      title += " (sign corrected)";

		g[0]->SetTitle((title + ";;" + ytitle).c_str());
    g[1]->SetTitle((title1 + ";" + xtitle1 + ";" + ytitle1).c_str());

    TPad *p[2];
    p[0] = new TPad("p1", "p1", 0.0, 0.5, 1.0, 1.0);	// mean
    p[1] = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.5);	// rms
		p[0]->SetBottomMargin(0);
		p[1]->SetTopMargin(0);

    TPaveStats *st[2];
    for (int i=0; i<2; i++) {
      st[i] = NULL;
      c->cd();
      p[i]->Draw();
      p[i]->SetGridx();

      p[i]->cd();
      g[i]->Draw("AP");
      if (alt[i])
        galt[i]->Draw("P SAME");

      p[i]->Update();
      st[i] = (TPaveStats *) g[i]->FindObject("stats");
      if (st[i]) {
        st[i]->SetName(var[i].c_str());
        st[i]->SetX1NDC(0.75); 
        st[i]->SetX2NDC(0.9); 
        st[i]->SetY1NDC(0.75+0.1*i);
        st[i]->SetY2NDC(0.9+0.1*i);
      }

      if (std::isnan(g[i]->GetYaxis()->GetXmax())) {
        TText *t = new TText(0.2, 0.45, Form("Blank page: no valid value for var: %s", var[i].c_str())); 
        t->SetNDC();
        t->SetTextSize(0.15);
        p[i]->Clear();
        t->Draw();
      }
    }

    TPaveText *t = (TPaveText *) p[0]->GetPrimitive("title");
    if (t)
      t->SetTextSize(0.07);
    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s_vs_%s.png", out_name, var[0].c_str(), var[1].c_str()));

    if (t)
      t->Delete();
    for (int i=0; i<2; i++) {
      g[i]->Delete();
      if (galt[i])  galt[i]->Delete();
      // p[i]->Delete();
      // if (st[i]) st[i]->Delete();
    }
    c->Clear();
  }
  cout << INFO << "Done with drawing Correlations." << ENDL;
}

void TCheckRS::DrawComps() {
	/*
  int MarkerStyles[] = {29, 33, 34, 31};
  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
    string branch1 = fVarName[var1].first;
    string branch2 = fVarName[var2].first;
    string name1 = branch1.substr(branch1.find_last_of('_')+1);
    string name2 = branch2.substr(branch2.find_last_of('_')+1);

    const char * err_var1 = Form("%s.err", branch1.c_str());
    const char * err_var2 = Form("%s.err", branch2.c_str());
    bool meanflag = fVarStatType[var1] == mean;

    TGraphErrors * g1 = new TGraphErrors();
    TGraphErrors * g2 = new TGraphErrors();
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
    TH1F * h_diff = new TH1F("diff", "", nOk, 0, nOk);

    double min, max;
    for(int i=0; i<nOk; i++) {
      double val1, err1=0;
      double val2, err2=0;
      val1 = fVarValue[var1][i];
      val2 = fVarValue[var2][i];
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
    }
    double margin = (max-min)/10;
    min -= margin;
    max += margin;

    g1->GetXaxis()->SetRangeUser(0, nOk+1);
    h_diff->GetXaxis()->SetRangeUser(0, nOk+1);

		g1->SetTitle(Form("%s & %s;;%s", var1.c_str(), var2.c_str(), fVarOutUnit[var1]));

    // g1->Fit("pol0");
    // g2->Fit("pol0");

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
    if (nOk + 1 < 100)
      g1->GetXaxis()->SetNdivisions(-(nOk+1));
    else
      g1->GetXaxis()->SetNdivisions(-0);
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

    double labelSize = 0.05;
    double labelY = 0.17*0.3;
    double tickY = 0.17;
    ax->SetNdivisions(-0);
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    unsigned int nDivisions = nOk + 1;
    if (nDivisions < 100) {
      ax->SetNdivisions(nDivisions);
      // ax->SetLabelOffset(0.02);
      for (int i=0; i<=nOk; i++) {
        ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
      }
    } else {
      ax->SetLabelSize(0);  // remove old labels
      for (int i=0; i<nOk; i++) {
        if (fMiniruns[i].second == 0) {
          TText * label = new TText(0.1 + (i+1)*0.85/(nOk+1), labelY, Form("%d", fMiniruns[i].first));
          label->SetNDC();
          label->SetTextAngle(90);
          label->SetTextSize(labelSize);
          label->SetTextAlign(12);
          label->Draw();
          TText * tick = new TText(0.1 + (i+1)*0.85/(nOk+1), tickY*1.02, "|");
          tick->SetNDC();
          tick->SetTextAlign(21);
          tick->SetTextSize(labelSize*0.8);
          tick->Draw();
        }
      }
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
	 */
  cout << INFO << "Done with drawing Comparisons." << ENDL;
}

	/*
void TCheckRS::DrawCors() {
  for (pair<string, string> cor : fCors) {
    string xvar = cor.second;
    string yvar = cor.first;
    string xbranch = fVarName[xvar].first;
    string ybranch = fVarName[yvar].first;

    bool xmeanflag = fVarStatType[xvar] == mean;
    bool ymeanflag = fVarStatType[yvar] == mean;
    const char * xerr_var = Form("%s.err", xbranch.c_str());
    const char * yerr_var = Form("%s.err", ybranch.c_str());

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

    for(int i=0, ibold=0, ibad=0; i<nOk; i++) {
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
    // g->GetXaxis()->SetRangeUser(0, nOk+1);
		g->SetTitle((cor.first + " vs " + cor.second + ";" + fVarOutUnit[xvar] + ";" + fVarOutUnit[yvar]).c_str());
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
  cout << INFO << "Done with drawing Correlations." << ENDL;
}
	 */
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
