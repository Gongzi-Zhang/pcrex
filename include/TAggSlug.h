#ifndef TAGGSLUG_H
#define TAGGSLUG_H

#include <iostream>
#include "TRSbase.h"

typedef struct _STAT { double mean, err, rms; } STAT;

class TAggSlug : public TRSbase {
	private:
		int slug;
		// const char *var_weight = "reg_asym_us_avg";
		set<string> fBrVars;
		// const char * out_pattern = "agg_minirun_xxxx_???";
	public:
		TAggSlug();
		~TAggSlug() { cout << INFO << "end of TAggSlug" << ENDL; };
    void GetConfig(const TConfig fConf);
		void SetAggSlug(const int s);
		void AggSlug();
		void AggSlugs();
};

TAggSlug::TAggSlug() :
	TRSbase()
{
  dir     = "rootfiles/";
  pattern = "agg_mini_xxxx.???.root";
  tree    = "mini";
}

void TAggSlug::GetConfig(const TConfig fConf) {
  TBase::GetConfig(fConf);
	if (fVars.find("num_samples") != fVars.end())
		fVars.erase("num_samples");
	for (string var : fVars) {
		string v = var.substr(0, var.find('.'));
    // fVars.erase(v);
		fVars.insert(v + ".mean");
		// fVars.insert(v + ".err");
		fVars.insert(v + ".rms");
		fBrVars.insert(v);
	}
	fVars.insert("num_samples");	// num_samples must be there for calculation of variance
	// fVars.insert(var_weight);	// make sure weight variable is always there
}

void TAggSlug::SetAggSlug(const int s) { 
  slug=s;
  fSlugs.clear();
  fRuns.clear();
  SetSlugs({slug});
  CheckRuns();
  nSlugs = fSlugs.size();
}

void TAggSlug::AggSlug()
{
	unsigned int num_runs = fRuns.size();
	unsigned int num_miniruns = nOk;
	unsigned int num_samples = 0;
	map<string, double> sum;
	// map<string, STAT> sum2;
	map<string, STAT> stat;

	bool update=true;	// update tree: add new branches
	TFile fout(Form("%s/agg_slug_%d.root", out_dir, slug), "update");
	TTree *tout = (TTree *) fout.Get("slug");
	if (!tout) {
		tout = new TTree("slug", "slug average value (weighted by det. width)");
		tout->Branch("slug", &slug, "slug/i");
		tout->Branch("num_runs", &num_runs, "num_runs/i");
		tout->Branch("num_miniruns", &num_miniruns, "num_miniruns/i");
		tout->Branch("num_samples", &num_samples, "num_samples/i");
		update=false;
	} 
	
	vector<TBranch *> brs;
	for (string custom : fCustoms)  // combine solo variables
		fVars.insert(custom);

	for (string var : fBrVars) {
		if (tout->GetBranch(var.c_str())) {
			fBrVars.erase(var);
			continue;
		} 
		TBranch *b = tout->Branch(var.c_str(), &stat[var], "mean/D:err/D:rms/D");
		if (update) {
			brs.push_back(b);
		}
	}
	if (update) {
		unsigned int n = 0;
		tout->SetBranchAddress("num_runs", &n);
		tout->GetEntry(0);
		if (num_runs != n) {
			cerr << ERROR << "Unmatch # of runs: "
				<< "\told: " << n
				<< "\tnew: " << num_runs << ENDL;
			exit(4);
		} else if (fBrVars.size() == 0) {
			cerr << ERROR << "no new variables in update mode" << ENDL;
			exit(0);
		}
	}
	map<string, double> sum_weight;
  map<string, double> n1; // so variables may have different num_samples
	map<string, double> m1;	// mean
	map<string, double> dev1;	// deviation
	for (string var : fBrVars) {
		sum[var] = 0;
    sum_weight[var] = 0;
    n1[var] = 0;
		m1[var] = 0;
		dev1[var] = 0;
	}
  for (int i=0; i<nOk; i++) {
		double n2 = fVarValue["num_samples"][i];
		for (string var : fBrVars) {
			double m2 = fVarValue[var+".mean"][i];
			double rms2 = fVarValue[var+".rms"][i];
      // double err2 = rms2/sqrt(n2);
      if (std::isnan(m2) && std::isnan(rms2)) // single arm nan value
        continue;
			double var2 = pow(rms2, 2);  // variance
      double dev2 = (n2-1)*var2;  // deviation
      double weight = n2/var2;    // w = 1/pow(err, 2)
      sum_weight[var] += weight;
			sum[var] += m2*weight; // weighted mean
			double delta = m2 - m1[var];
			m1[var] += n2/(n1[var]+n2)*delta;
			dev1[var] = dev1[var] + dev2 + n1[var]*n2/(n1[var]+n2)*pow(delta, 2);
      n1[var] += n2;
		}
		num_samples += n2;
	}
	for (string var : fBrVars) {
    if (sum_weight[var] == 0)
      stat[var] = {0/0., 0/0., 0/0.};
    else {
      stat[var].mean = sum[var]/sum_weight[var];
      stat[var].err = sqrt(1/sum_weight[var]);
      stat[var].rms = sqrt(dev1[var]/(n1[var]-1));
    }
	}
	fout.cd();
	if (update) {
		for (TBranch *b : brs)
			b->Fill();
		tout->Write("", TObject::kOverwrite);
	} else {
		tout->Fill();
		tout->Write();
	}
	tout->Delete();
	fout.Close();
}

void TAggSlug::AggSlugs() {
  set<int> slugs = fSlugs;
  for (int s : slugs) {
    cout << INFO << "aggregating slug: " << s << ENDL;
    SetAggSlug(s);
    GetValues();
    AggSlug();
  }
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
