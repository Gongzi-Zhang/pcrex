#ifndef TAGGREGATE_H
#define TAGGREGATE_H

#include <iostream>
#include "TBase.h"

struct STAT { double mean, err, rms; };

class TAggregate : public TBase 
{
	private:
		const char * out_dir = "rootfiles";
		// const char * out_pattern = "agg_minirun_xxxx_???";
	public:
		TAggregate(const char * config_file, const char * run_list = NULL);
		~TAggregate() { cout << INFO << "end of TAggregate" << ENDL; };
		void SetOutDir(const char *dir);
		void CheckOutDir();
		void Aggregate();
};

TAggregate::TAggregate(const char * config_file, const char * run_list) :
	TBase(config_file, run_list)
{}

void TAggregate::SetOutDir(const char *dir) 
{
	if (!dir) {
		cerr << FATAL << "Null out dir value!" << ENDL;
		exit(104);
	}
	out_dir = dir;
}

void TAggregate::CheckOutDir() 
{
	struct stat info;
	if (stat(out_dir, &info) != 0) {
		cerr << FATAL << "can't access specified dir: " << out_dir << ENDL;
		exit(104);
	} else if ( !(info.st_mode & S_IFDIR)) {
		cerr << FATAL << "not a dir: " << out_dir << ENDL;
		exit(104);
	}
	/*
	if (stat(out_dir, &info) != 0) {
		cerr << WARNING << "Out dir doesn't exist, create it." << ENDL;
		int status = mkdir(out_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (status != 0) {
			cerr << ERROR << "Can't create specified dir: " << out_dir << ENDL;
			exit(44);
		}
	}
	*/
}

void TAggregate::Aggregate()
{
	unsigned int num_samples = 0;
	map<string, double> mini_sum, sum;
	map<string, double> mini_sum2, sum2;
	map<string, STAT> mini_stat, stat;
	unsigned int mini;
	unsigned int N;

  char hostname[32];
  gethostname(hostname, 32);
  const char *run_prefix = "./", *mini_prefix = "./";
  if (Contain(hostname, "aonl") || Contain(hostname, "adaq")) {
    run_prefix  = "/adaqfs/home/apar/PREX/prompt/japanOutput/";
    mini_prefix = "/adaqfs/home/apar/PREX/prompt/results/"; 
  } else if (Contain(hostname, "ifarm")) {
    run_prefix  = "/lustre/expphy/volatile/halla/parity/crex-respin2/japanOutput/";
    mini_prefix = "/lustre/expphy/volatile/halla/parity/crex-respin2/postpan_respin/";
  }

  int iok = 0;
  for (int run : fRuns) {
    const int nburst_dv = (run <= 6464) ? 48 : 56;
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

    const size_t sessions = fRootFile[run].size();
    for (size_t session=0; session < sessions; session++) {
      bool update=true;	// update tree: add new branches
      vector<TBranch *> mini_brs, run_brs;

      TFile fout(Form("%s/agg_minirun_%d_%03ld.root", out_dir, run, session), "update");
      TTree *tout_mini = (TTree *) fout.Get("mini");
      TTree *tout_run	 = (TTree *) fout.Get("run");
      if (!tout_mini) {
        tout_mini = new TTree("mini", "mini run average value");
        tout_mini->Branch("run", &run, "run/i");
        tout_mini->Branch("minirun", &mini, "minirun/i");
        tout_mini->Branch("num_samples", &num_samples, "num_samples/i");

        tout_run = new TTree("run", "run average value");
        tout_run->Branch("run", &run, "run/i");
        tout_run->Branch("minirun", &mini, "minirun/i");
        tout_run->Branch("num_samples", &N, "num_samples/i");

        update=false;
      } 

      for (string custom : fCustoms)  // combine solo variables
        fVars.insert(custom);

      for (string var : fVars) {
        if (tout_mini->GetBranch(var.c_str())) {
          fVars.erase(var);
          continue;
        } 
        TBranch *b = tout_mini->Branch(var.c_str(), &mini_stat[var], "mean/D:err/D:rms/D");
        TBranch *b1 = tout_run->Branch(var.c_str(), &stat[var], "mean/D:err/D:rms/D");
        if (update) {
          mini_brs.push_back(b);
          run_brs.push_back(b1);
        }
      }
      for (vector<pair<string, string>>::iterator it=fSlopes.begin(); it != fSlopes.end(); ) { // slope
        string var = it->first + '_' + it->second;
        if (tout_mini->GetBranch(var.c_str())) {
          it = fSlopes.erase(it);
          continue;
        }
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
        it++;
      }
      if (update && fVars.size() == 0 && fSlopes.size() == 0) {
        cerr << ERROR << "no new variables in update mode" << ENDL;
        exit(0);
      }

      mini = 0;
      N	= fEntryNumber[run][session].size();
      const int Nmini = N>9000 ? N/9000 : 1;
      if (update && Nmini != tout_mini->GetEntries()) {
        cerr << ERROR << "Unmatch # of miniruns: "
          << "\told: " << tout_mini->GetEntries() 
          << "\tnew: " << Nmini << ENDL;
      }
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
      for (mini=0; mini<Nmini; mini++) {
        cout << INFO << "aggregate minirun: " << mini << " of run: " << run << " session: " << session << ENDL;
        num_samples = (mini == Nmini-1) ? N-9000*mini : 9000;
        for (int n=0; n<num_samples; n++, iok++) {
          for (string var : fVars) {
            // FIXME: unit
            double val = fVarValue[var][iok];
            mini_sum[var] += val;
            mini_sum2[var] += val * val;
            sum[var] += val;
            sum2[var] += val * val;
          }
        }
        for (string var : fVars) {
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

      for (string var : fVars) {
        stat[var].mean = sum[var]/N;
        stat[var].rms = sqrt((sum2[var] - sum[var]*sum[var]/N)/(N-1));
        stat[var].err = stat[var].rms / sqrt(N);
        sum[var] = 0;
        sum2[var] = 0;
      }
      if (nSlopes > 0) {
        trun_slope->GetEntry(0);
        for (pair<string, string> slope : fSlopes) {
          string var = slope.first + '_' + slope.second;
          stat[var].mean = run_slope[fSlopeIndex[slope].second][burst_dv_index[slope.first]];
          stat[var].err = run_slope_err[fSlopeIndex[slope].second][burst_dv_index[slope.first]];
          stat[var].rms = 0;
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
      if (nSlopes > 0) {
        trun_slope->Delete();
        tmini_slope->Delete();
        tburst_slope->Delete();
        frun_slope->Close();
        fmini_slope->Close();
      }
      fout.Close();
    }
  }
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
