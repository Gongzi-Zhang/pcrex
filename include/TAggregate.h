#ifndef TAGGREGATE_H
#define TAGGREGATE_H

#include <iostream>
#include <glob.h>
#include "TFile.h"
#include "TTree.h"
#include "TRSbase.h"

struct STAT { double mean, err, rms; };

class TAggregate : public TRSbase 
{
	private:
    int run;
		// const char * out_pattern = "agg_minirun_xxxx_???";
	public:
		TAggregate();
		~TAggregate() { cout << INFO << "end of TAggregate" << ENDL; };
    void SetAggRun(int r);
    void CheckVars();
    void ProcessValues();
		void Aggregate();
    void AggregateRuns();
};

TAggregate::TAggregate() :
	TRSbase()
{}

void TAggregate::SetAggRun(int r) 
{ 
  run=r; 
  fRuns.clear(); 
  fSlugs.clear(); 
  SetRuns({run}); 
  CheckRuns();
  nSlugs = fSlugs.size();
}

void TAggregate::CheckVars() {
  const char *fname = Form("%s/agg_minirun_%d.root", out_dir, run);
  glob_t globbuf;
  glob(fname, 0, NULL, &globbuf);
  if (globbuf.gl_pathc == 0) {
    globfree(&globbuf);
    return;
  }
  globfree(&globbuf);

  TFile fout(fname, "read");
  if (!fout.IsOpen()) {
    cerr << ERROR << "Can't open file: " << fname << ENDL;
    fout.Close();
    exit(24);
  }
  TTree *trun	 = (TTree *) fout.Get("run");
  if (!trun) {
    fout.Close();
    return;
  }

  for (string var : fVars) {
    if (trun->GetBranch(var.c_str())) {
      fVars.erase(var);
      continue;
    } 
  }
  for (vector<string>::iterator it=fCustoms.begin(); it!=fCustoms.end(); ) {
    if (trun->GetBranch(it->c_str())) {
      it = fCustoms.erase(it);
      continue;
    } 
    it++;
  }
  for (vector<pair<string, string>>::iterator it=fSlopes.begin(); it != fSlopes.end(); ) { 
    string var = it->first + '_' + it->second;
    if (trun->GetBranch(var.c_str())) {
      it = fSlopes.erase(it);
      continue;
    }
    it++;
  }
  trun->Delete();
  fout.Close();
}

void TAggregate::ProcessValues() {
  fRunArm[run] = GetRunArmFlag(run);
  int iok = 0;
  const int sessions = fRootFile[run].size();
  if (fRunArm[run] != 0) {  // single arm
    const char *arm = "r";
    const char *other_arm = "l";
    if (fRunArm[run] == 2) {
      arm = "l";
      other_arm = "r";
    }
    cerr << WARNING << "single arm (" << arm << ") run: " << run << ENDL;
    map<string, string> svar;
    set<string> zvar;
    for (string var : fVars) {
      int pos;
      string v = var;
      if ((pos = var.find("_avg")) != string::npos) {
        v.replace(pos, 4, arm);
      } else if ((pos = var.find("_dd")) != string::npos) {
        v.replace(pos, 3, arm);
      } else {
        if (var.find(Form("asym_us%s", other_arm)) != string::npos) {
          cerr << WARNING << "black out (make it nan) var: " << var << ENDL;
          zvar.insert(var);
        }
        continue;
      } 

      if (fVars.find(v) == fVars.end()) {
        cerr << ALERT << "run-" << run << " is a single arm run, combo var: " << var 
          << "can't find its replacement: " << v << ENDL;
        break;
      }
      cerr << WARNING << "replace var: " << var << " with " << v << ENDL;
      svar[var] = v;

    }
    for (int session=0; session < sessions; session++) {
      int n	= fEntryNumber[run][session].size();
      for (int i=0; i<n; i++, iok++) {
        for (pair<string, string> p : svar)
          fVarValue[p.first][iok] = fVarValue[p.second][iok];
        for (string var : zvar)
          fVarValue[var][iok] = 0/0.;
      }
    }
  }
  GetCustomValues();
}

void TAggregate::Aggregate()
{
	unsigned int num_samples = 0;
	map<string, double> mini_sum, sum;
	map<string, double> mini_sum2, sum2;
  map<string, double> sum_weight;
	map<string, STAT> mini_stat, stat;
	unsigned int minirun = 0; // number of miniruns
	unsigned int N = 0;   // total number of mul patterns

  char hostname[32];
  gethostname(hostname, 32);
  const char *run_prefix = "./", *mini_prefix = "./";
  int nburst_dv = 56;
  if (Contain(hostname, "aonl") || Contain(hostname, "adaq")) {
    run_prefix  = "/adaqfs/home/apar/PREX/prompt/japanOutput/";
    mini_prefix = "/adaqfs/home/apar/PREX/prompt/results/"; 
		if (run <= 6464)
			nburst_dv = 48;
  } else if (Contain(hostname, "ifarm")) {
    run_prefix  = "/lustre19/expphy/volatile/halla/parity/crex-respin1/japanOutput/";
    mini_prefix = "/lustre19/expphy/volatile/halla/parity/crex-respin1/postpan_respin/";
  }

  int iok = 0;
  double run_slope[5][nburst_dv], run_slope_err[5][nburst_dv];
  // double mini_slope[61][5], mini_slope_err[61][5];
  double burst_slope[5][nburst_dv], burst_slope_err[5][nburst_dv];
  for (int i=0; i<5; i++)
    for (int j=0; j<nburst_dv; j++) {
      run_slope[i][j] = 0;
      run_slope_err[i][j] = 0;
      burst_slope[i][j] = 0;
      burst_slope_err[i][j] = 0;
    }
  map<string, int> burst_dv_index;
  vector<string> &BURST_DV = (run <= 6464) ? BURST_DV1 : BURST_DV2;
  for (vector<pair<string, string>>::iterator it_s = fSlopes.begin(); it_s != fSlopes.end(); ) {
    vector<string>::iterator it = find(BURST_DV.begin(), BURST_DV.end(), it_s->first);
    if (it != BURST_DV.end()) {
      burst_dv_index[it_s->first] = it-BURST_DV.begin();
      it_s++;
    } else {
      cerr << WARNING << "^^^^run: " << run << "\tno such device: " << it_s->first << " in burst device list" << ENDL;
      it_s = fSlopes.erase(it_s);
    }
  }

  bool update=true;	// update tree: add new branches
  vector<TBranch *> mini_brs, run_brs;

  const char *fname = Form("%s/agg_minirun_%d.root", out_dir, run);
  TFile fout(fname, "update");
  if (!fout.IsOpen()) {
    cerr << ERROR << "Can't open file: " << fname << ENDL;
    fout.Close();
    return;
  }
  TTree *tout_mini = (TTree *) fout.Get("mini");
  TTree *tout_run	 = (TTree *) fout.Get("run");
  if (!tout_mini) {
    tout_mini = new TTree("mini", "mini run average value");
    tout_mini->Branch("run", &run, "run/i");
    tout_mini->Branch("minirun", &minirun, "minirun/i");
    tout_mini->Branch("num_samples", &num_samples, "num_samples/i");

    tout_run = new TTree("run", "run average value");
    tout_run->Branch("run", &run, "run/i");
    tout_run->Branch("minirun", &minirun, "minirun/i");
    tout_run->Branch("num_samples", &N, "num_samples/i");

    update=false;
  } 

  for (string custom : fCustoms)
    fVars.insert(custom);
  set<string> allVars = fVars; // fVars + fCustoms
  for (string custom : fCustoms)
    allVars.insert(custom);

  // initialization
  for (string var : allVars) {
    mini_sum[var] = 0;
    mini_sum2[var] = 0;
    mini_stat[var] = {0./0., 0./0., 0./0.};
    sum[var] = 0;
    sum2[var] = 0;
    stat[var] = {0./0., 0./0., 0./0.};
    TBranch *b = tout_mini->Branch(var.c_str(), &mini_stat[var], "mean/D:err/D:rms/D");
    TBranch *b1 = tout_run->Branch(var.c_str(), &stat[var], "mean/D:err/D:rms/D");
    if (update) {
      mini_brs.push_back(b);
      run_brs.push_back(b1);
    }
  }
  for (pair<string, string> slope : fSlopes) { 
    string var = slope.first + '_' + slope.second;
    string mini_var = "reg_" + var;
    string burst_var = "burst_" + var;
    TBranch *b_mini = tout_mini->Branch(mini_var.c_str(), &mini_stat[mini_var], "mean/D:err/D:rms/D");
    TBranch *b_burst = tout_mini->Branch(burst_var.c_str(), &mini_stat[burst_var], "mean/D:err/D:rms/D");
    TBranch *b1 = tout_run->Branch(var.c_str(), &stat[var], "mean/D:err/D:rms/D");
    if (update) {
      mini_brs.push_back(b_mini);
      mini_brs.push_back(b_burst);
      run_brs.push_back(b1);
    }

    sum[var] = 0;
    sum_weight[var] = 0;
    stat[var] = {0./0., 0./0., 0./0.};
  }

  int Nmini = 0;
  const int sessions = fRootFile[run].size();
  for (int session=0; session < sessions; session++) {
    int n	= fEntryNumber[run][session].size();
    if (n == 0) 
      continue;
    Nmini += n>9000 ? n/9000 : 1;
    N += n;
  }

  if (update && Nmini != tout_mini->GetEntries()) {
    cerr << ERROR << "Unmatch # of miniruns: "
      << "\told: " << tout_mini->GetEntries() 
      << "\tnew: " << Nmini << ENDL;
    return;
  }

  minirun = 0;
  for (int session=0; session < sessions; session++) {
    int n	= fEntryNumber[run][session].size();
    if (n == 0) {
      cerr << WARNING << "no valid minirun in run: " << run << ", session: " << session << ENDL;
      continue;
    }
    int Smini = n>9000 ? n/9000 : 1; // number of miniruns in a session
    TFile *frun_slope, *fmini_slope; 
    TTree *trun_slope, *tmini_slope, *tburst_slope; // FIXME somehow, mini slope is different from burst slope
    if (nSlopes > 0) {
      const char *run_name = Form("%s/prexPrompt_pass2_%d.%03d.root", run_prefix, run, session);
      const char *mini_name = Form("%s/prexPrompt_%d_%03d_regress_postpan.root", mini_prefix, run, session);
      frun_slope = new TFile(run_name, "read");
      fmini_slope = new TFile(mini_name, "read");
      if (!frun_slope->IsOpen() || !fmini_slope->IsOpen()) {
        cerr << ERROR << "^^^^run: " << run << "\tsession: " << session << "\tcan't find some rootfiles for slope:\n"
          << "\t" << run_name << endl
          << "\t" << mini_name << ENDL;
        frun_slope->Close();
        fmini_slope->Close();
        continue;
      }
      const char *run_tree = "lrb_alldet"; 
      const char *mini_tree = "mini";
      const char *burst_tree = "burst_lrb_alldet";
      trun_slope = (TTree *) frun_slope->Get(run_tree);
      tmini_slope = (TTree *) fmini_slope->Get(mini_tree);
      tburst_slope = (TTree *) frun_slope->Get(burst_tree);
      if (!trun_slope || !tmini_slope || !tburst_slope) {
        cerr << ERROR << "^^^^run: " << run << "\tsession: " << session << "\tcan't read some trees: \n"
          << "\t" << run_tree << endl
          << "\t" << mini_tree << endl
          << "\t" << burst_tree << ENDL;
        continue;
      }
      trun_slope->SetBranchAddress("A", run_slope);
      trun_slope->SetBranchAddress("dA", run_slope_err);
      tmini_slope->SetBranchAddress("coeff", slope_buf);
      tmini_slope->SetBranchAddress("err_coeff", slope_err_buf);
      tburst_slope->SetBranchAddress("|statA", burst_slope);
      tburst_slope->SetBranchAddress("|statdA", burst_slope_err);
    }

    for (int mini=0; mini<Smini; mini++, minirun++) {
      cout << INFO << "aggregate minirun: " << mini << " of run: " << run << " session: " << session << ENDL;
      num_samples = (mini == Smini-1) ? n-9000*mini : 9000;
      for (int i=0; i<num_samples; i++, iok++) {
        for (string var : allVars) {
          double val = fVarValue[var][iok];
          mini_sum[var] += val;
          mini_sum2[var] += val * val;
          sum[var] += val;
          sum2[var] += val * val;
        }
      }
      for (string var : allVars) {
        mini_stat[var].mean = mini_sum[var]/num_samples;
        mini_stat[var].rms = sqrt((mini_sum2[var] - mini_sum[var]*mini_sum[var]/num_samples)/(num_samples-1));
        mini_stat[var].err = mini_stat[var].rms / sqrt(num_samples);
        mini_sum[var] = 0;
        mini_sum2[var] = 0;
      }
      if (nSlopes > 0) {
        trun_slope->GetEntry(mini);
        tmini_slope->GetEntry(mini);
        tburst_slope->GetEntry(mini);
        for (pair<string, string> slope : fSlopes) {
          string var = slope.first + '_' + slope.second;
          string mini_var = "reg_" + var;
          string burst_var = "burst_" + var;

          if (fRunArm[run] != 0) {  // single arm
            const char *arm = "r";
            const char *other_arm = "l";
            if (fRunArm[run] == 2) {
              arm = "l";
              other_arm = "r";
            }

            if (slope.first.find(Form("asym_us%s", other_arm))) {
              mini_stat[mini_var] = {0/0., 0/0., 0/0.};
              mini_stat[burst_var] = {0/0., 0/0., 0/0.};
              continue;
            }

            int pos;
            if ((pos = slope.first.find("_avg")) != string::npos)
              slope.first.replace(pos, 4, arm);
            else if  ((pos = slope.first.find("_dd")) != string::npos)
              slope.first.replace(pos, 3, arm);
          }
          mini_stat[mini_var].mean = slope_buf[fSlopeIndex[slope].first*cols+fSlopeIndex[slope].second];
          mini_stat[mini_var].err = slope_err_buf[fSlopeIndex[slope].first*cols+fSlopeIndex[slope].second];
          mini_stat[mini_var].rms = 0;
          mini_stat[burst_var].mean = burst_slope[fSlopeIndex[slope].second][burst_dv_index[slope.first]];
          mini_stat[burst_var].err = burst_slope_err[fSlopeIndex[slope].second][burst_dv_index[slope.first]];
          mini_stat[burst_var].rms = 0;
        }
      }
      if (update) {
        for (TBranch *b : mini_brs)
          b->Fill();
      } else 
        tout_mini->Fill();
    }
    if (nSlopes > 0) {  // average session-level slope values, weighted by 1/err²
      trun_slope->GetEntry(0);
      for (pair<string, string> slope : fSlopes) {
        string var = slope.first + '_' + slope.second;

        if (fRunArm[run] != 0) {  // single arm
          const char *arm = "r";
          const char *other_arm = "l";
          if (fRunArm[run] == 2) {
            arm = "l";
            other_arm = "r";
          }

          if (slope.first.find(Form("asym_us%s", other_arm)) != string::npos)
            continue;

          int pos;
          if ((pos = slope.first.find("_avg")) != string::npos)
            slope.first.replace(pos, 4, arm);
          else if  ((pos = slope.first.find("_dd")) != string::npos)
            slope.first.replace(pos, 3, arm);
        }

        double mean = run_slope[fSlopeIndex[slope].second][burst_dv_index[slope.first]];
        double err = run_slope_err[fSlopeIndex[slope].second][burst_dv_index[slope.first]];
        double weight = 1/pow(err, 2);
        sum[var] += weight*mean;
        sum_weight[var] += weight;
      }
      trun_slope->Delete();
      tmini_slope->Delete();
      tburst_slope->Delete();
      frun_slope->Close();
      fmini_slope->Close();
    }
  }

  for (string var : allVars) {
    stat[var].mean = sum[var]/N;
    stat[var].rms = sqrt((sum2[var] - sum[var]*sum[var]/N)/(N-1));
    stat[var].err = stat[var].rms / sqrt(N);
    sum[var] = 0;
    sum2[var] = 0;
  }
  if (nSlopes > 0) {
    for (pair<string, string> slope : fSlopes) {
      string var = slope.first + '_' + slope.second;
      if (sum_weight[var] == 0)
        stat[var] = {0/0., 0/0., 0/0.};
      else {
        stat[var].mean = sum[var]/sum_weight[var];
        stat[var].err = sqrt(1/sum_weight[var]);
        stat[var].rms = 0;
      }
    }
  }
  fout.cd();
  if (update) {
    tout_mini->Write("", TObject::kOverwrite);
    for (TBranch *b : run_brs)
      b->Fill();
    tout_run->Write("", TObject::kOverwrite);
  } else {
    tout_mini->Write();
    tout_run->Fill();
    tout_run->Write();
  }
  tout_mini->Delete();
  tout_run->Delete();
  fout.Close();
}

void TAggregate::AggregateRuns() {
  set<int> runs = fRuns;
  fSlugs.clear(); 
  nSlugs = 0;
  for (int r : runs) {
    cout << INFO << "aggregating run: " << r << ENDL;
    set<string> varBuf = fVars;
    vector<string> cusBuf = fCustoms;
    vector<pair<string, string>> slopeBuf = fSlopes;
		run=r; 
		fRuns.clear(); 
		fRuns.insert(run); 
    CheckVars();
    if (!(fVars.size() + fCustoms.size() + fSlopes.size())) {
      cerr << WARNING << "run: " << run << ". No new variables in update mode, skip it" << ENDL;
      fVars = varBuf;
      fCustoms = cusBuf;
      fSlopes = slopeBuf;
      continue;
    }
    if (fVars.size()) {
      cout << INFO << fVars.size() << " new variables: ";
      for (string var : fVars)
        cout << endl << "\t" << var;
      cout << ENDL;
    }
    if (fCustoms.size()) {
      cout << INFO << fCustoms.size() << " new custom variables: ";
      for (string var : fCustoms)
        cout << endl << "\t" << var;
      cout << ENDL;
    }
    if (fSlopes.size()) {
      cout << INFO << fSlopes.size() << " new slope variables: ";
      for (pair<string, string> slope : fSlopes)
        cout << endl << "\t" << slope.first << " : " << slope.second;
      cout << ENDL;
    }
    GetValues();
    ProcessValues();
    Aggregate();
    fVars = varBuf;
    fCustoms = cusBuf;
    fSlopes = slopeBuf;
  }
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
