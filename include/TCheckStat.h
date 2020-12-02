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
#include "TBase.h"

enum SType {mean, err, rms};	// statistical type

map<int, const char *> legends = {
  {-1,  "left in"},
  {1,		"left out"},
  {2,		"right in"},
  {-2,  "right out"},
  {-3,  "up in"},
  {3,		"up out"},
  {4,		"down in"},
  {-4,  "down out"},
};

using namespace std;

class TCheckStat : public TBase {

    // ClassDe (TCheckStat, 0) // check statistics

  private:
		set<int> fBoldRuns;
		int nBoldRuns;
    bool  sign = false;
    vector<int> flips;

		map<string, SType> fVarStatType;
    vector<pair<int, int>> fMiniruns;
		int nMiniruns;
    map<string, set<pair<int, int>>>  fSoloBadMiniruns;
    map<pair<string, string>, set<pair<int, int>>>	  fCompBadMiniruns;
    map<pair<string, string>, set<pair<int, int>>>	  fSlopeBadMiniruns;
    map<pair<string, string>, set<pair<int, int>>>	  fCorBadMiniruns;

  public:
     TCheckStat(const char*, const char* run_list = NULL);
     ~TCheckStat();
     void SetBoldRuns(set<int> runs);
     void SetSign() {sign = true;}
		 void ProcessValues();
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

TCheckStat::TCheckStat(const char* config_file, const char *run_list) :
  TBase(config_file, run_list)
{
	program		= checkstatistics;
	out_name	= "check";
	// dir       = "/chafs2/work1/apar/postpan-outputs/";
	// pattern   = "prexPrompt_xxxx_???_regress_postpan.root"; 
	// tree      = "mini";
  fBoldRuns = fConf.GetBoldRuns();
}

TCheckStat::~TCheckStat() {
  cout << INFO << "Release TCheckStat" << ENDL;
}

void TCheckStat::SetBoldRuns(set<int> runs) {
  for(int run : runs) {
    if (run < START_RUN || run > END_RUN) {
      cerr << ERROR << "Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << ENDL;
      continue;
    }
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
  nBoldRuns = fBoldRuns.size();
}

void TCheckStat::ProcessValues() {
	for (map<int, int>::const_iterator it = fRunSign.cbegin(); it!=fRunSign.cend(); it++) {
		if (find(flips.cbegin(), flips.cend(), it->second) == flips.cend())
			flips.push_back(it->second);
	}

	nMiniruns = fEntryNumber.size();
	{
		for (int run : fRuns) {
      const size_t sessions = fRootFile[run].size();
      for (size_t session=0; session < sessions; session++) {
        for (int m=0; m<fEntryNumber[run][session].size(); m++)
          fMiniruns.push_back(make_pair(run, fEntryNumber[run][session][m]));
      }
		}
	}
	for (string var : fVars) {
		// unit correction
		fVarUnit[var] = GetUnit(var);
		for (int m=0; m<nMiniruns; m++) {
			fVarValue[var][m] /= UNITS[fVarUnit[var]];
		}

		// sign correction: only for mean value
		if (sign && fVarStatType[var] == mean) {
      int m = 0;
			for (int run : fRuns) {
        int s = fRunSign[run] > 0 ? 1 : (fRunSign[run] < 0 ? -1 : 0);
        const size_t sessions = fRootFile[run].size();
        for (size_t session=0; session < sessions; session++) {
          for (int i = 0; i < fEntryNumber[run][session].size(); i++, m++)
            fVarValue[var][m] *= s;
        }
      }
		}
	}

	for (string var : fVars) {	// weight raw det asym with lagr. det asym error
		TString v = var;
		if (v.CountChar('.') == 2)
			v = var.substr(var.find('.')+1);
		if (	 v == "asym_us_avg.err"
				|| v == "asym_bcm_target.err"	
				|| v == "asym_bcm_target.hw_sum_err"	
				|| (var.find("diff_bpm") != string::npos && (v.EndsWith(".hw_sum_err") || v.EndsWith(".err"))))
		{
      int m = 0;
			for (int run : fRuns) {
        const size_t sessions = fRootFile[run].size();
        for (size_t session=0; session < sessions; session++) {
          for (int i = 0; i < fEntryNumber[run][session].size(); i++, m++)
            fVarValue[var][m] = fVarValue["lagr_asym_us_avg.err"][m];	// weighted by lagr_asym_us_avg err bar
        }
			}
		}
	}
}

void TCheckStat::CheckValues() {
  for (string solo : fSolos) {
    double low_cut  = fSoloCut[solo].low;
    double high_cut = fSoloCut[solo].high;
    double burp_cut = fSoloCut[solo].burplevel;
    double unit = UNITS[fVarUnit[solo]];
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
    double low_cut  = fCompCut[comp].low;
    double high_cut = fCompCut[comp].high;
    double unit = UNITS[GetUnit(var1)];
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
        fCompBadMiniruns[comp].insert(fMiniruns[i]);
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
    for (int i=0; i<nMiniruns; i++) {
      double val = fSlopeValue[slope][i];
      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (burp_cut != 1024 && abs(val-mean) > burp_cut)) {
        cout << ALERT << "bad datapoint in slope: " << slope.first << " vs " << slope.second 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << ENDL;
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
    double low_cut   = fCorCut[cor].low;
    double high_cut  = fCorCut[cor].high;
    double xunit = UNITS[GetUnit(xvar)];
    double yunit = UNITS[GetUnit(yvar)];
    if (low_cut != 1024)
      low_cut /= (yunit/xunit);
    if (high_cut != 1024)
      high_cut /= (yunit/xunit);
    // const double 
    for (int i=0; i<nMiniruns; i++) {
      double xval = fVarValue[xvar][i];
      double yval = fVarValue[yvar][i];

			/*
      if () {
        cout << ALERT << "bad datapoint in Cor: " << yvar << " vs " << xvar 
             << " in run: " << fMiniruns[i].first << "." << fMiniruns[i].second << ENDL;
        fCorBadMiniruns[*it].insert(fMiniruns[i]);
      }
			*/
    }
  }
  cout << INFO << "done with checking values" << ENDL;
}

void TCheckStat::Draw() {
	// make sure get err for mean values
	for (string var : fVars) {
		TString v = var;
		if (v.EndsWith(".mean")) {
			fVarStatType[var] = mean;
			string err_var = var;	
			err_var.replace(err_var.find(".mean"), 5, ".err");	
			fVars.insert(err_var);	// add new elements while looping the set
			fVarStatType[err_var] = err;
		} else if (v.EndsWith(".hw_sum")) {
			fVarStatType[var] = mean;
			string err_var = var;	
			err_var.replace(err_var.find(".hw_sum"), 7, ".hw_sum_err");
			fVars.insert(err_var);
			fVarStatType[err_var] = err;
		} else if (v.EndsWith(".err") || v.EndsWith(".hw_sum_err")) {
			fVarStatType[var] = err;
		} else if (v.EndsWith(".rms") || v.EndsWith(".hw_sum_m2") ) {
			fVarStatType[var] = rms;
		} else {
			cerr << WARNING << "unknow statistical for var: " << var << ENDL;
			// exit(204);
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
  // DrawSlopes();
  // DrawComps();
  // DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << INFO << "done with drawing plots" << ENDL;
}

void TCheckStat::DrawSolos() {
  for (string solo : fSolos) {
		string branch = fVarName[solo].first;
		string leaf = fVarName[solo].second;
    string err_var;
		bool meanflag = fVarStatType[solo] == mean;

    if (meanflag) {
			if (leaf.find("mean") != string::npos)
				err_var = branch + ".err";
			else if (leaf.find("hw_sum") != string::npos)
				err_var = branch + ".hw_sum_err";
		}

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bold = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips;
    for (int i=0; i<flips.size(); i++) {
      g_flips[flips[i]] = new TGraphErrors();
    }

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
      double val, err=0;
      val = fVarValue[solo][i];
      if (meanflag) {
        err = fVarValue[err_var][i];
      }
      g->SetPoint(i, i+1, val);
      g->SetPointError(i, 0, err);

      int ipoint = g_flips[fRunSign[fMiniruns[i].first]]->GetN();
      g_flips[fRunSign[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val);
      g_flips[fRunSign[fMiniruns[i].first]]->SetPointError(ipoint, 0, err);

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
		string title = solo;
		if (count(title.begin(), title.end(), '.') == 2) {
			title = title.substr(title.find('.')+1);
		}
    if (sign)
      g->SetTitle((title + " (sign corrected);;" + fVarUnit[solo]).c_str());
    else
      g->SetTitle((title + ";;" + fVarUnit[solo]).c_str());
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
		cout << OUTPUT << solo << "\t" << mean_value << " ± " << fit->GetParError(0) << ENDL;

    TGraph * pull = NULL; 
    if (meanflag) {
      pull = new TGraph();

      for (int i=0; i<nMiniruns; i++) {
        double ratio = 0;
        if (fVarValue[err_var][i]!= 0)
          ratio = (fVarValue[solo][i]-mean_value)/fVarValue[err_var][i];

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
    if (meanflag) {
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

    double labelSize = 0.03;
    double labelY = 0.16 * 0.5;
    double tickY = 0.16;
    if (meanflag) {
      labelSize = 0.05;
      labelY = 0.17*0.3;
      tickY = 0.17;
      g->GetXaxis()->SetLabelSize(0);
      if (nMiniruns + 1 < 100)
        g->GetXaxis()->SetNdivisions(-(nMiniruns+1));
      else 
        g->GetXaxis()->SetNdivisions(0);

      p2->cd();
      pull->SetFillColor(kGreen);
      pull->SetLineColor(kGreen);
      pull->Draw("AB");
      ax = pull->GetXaxis();
    }

    ax->SetNdivisions(-0);
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    unsigned int nDivisions = nMiniruns + 1;
    if (nDivisions < 100) {
      ax->SetNdivisions(-nDivisions);
      // ax->SetLabelOffset(0.02);
      for (int i=0; i<=nMiniruns; i++) {
        ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
      }
    } else {
      ax->SetLabelSize(0);  // remove old labels
      for (int i=0; i<nMiniruns; i++) {
        if (fMiniruns[i].second == 0) {
          TText * label = new TText(0.1 + (i+1)*0.85/(nMiniruns+1), labelY, Form("%d", fMiniruns[i].first));
          label->SetNDC();
          label->SetTextAngle(90);
          label->SetTextSize(labelSize);
          label->SetTextAlign(12);
          label->Draw();
          TText * tick = new TText(0.1 + (i+1)*0.85/(nMiniruns+1), tickY*1.02, "|");
          tick->SetNDC();
          tick->SetTextAlign(21);
          tick->SetTextSize(labelSize*0.8);
          tick->Draw();
        }
      }
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
  cout << INFO << "Done with drawing Solos." << ENDL;
}

void TCheckStat::DrawSlopes() {
  for (pair<string, string> slope : fSlopes) {
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
      val = fSlopeValue[slope][i];
      err = fSlopeErr[slope][i];
      g->SetPoint(i, i+1, val);
      g->SetPointError(i, 0, err);

      int ipoint = g_flips[fRunSign[fMiniruns[i].first]]->GetN();
      g_flips[fRunSign[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val);
      g_flips[fRunSign[fMiniruns[i].first]]->SetPointError(ipoint, 0, err);
      
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
      if (fSlopeErr[slope][i] != 0)
        ratio = (fSlopeValue[slope][i]-mean_value)/fSlopeErr[slope][i];

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
    if (nMiniruns + 1 < 100)
      g->GetXaxis()->SetNdivisions(-(nMiniruns+1));
    else 
      g->GetXaxis()->SetNdivisions(-0);
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

    double labelSize = 0.05;
    double labelY = 0.17*0.3;
    double tickY = 0.17;
    ax->SetNdivisions(-0);
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    unsigned int nDivisions = nMiniruns + 1;
    if (nDivisions < 100) {
      ax->SetNdivisions(nDivisions);
      // ax->SetLabelOffset(0.02);
      for (int i=0; i<=nMiniruns; i++) {
        ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
      }
    } else {
      ax->SetLabelSize(0);  // remove old labels
      for (int i=0; i<nMiniruns; i++) {
        if (fMiniruns[i].second == 0) {
          TText * label = new TText(0.1 + (i+1)*0.85/(nMiniruns+1), labelY, Form("%d", fMiniruns[i].first));
          label->SetNDC();
          label->SetTextAngle(90);
          label->SetTextSize(labelSize);
          label->SetTextAlign(12);
          label->Draw();
          TText * tick = new TText(0.1 + (i+1)*0.85/(nMiniruns+1), tickY*1.02, "|");
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
      c->Print(Form("%s_%s_%s.png", out_name, slope.first.c_str(), slope.second.c_str()));
    c->Clear();
    pull->Delete();
    pull = NULL;
  }
  cout << INFO << "Done with drawing Slopes." << ENDL;
}

void TCheckStat::DrawComps() {
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
      val1 = fVarValue[var1][i];
      val2 = fVarValue[var2][i];
      if (meanflag) {
        err1 = fVarValue[err_var1][i];
        err2 = fVarValue[err_var2][i];
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
      int ipoint = g_flips1[fRunSign[fMiniruns[i].first]]->GetN();
      g_flips1[fRunSign[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val1);
      g_flips1[fRunSign[fMiniruns[i].first]]->SetPointError(ipoint, 0, err1);
      g_flips2[fRunSign[fMiniruns[i].first]]->SetPoint(ipoint, i+1, val2);
      g_flips2[fRunSign[fMiniruns[i].first]]->SetPointError(ipoint, 0, err2);
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
      g1->SetTitle(Form("%s & %s (sign corrected);;%s", var1.c_str(), var2.c_str(), fVarUnit[var1]));
    else 
      g1->SetTitle(Form("%s & %s;;%s", var1.c_str(), var2.c_str(), fVarUnit[var1]));
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
    if (nMiniruns + 1 < 100)
      g1->GetXaxis()->SetNdivisions(-(nMiniruns+1));
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
    unsigned int nDivisions = nMiniruns + 1;
    if (nDivisions < 100) {
      ax->SetNdivisions(nDivisions);
      // ax->SetLabelOffset(0.02);
      for (int i=0; i<=nMiniruns; i++) {
        ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d_%02d", fMiniruns[i].first, fMiniruns[i].second));
      }
    } else {
      ax->SetLabelSize(0);  // remove old labels
      for (int i=0; i<nMiniruns; i++) {
        if (fMiniruns[i].second == 0) {
          TText * label = new TText(0.1 + (i+1)*0.85/(nMiniruns+1), labelY, Form("%d", fMiniruns[i].first));
          label->SetNDC();
          label->SetTextAngle(90);
          label->SetTextSize(labelSize);
          label->SetTextAlign(12);
          label->Draw();
          TText * tick = new TText(0.1 + (i+1)*0.85/(nMiniruns+1), tickY*1.02, "|");
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
  cout << INFO << "Done with drawing Comparisons." << ENDL;
}

void TCheckStat::DrawCors() {
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

    for(int i=0, ibold=0, ibad=0; i<nMiniruns; i++) {
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
    // g->GetXaxis()->SetRangeUser(0, nMiniruns+1);
    if (sign)
      g->SetTitle((cor.first + " vs " + cor.second + " (sign corrected);" + fVarUnit[xvar] + ";" + fVarUnit[yvar]).c_str());
    else
      g->SetTitle((cor.first + " vs " + cor.second + ";" + fVarUnit[xvar] + ";" + fVarUnit[yvar]).c_str());
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

const char * TCheckStat::GetUnit (string var) {
  string branch = fVarName[var].first;
  string leaf   = fVarName[var].second;
  if (branch.find("asym") != string::npos) {
    if (fVarStatType[var] == mean || fVarStatType[var] == err)
      return "ppb";
    else if (fVarStatType[var] == rms)
      return "ppm";
  } else if (branch.find("diff") != string::npos) {
    if (fVarStatType[var] == mean || fVarStatType[var] == err)
      return "nm";
    else if (fVarStatType[var] == rms)
      return "um";
  }
	return "";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
