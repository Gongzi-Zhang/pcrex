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

		// slopes
		map<string, vector<STAT>> fSlopeValue;
		map<pair<string, string>, pair<int, int>> fSlopeIndex;
		int rows, cols;

		// const char * out_pattern = "agg_minirun_xxxx_???";
	public:
		TAggregate();
		~TAggregate() { cout << INFO << "end of TAggregate" << ENDL; };
    void SetAggRun(int r);
		void CheckSlopeVars();
    void CheckVars();
		void GetSlopeValues();
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

void TAggregate::CheckSlopeVars() 
{
	if (!nSlopes)
		return;

  srand(time(NULL));
  int s = rand() % nRuns;
  set<int>::const_iterator it_r=fRuns.cbegin();
  for(int i=0; i<s; i++)
    it_r++;

	while (it_r != fRuns.cend())
	{
		int r = *it_r;
		const char *file_name = Form("/lustre19/expphy/volatile/halla/parity/crex-respin2/postpan_respin/prexPrompt_%d_000_regress_postpan.root", r);
		cerr << INFO << "use run: " << file_name << " to check slope variables" << ENDL;
		TFile * f_rootfile = new TFile(file_name, "read");
		if (!f_rootfile->IsOpen()) {
			cerr << ERROR << run << "-" << r << ": can't read root file: " << file_name << ENDL;
			f_rootfile->Close();
			exit(44);
		}

		vector<TString> *l_iv, *l_dv;
		l_iv = (vector<TString>*) f_rootfile->Get("IVNames");
		l_dv = (vector<TString>*) f_rootfile->Get("DVNames");
		if (l_iv == NULL || l_dv == NULL){
			cerr << WARNING << "run-" << r << " ^^^^ can't read IVNames or DVNames in root file: " 
					 << file_name << ", skip this run." << ENDL;
			f_rootfile->Close();
			it_r++;
			if (it_r == fRuns.cend())
				it_r = fRuns.cbegin();
		}

		rows = l_dv->size();
		cols = l_iv->size();
		for (vector<pair<string, string>>::iterator it=fSlopes.begin(); it != fSlopes.end(); it++) {
			string dv = it->first;
			string iv = it->second;
			vector<TString>::const_iterator it_dv = find(l_dv->cbegin(), l_dv->cend(), dv);
			vector<TString>::const_iterator it_iv = find(l_iv->cbegin(), l_iv->cend(), iv);
			if (it_dv == l_dv->cend()) 
			{
				cerr << ERROR << "Invalid dv name for slope: " << dv << ENDL;
				cout << DEBUG << "List of valid dv names:" << ENDL;
				for (vector<TString>::const_iterator it = l_dv->cbegin(); it != l_dv->cend(); it++) 
					cout << "\t" << (*it).Data() << endl;
				exit(44);
			}
			if (it_iv == l_iv->cend()) 
			{
				cerr << ERROR << "Invalid iv name for slope: " << iv << ENDL;
				cout << DEBUG << "List of valid dv names:" << ENDL;
				for (vector<TString>::const_iterator it = l_iv->cbegin(); it != l_iv->cend(); it++) 
					cout << "\t" << (*it).Data() << endl;
				exit(44);
			}
			fSlopeIndex[*it] = make_pair(it_dv-l_dv->cbegin(), it_iv-l_iv->cbegin());
		}
		f_rootfile->Close();
		break;
	}
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
  for (vector<pair<string, string>>::iterator it=fSlopes.begin(); it != fSlopes.end(); ) { 
    string var = "reg_" + it->first + '_' + it->second;	// check only reg slope
    if (trun->GetBranch(var.c_str())) {
      it = fSlopes.erase(it);
      continue;
    }
    it++;
  }
  trun->Delete();
  fout.Close();
}

void TAggregate::GetSlopeValues()
{
	if (nSlopes == 0)
		return;

  char hostname[32];
  gethostname(hostname, 32);
  const char *lrb_prefix = "./", *reg_prefix = "./";
  int nlrb_dv = 80;
  if (Contain(hostname, "aonl") || Contain(hostname, "adaq")) {
    lrb_prefix  = "/adaqfs/home/apar/PREX/prompt/japanOutput/";
    reg_prefix = "/adaqfs/home/apar/PREX/prompt/results/"; 
  } else if (Contain(hostname, "ifarm")) {
    lrb_prefix  = "/lustre19/expphy/volatile/halla/parity/crex-respin2/japanOutput/";
    reg_prefix = "/lustre19/expphy/volatile/halla/parity/crex-respin2/postpan_respin/";
  }

  // double lrb_run[5][nlrb_dv], lrb_run_err[5][nlrb_dv];
  double lrb_mini_buf[5][nlrb_dv], lrb_mini_err_buf[5][nlrb_dv];
	double *reg_mini_buf = new double[rows*cols];
	double *reg_mini_err_buf = new double[rows*cols];
  map<string, int> lrb_dv_index;
  for (vector<pair<string, string>>::iterator it_s = fSlopes.begin(); it_s != fSlopes.end(); it_s++) 
	{	// dv device index in lrb slope device list
		lrb_dv_index[it_s->first] = -1;
    vector<string>::iterator it = find(LRB_DV.begin(), LRB_DV.end(), it_s->first);
    if (it != LRB_DV.end()) 
      lrb_dv_index[it_s->first] = it-LRB_DV.begin();
    else 
      cerr << WARNING << "^^^^run: " << run << "\tno such device: " << it_s->first << " in lrb slope device list" << ENDL;
  }

	TFile *flrb, *freg; 
	TTree *treg_mini, *tlrb_mini, *tlrb_run;
  const int sessions = fRootFile[run].size();
	for (int session=0; session < sessions; session++) {
		const char *lrb_name = Form("%s/prexPrompt_pass2_%d.%03d.root", lrb_prefix, run, session);
		const char *reg_name = Form("%s/prexPrompt_%d_%03d_regress_postpan.root", reg_prefix, run, session);
		flrb = new TFile(lrb_name, "read");
		freg = new TFile(reg_name, "read");
		if (!flrb->IsOpen() || !freg->IsOpen()) {
			cerr << ERROR << "^^^^run: " << run << "\tsession: " << session << "\tcan't find some rootfiles for slope:\n"
				<< "\t" << lrb_name << endl
				<< "\t" << reg_name << ENDL;
			flrb->Close();
			freg->Close();
			continue;
		}
		const char *reg_mini_tree = "mini";
		const char *lrb_mini_tree = "burst_lrb_alldet";
		// const char *lrb_run_tree = "lrb_alldet"; 
		treg_mini = (TTree *) freg->Get(reg_mini_tree);
		tlrb_mini = (TTree *) flrb->Get(lrb_mini_tree);
		// tlrb_run = (TTree *) flrb->Get(lrb_run_tree);
		if (!treg_mini || !tlrb_mini) {
			cerr << ERROR << "^^^^run: " << run << "\tsession: " << session << "\tcan't read some trees: \n"
				<< "\t" << reg_mini_tree << endl
				<< "\t" << lrb_mini_tree << ENDL;
				// << "\t" << lrb_run_tree << endl
			continue;
		}
		treg_mini->SetBranchAddress("coeff", reg_mini_buf);
		treg_mini->SetBranchAddress("err_coeff", reg_mini_err_buf);
		tlrb_mini->SetBranchAddress("|statA", lrb_mini_buf);
		tlrb_mini->SetBranchAddress("|statdA", lrb_mini_err_buf);
		// tlrb_run_tree->SetBranchAddress("A", run);
		// tlrb_run_tree->SetBranchAddress("dA", run_err);

		const int Nmini = treg_mini->GetEntries();
		for (int mini=0; mini<Nmini; mini++)
		{
			treg_mini->GetEntry(mini);
			tlrb_mini->GetEntry(mini);
			for (pair<string, string> slope : fSlopes) 
			{
				string var = slope.first + '_' + slope.second;
				string reg_var = "reg_" + var;
				string lrb_var = "lrb_" + var;

				if (fRunArm[run] != 0) {  // single arm
					const char *arm = "r";
					const char *other_arm = "l";
					if (fRunArm[run] == 2) {
						arm = "l";
						other_arm = "r";
					}

					if (slope.first.find(Form("asym_us%s", other_arm))) {
						fSlopeValue[reg_var].push_back({0/0., 0/0., 0/0.});
						fSlopeValue[lrb_var].push_back({0/0., 0/0., 0/0.});
						continue;
					}

					int pos;
					if ((pos = slope.first.find("_avg")) != string::npos)
						slope.first.replace(pos, 4, arm);
					else if  ((pos = slope.first.find("_dd")) != string::npos)
						slope.first.replace(pos, 3, arm);
				}
				fSlopeValue[reg_var].push_back(
					{ reg_mini_buf[fSlopeIndex[slope].first*cols+fSlopeIndex[slope].second],
						reg_mini_err_buf[fSlopeIndex[slope].first*cols+fSlopeIndex[slope].second],
						0/0.} );
				if (fSlopeIndex[slope].second < 5 && lrb_dv_index[slope.first] != -1)
					fSlopeValue[lrb_var].push_back(
						{ lrb_mini_buf[fSlopeIndex[slope].second][lrb_dv_index[slope.first]],
							lrb_mini_err_buf[fSlopeIndex[slope].second][lrb_dv_index[slope.first]],
							0/0.} );
				else
					fSlopeValue[lrb_var].push_back({0/0., 0/0., 0/0.});
			}
		}

		treg_mini->Delete();
		tlrb_mini->Delete();
		// tlrb_run->Delete();
		flrb->Close();
		freg->Close();
	}
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
  map<string, double> sum_weight;	// for slopes
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
  for (pair<string, string> slope : fSlopes) { 
    string var = slope.first + '_' + slope.second;
    string reg_var = "reg_" + var;
    string lrb_var = "lrb_" + var;
    TBranch *b_reg = tout_mini->Branch(reg_var.c_str(), &mini_stat[reg_var], "mean/D:err/D:rms/D");
    TBranch *b_lrb = tout_mini->Branch(lrb_var.c_str(), &mini_stat[lrb_var], "mean/D:err/D:rms/D");
    TBranch *b1_reg = tout_run->Branch(reg_var.c_str(), &stat[reg_var],	"mean/D:err/D:rms/D");
    TBranch *b1_lrb = tout_run->Branch(lrb_var.c_str(), &stat[lrb_var],	"mean/D:err/D:rms/D");
    if (update) {
      mini_brs.push_back(b_reg);
      mini_brs.push_back(b_lrb);
      run_brs.push_back(b1_reg);
      run_brs.push_back(b1_lrb);
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
			for (pair<string, string> slope : fSlopes) {
				for (string method : cmethods)
				{
					string var = method + '_' + slope.first + '_' + slope.second;
					mini_stat[var] = fSlopeValue[var][mini];
					double weight = 1/pow(mini_stat[var].err, 2);
					sum[var] += weight*mini_stat[var].mean;
					sum_weight[var] += weight;
				}
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
	for (pair<string, string> slope : fSlopes) 
	{
		for (string method : cmethods)
		{
			string var = method + '_' + slope.first + '_' + slope.second;
			if (sum_weight[var] == 0)
				stat[var] = {0/0., 0/0., 0/0.};
			else {
				stat[var].mean = sum[var]/sum_weight[var];
				stat[var].err = sqrt(1/sum_weight[var]);
				stat[var].rms = 0;
			}
			fSlopeValue[var].clear();
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
	vector<string> soloBuf = fSolos;
	vector<string> cusBuf = fCustoms;
	vector<pair<string, string>> slopeBuf = fSlopes;
  for (int r : runs) {
    cout << INFO << "aggregating run: " << r << ENDL;
		SetAggRun(r);
    CheckVars();
		fVars.clear();
    if (!(fSolos.size() + fCustoms.size() + fSlopes.size())) {
      cerr << WARNING << "run: " << run << ". No new variables in update mode, skip it" << ENDL;
      fSolos = soloBuf;
      fCustoms = cusBuf;
      fSlopes = slopeBuf;
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
    if (fSlopes.size()) {
      cout << INFO << fSlopes.size() << " new slope variables: " << ENDL;
      for (pair<string, string> slope : fSlopes)
        cout << "\t" << slope.first << " : " << slope.second << endl;
    }
    int n = GetValues();
		if (0 == n)
		{
			cerr << WARNING << "run: " << r << ". No event for aggregation." << ENDL;
			continue;
		}
		GetSlopeValues();
    ProcessValues();
    Aggregate();
    fCustoms = cusBuf;
    fSlopes = slopeBuf;
  }
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
