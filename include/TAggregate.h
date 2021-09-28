#ifndef TAGGREGATE_H
#define TAGGREGATE_H

#include <iostream>
#include <glob.h>
#include "TFile.h"
#include "TTree.h"
#include "TRSbase.h"

struct STAT { double mean, err, rms; };
string cmethods[] = {"reg", "lrb"};	// correction methods; FIXME: bmw

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

  for (vector<string>::iterator it=fSolos.begin(); it!=fSolos.end();) {
    if (trun->GetBranch(it->c_str())) {
      it = fSolos.erase(it);
      continue;
    } 
		it++;
  }
  for (vector<string>::iterator it=fCustoms.begin(); it!=fCustoms.end(); ) {
    if (trun->GetBranch(it->c_str())) {
      it = fCustoms.erase(it);
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
	map<string, STAT> mini_stat, stat;
	unsigned int minirun = 0; // number of miniruns
	unsigned int N = 0;   // total number of mul patterns

  int iok = 0;

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

  set<string> allVars; // fSolos + fCustoms
  for (string solo : fSolos)
		allVars.insert(solo);
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
  for (int session=0; session < sessions; session++) 
	{
    int n	= fEntryNumber[run][session].size();
    if (n == 0) 
		{
      cerr << WARNING << "no valid minirun in run: " << run << ", session: " << session << ENDL;
      continue;
    }
    int Smini = n>9000 ? n/9000 : 1; // number of miniruns in a session
    for (int mini=0; mini<Smini; mini++, minirun++) 
		{
      cout << INFO << "aggregate minirun: " << mini << " of run: " << run << " session: " << session << ENDL;
      num_samples = (mini == Smini-1) ? n-9000*mini : 9000;
      for (int i=0; i<num_samples; i++, iok++) {
        for (string var : allVars) {
          double val = fVarValue[var][iok];
          mini_sum[var] += val;
          mini_sum2[var] += val * val;
        }
      }
      for (string var : allVars) 
			{
        mini_stat[var].mean = mini_sum[var]/num_samples;
				if (1 == num_samples)
					mini_stat[var].rms = 0;
				else
					mini_stat[var].rms = sqrt((mini_sum2[var] - mini_sum[var]*mini_sum[var]/num_samples)/(num_samples-1));
        mini_stat[var].err = mini_stat[var].rms / sqrt(num_samples);
				sum[var] += mini_sum[var];
				sum2[var] += mini_sum2[var];
        mini_sum[var] = 0;
        mini_sum2[var] = 0;
      }
      if (update) {
        for (TBranch *b : mini_brs)
          b->Fill();
      } else 
        tout_mini->Fill();
    }
  }

  for (string var : allVars) {
    stat[var].mean = sum[var]/N;
		if (1 == N)
			stat[var].rms = 0;
		else
			stat[var].rms = sqrt((sum2[var] - sum[var]*sum[var]/N)/(N-1));
    stat[var].err = stat[var].rms / sqrt(N);
    sum[var] = 0;
    sum2[var] = 0;
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
	vector<string> soloBuf = fSolos;
	vector<string> cusBuf = fCustoms;
  for (int r : runs) {
    cout << INFO << "aggregating run: " << r << ENDL;
		SetAggRun(r);
    CheckVars();
		fVars.clear();
    if (!(fSolos.size() + fCustoms.size())) {
      cerr << WARNING << "run: " << run << ". No new variables in update mode, skip it" << ENDL;
      fSolos = soloBuf;
      fCustoms = cusBuf;
      continue;
    }
    if (fSolos.size()) {
      cout << INFO << fSolos.size() << " new variables: " << ENDL;
      for (string solo : fSolos)
			{
        cout << "\t" << solo << endl;
				fVars.insert(solo);
			}
    }
    if (fCustoms.size()) {
      cout << INFO << fCustoms.size() << " new custom variables: " << ENDL;
      for (string custom : fCustoms)
			{
        cout << "\t" << custom << endl;
				for (string var : GetVariables(fCustomDef[custom]))
					fVars.insert(var);
			}
    }
    int n = GetValues();
		if (0 == n)
		{
			cerr << WARNING << "run: " << r << ". No event for aggregation." << ENDL;
			continue;
		}
    ProcessValues();
    Aggregate();
    fCustoms = cusBuf;
  }
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
