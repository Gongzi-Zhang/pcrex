#ifndef TCHECKSLUG_H
#define TCHECKSLUG_H

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
#include "TSlugBase.h"

using namespace std;

class TCheckSlug : public TSlugBase {

    // ClassDe (TCheckSlug, 0) // check statistics

  private:
    set<string> fBrVars;
    map<string, string> fVarInUnit;
    map<string, string> fVarOutUnit;
    TCanvas *c;

  public:
     TCheckSlug();
     ~TCheckSlug();
     void GetConfig(const TConfig fConf);
     void CheckVars();
		 void ProcessValues();
     void CheckValues();
     void Draw();
     void DrawSolos();
     void DrawComps();
     void DrawCors();

     // auxiliary funcitons
     const char * GetInUnit(string var);
     const char * GetOutUnit(string var);
};

// ClassImp(TCheckSlug);

TCheckSlug::TCheckSlug() :
  TSlugBase()
{
	out_name	= "checkslug";
	dir       = "rootfiles/";
	pattern   = "agg_slug_xxxx.root"; 
	tree      = "slug";
}

TCheckSlug::~TCheckSlug() {
  cout << INFO << "Release TCheckSlug" << ENDL;
}

void TCheckSlug::GetConfig(const TConfig fConf) {
  TBase::GetConfig(fConf);
  for (string var : fSolos) {
    if (var.find('.') != string::npos) {  // FIXME: what's the format should be
      cerr << ERROR << "branch only, no leaf allowed, in var: " << var << ENDL;
      exit(14);
    }
		fVars.insert(var + ".mean");
		fVars.insert(var + ".err");
		fVars.insert(var + ".rms");
		fBrVars.insert(var);
  }
  vector<pair<string, string>> pairs = fComps;
  for (pair<string, string> cor : fCors)
    pairs.push_back(cor);

  for (pair<string, string> p : pairs) {
    string var1 = p.first;
    string var2 = p.second;
    if (var1.find('.') != string::npos 
        || var2.find('.') != string::npos ) {  // FIXME: what's the format should be
      cerr << ERROR << "branch only, no leaf allowed, in var: " << var1 << "/" << var2 << ENDL;
      exit(14);
    }
		fVars.insert(var1 + ".mean");
		fVars.insert(var1 + ".err");
		fVars.insert(var1 + ".rms");
		fBrVars.insert(var1);
		fVars.insert(var2 + ".mean");
		fVars.insert(var2 + ".err");
		fVars.insert(var2 + ".rms");
		fBrVars.insert(var2);
  }
}

void TCheckSlug::CheckVars() {
  TBase::CheckVars();
  for (string var : fBrVars) 
    fVars.erase(var);
}

void TCheckSlug::ProcessValues() {
  for (string var : fVars) {
    fVarInUnit[var] = GetInUnit(var);
    fVarOutUnit[var] = GetOutUnit(var);

    for (int i=0; i<nOk; i++) {
      fVarValue[var][i] *= (UNITS[fVarInUnit[var]] / UNITS[fVarOutUnit[var]]);
    }
	}

	// 	// sign correction: only for mean value
	// 	if (sign && fVarStatType[var] == mean) {
  //     int m = 0;
	// 		for (int run : fRuns) {
  //       int s = fRunSign[run] > 0 ? 1 : (fRunSign[run] < 0 ? -1 : 0);
  //       const size_t sessions = fRootFile[run].size();
  //       for (size_t session=0; session < sessions; session++) {
  //         for (int i = 0; i < fEntryNumber[run][session].size(); i++, m++)
  //           fVarValue[var][m] *= s;
  //       }
  //     }
	// 	}
	// }
}

void TCheckSlug::CheckValues() {  // FIXME: do i need it?
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
    double unit = UNITS[GetUnit(var1)];
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

  for (pair<string, string> slope : fSlopes) {
    string dv = slope.first;
    string iv = slope.second;
    double low_cut  = fSlopeCut[slope].low;
    double high_cut = fSlopeCut[slope].high;
    double burp_cut = fSlopeCut[slope].burplevel;

    double dunit = UNITS[GetUnit(dv)];
    double iunit = UNITS[GetUnit(iv)];
    if (low_cut != 1024)
      low_cut /= (dunit/iunit);
    if (high_cut != 1024)
      high_cut /= (dunit/iunit);
    if (burp_cut != 1024)
      burp_cut /= (dunit/iunit);

    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean = fSlopeValue[slope][0];
    double sigma = fSlopeErr[slope][0];
    for (int i=0; i<nOk; i++) {
      double val = fSlopeValue[slope][i];
      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (burp_cut != 1024 && abs(val-mean) > burp_cut)) {
        cout << ALERT << "bad datapoint in slope: " << slope.first << " vs " << slope.second 
             << " in slug: " << ENDL;
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
    double low_cut   = fCorCut[cor].low;
    double high_cut  = fCorCut[cor].high;
    double xunit = UNITS[GetUnit(xvar)];
    double yunit = UNITS[GetUnit(yvar)];
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

void TCheckSlug::Draw() {
	// make sure get err for mean values

	CheckSlugs();
	CheckVars();
	GetValues();
	ProcessValues();
	// CheckValues();

	if (nOk > 100)
		c = new TCanvas("c", "c", 1800, 600);
	else 
		c = new TCanvas("c", "c", 1200, 600);
  // c->SetGridy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(1110);
  // gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  gStyle->SetBarWidth(1.05);
  gStyle->SetTitleSize(0.09, "XY");
  gStyle->SetTitleXOffset(0.4);
  gStyle->SetTitleYOffset(0.15);

  if (format == pdf)
    c->Print(Form("%s.pdf[", out_name));

  DrawSolos();
  // DrawSlopes();
  // DrawComps();
  // DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << INFO << "done with drawing plots" << ENDL;
}

void TCheckSlug::DrawSolos() {
  for (string solo : fSolos) {
    TGraphErrors *g1 = new TGraphErrors();
    TGraphErrors *galt1 = NULL, *galt2 = NULL;
    TGraphErrors *g2 = new TGraphErrors();

    bool alt = false;
    vector<int> &alt_slugs = fVarUseAlt[solo];
    if (alt_slugs.size()) {
      alt = true;
      galt1 = new TGraphErrors();
      galt2 = new TGraphErrors();
      
      // style
      g1->SetMarkerColor(kBlue);
      g2->SetMarkerColor(kBlue);
      galt1->SetMarkerStyle(20);
      galt2->SetMarkerStyle(20);
      galt1->SetMarkerColor(kRed);
      galt2->SetMarkerColor(kRed);
    }

    int i = 0, j=0;
    for(int slug : fSlugs) {
			// no need for session
      double val, err, rms;
      val = fVarValue[solo + ".mean"][i];
			err = fVarValue[solo+".err"][i];
			rms = fVarValue[solo+".rms"][i];
      g1->SetPoint(i, slug, val);
      g1->SetPointError(i, 0, err);
      g2->SetPoint(i, slug, rms);

      if (alt && find(alt_slugs.begin(), alt_slugs.end(), slug) != alt_slugs.end()) {
        galt1->SetPoint(j, slug, val);
        galt1->SetPointError(j, 0, err);
        galt2->SetPoint(j, slug, rms);
        j++;
      }
      i++;
    }
    // g->GetXaxis()->SetRangeUser(0, nOk+1);
		string title = solo;
    if (alt)
      title = Form("#color[4]{%s}(#color[2]{%s})", solo.c_str(), fBrAlt[solo].c_str());

    // style
		g1->SetTitle((title + ";;" + fVarOutUnit[solo + ".mean"]).c_str());
    g1->SetMarkerStyle(20);

    g2->SetMarkerStyle(20);
    g2->SetTitle(Form(";slug;RMS Width (%s);", fVarOutUnit[solo+".rms"].c_str()));

    // g->Fit("pol0");
    // TF1 * fit= g->GetFunction("pol0");
    // double mean_value = fit->GetParameter(0);
		// cout << OUTPUT << solo << "\t" << mean_value << " ± " << fit->GetParError(0) << ENDL;

    c->cd();
    TPad *p1 = new TPad("p1", "p1", 0.0, 0.5, 1.0, 1.0);	// mean
    TPad *p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.5);	// rms
		p1->SetBottomMargin(0);
		p2->SetTopMargin(0);

		p1->Draw();
		p1->SetGridx();
		p2->Draw();
		p2->SetGridx();

    p1->cd();
    g1->Draw("AP");
    if (alt)
      galt1->Draw("P SAME");
    p1->Update();
		p2->cd();
		g2->Draw("AP");
    if (alt)
      galt2->Draw("P SAME");
    p2->Update();
    // st = (TPaveStats *) g->FindObject("stats");
    // st->SetName("g_stats");
    // double width = 0.7/(flips.size() + 1);
    // st->SetX2NDC(0.95); 
    // st->SetX1NDC(0.95-width); 
    // st->SetY2NDC(0.9);
    // st->SetY1NDC(0.8);

    // TAxis * ax = g->GetXaxis();
    // TAxis * ay = g->GetYaxis();

    // double min = ay->GetXmin();
    // double max = ay->GetXmax();
    // ay->SetRangeUser(min, max+(max-min)/9);

    TPaveText *t = (TPaveText *) p1->GetPrimitive("title");
    t->SetTextSize(0.09);
    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, solo.c_str()));

    t->Delete();
		g1->Delete();
		g2->Delete();
    if (galt1)  galt1->Delete();
    if (galt2)  galt2->Delete();
    c->Clear();
  }
  cout << INFO << "Done with drawing Solos." << ENDL;
}

void TCheckSlug::DrawComps() {
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

void TCheckSlug::DrawCors() {
	/*
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
	 */
  cout << INFO << "Done with drawing Correlations." << ENDL;
}

const char * TCheckSlug::GetInUnit (string var) {
  string branch = fVarName[var].first;
  if (branch.find("asym") != string::npos) 
    return "";
  else if (branch.find("diff") != string::npos) 
    return "mm";
  else if (branch.find("yield") != string::npos) {
    if (branch.find("bcm") != string::npos) 
      return "uA";
    else if (branch.find("bpm") != string::npos) 
      return "mm";
  } else
    return "";
}

const char * TCheckSlug::GetOutUnit (string var) {
  string branch = fVarName[var].first;
  string leaf   = fVarName[var].second;
  if (branch.find("asym") != string::npos) {
    if (leaf.find("mean") != string::npos || leaf.find("err") != string::npos)
      return "ppb";
    else // if (var.find("rms") != string::npos)
      return "ppm";
  } else if (branch.find("diff") != string::npos) {
    if (leaf.find("mean") != string::npos || leaf.find("err") != string::npos)
      return "nm";
    else // if (var.find("rms") != string::npos)
      return "um";
  } else if (branch.find("yield") != string::npos) {
    if (branch.find("bcm") != string::npos) 
      return "uA";
  } else
    return "";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
