#ifndef TREGRESS_H
#define TREGRESS_H

#include "TFile.h"
#include "TTree.h"
#include "TMatrixD.h"

#include "TRSbase.h"

class TRegress : public TRSbase
{
	private:
		int run;
		vector<string> IvNames;
		vector<string> DvNames;
		int niv, ndv;
		const char *burst_var = "BurstCounter";
		const char *prefix = "reg";

	public:
		TRegress();
		~TRegress() { cout << INFO << "end of regression" << ENDL; };
		void GetConfig(TConfig &fConf);
		void SetRegRun(int r);
		void CheckVars();
		void Regress();
		void RegressRuns();
};

TRegress::TRegress() :
	TRSbase()
{}

void TRegress::GetConfig(TConfig &fConf)
{
	TBase::GetConfig(fConf);

	IvNames.clear();
	DvNames.clear();
	IvNames = fConf.GetIv();
	DvNames = fConf.GetDv();
	niv = IvNames.size();
	ndv = DvNames.size();
}

void TRegress::SetRegRun(int r) 
{ 
  run=r; 
  fRuns.clear(); 
  fSlugs.clear(); 
  SetRuns({run}); 
  // CheckRuns();
  nSlugs = 0;
}

void TRegress::CheckVars()
{
	fVars.insert(burst_var);	// make sure it has the burst_var branch
	TBase::CheckVars();
	cout << INFO << niv << " iv variables:" << endl;
	for (string iv : IvNames)
		cout << "\t" << iv << endl;
	cout << ENDL;
	cout << INFO << ndv << " dv variables:" << endl;
	for (string dv : DvNames)
		cout << "\t" << dv << endl;
	cout << ENDL;
}

void TRegress::Regress()
{
	const int sessions = fRootFile[run].size();
	for (int s=0; s<sessions; s++)
	{
		const int N = fEntryNumber[run][s].size();
		if (0 == N)
			continue;

		TTree *tout_reg = new TTree("reg", "regressed variables");
		TTree *tout_slope = new TTree("slope", "regressed slopes");

		int npattern_buf;
		TMatrixD slope(ndv, niv);
		TMatrixD slope_err(ndv, niv);
		map<string, double> value;
		for (int i=0; i<ndv; i++)
		{
			string dv = DvNames[i];
			tout_reg->Branch(Form("reg_%s", dv.c_str()), &value[dv], Form("reg_%s/D", dv.c_str()));

			for (int j=0; j<niv; j++)
			{
				string var = dv + "_" + IvNames[j];
				tout_slope->Branch(var.c_str(), &slope[i][j], "mean/D");	
				tout_slope->Branch(Form("%s_err", var.c_str()), &slope_err[i][j], "err/D");	
			}
		}
		tout_slope->Branch("npattern", &npattern_buf, "npattern/I");

		const int Nmini = fVarValue[burst_var][N-1] + 1;
		cout << DEBUG << "number of miniruns in run " << run << ": " << Nmini << ENDL;
		TMatrixD X(niv, niv);
		TMatrixD Y(ndv, niv);
		double iv_sum[niv] = {0};
		double dv_sum[ndv] = {0};
		double iv_sum2[niv] = {0};
		double dv_sum2[ndv] = {0};
		double iv_mean[Nmini][niv] = {0};
		double dv_mean[Nmini][ndv] = {0};
		double iv_err[Nmini][niv] = {0};
		double dv_err[Nmini][ndv] = {0};
		int npattern[Nmini] = {0};
		pair<int, int> minirange[Nmini] = {{0, 0}};

		// first pass, seperate miniruns based on BurstCounter;
		// get entry range for each minirun
		// get mean value for each minirun 
		int mini = 0;
		int mini_start = 0;
		for (int n=0; n<N; n++)
		{
			if (mini != fVarValue[burst_var][n])	
			{	// next burst
				for (int i=0; i<niv; i++) 
				{
					iv_mean[mini][i] = iv_sum[i]/npattern[mini];
					// iv_err[mini][i] = sqrt( (iv_sum2[i] - npattern[mini]*pow(iv_mean[mini][i], 2))	/ 
					// 												npattern[mini]);
					iv_sum[i] = 0;
					// iv_sum2[i] = 0;
				}
				for (int i=0; i<ndv; i++)
				{
					dv_mean[mini][i] = dv_sum[i]/npattern[mini];
					dv_err[mini][i] = sqrt( (dv_sum2[i] - npattern[mini]*pow(dv_mean[mini][i], 2))	/ 
																	npattern[mini]);
					dv_sum[i] = 0;
					dv_sum2[i] = 0;
				}
				minirange[mini] = {mini_start, n-1};
				mini_start = n;
				mini++;
				npattern[mini] = 0;
			}

			for (int i=0; i<niv; i++)
			{
				iv_sum[i] += fVarValue[IvNames[i]][n];
				// iv_sum2[i] += pow(fVarValue[IvNames[i]][n], 2);
			}
			for (int i=0; i<ndv; i++)
			{
				dv_sum[i] += fVarValue[DvNames[i]][n];
				dv_sum2[i] += pow(fVarValue[DvNames[i]][n], 2);
			}
			npattern[mini]++;
		}
		// for the last minirun
		for (int i=0; i<niv; i++) 
		{
			iv_mean[mini][i] = iv_sum[i]/npattern[mini];
			// iv_err[mini][i] = sqrt( (iv_sum2[i] - npattern[mini]*pow(iv_mean[mini][i], 2))	/ 
			// 												npattern[mini]);
			iv_sum[i] = 0;
			// iv_sum2[i] = 0;
		}
		for (int i=0; i<ndv; i++)
		{
			dv_mean[mini][i] = dv_sum[i]/npattern[mini];
			dv_err[mini][i] = sqrt( (dv_sum2[i] - npattern[mini]*pow(dv_mean[mini][i], 2))	/ 
															npattern[mini]);
			dv_sum[i] = 0;
			dv_sum2[i] = 0;
		}
		minirange[mini] = {mini_start, N-1};

		// second pass, to calculate the matric: X_ij and Y_ij
		for (mini=0; mini<Nmini; mini++)
		{
			for (int r=0; r<niv; r++)
			{
				for (int c=r; c<niv; c++)
				{
					for(int n=minirange[mini].first; n <= minirange[mini].second; n++)
					{
						X[r][c] += (fVarValue[IvNames[r]][n] - iv_mean[mini][r]) * 
											 (fVarValue[IvNames[c]][n] - iv_mean[mini][c]);
					}
					if (c != r)
						X[c][r] = X[r][c];
				}
			}

			TMatrixD Xinv(niv, niv);
			double det = X.Determinant();
			if (abs(det) < 1e-38)
			{
				cerr << ERROR << "singular X matrix in minirun: " << mini << ", 0 value will be used" << ENDL;
				for (int i=0; i<niv; i++)
					cerr << IvNames[i] << "\t";
				cerr << endl;
				X.Print();

				for (int i=0; i<niv; i++)
					for (int j=0; j<niv; j++)
						Xinv(i, j)  = 0;
			}
			else
			{
				Xinv = X;
				Xinv.Invert();
			}

			for (int r=0; r<ndv; r++)
			{
				for (int c=0; c<niv; c++)
				{
					for(int n=minirange[mini].first; n <= minirange[mini].second; n++)
					{
						Y[r][c] += (fVarValue[DvNames[r]][n] - dv_mean[mini][r]) * 
											 (fVarValue[IvNames[c]][n] - iv_mean[mini][c]);
					}
				}
			}

			slope = Y * Xinv;
			for (int r=0; r<ndv; r++)
			{
				for (int c=0; c<niv; c++)
				{
					slope_err[r][c] = dv_err[mini][r]*sqrt(Xinv[c][c]);
				}
			}
			// fill the slope tree
      npattern_buf = npattern[mini];
			tout_slope->Fill();
			
			// Correction
			for(int n=minirange[mini].first; n <= minirange[mini].second; n++)
			{
				for (int r=0; r<ndv; r++)
				{
					double cor = 0;
					for (int c=0; c<niv; c++)
					{
						cor += slope(r, c)*(fVarValue[IvNames[c]][n]/*-iv_mean(c)*/);
					}
					fVarValue[DvNames[r]][n] -= cor;
				}
			}
		}
		for(int n=0; n < N; n++)
		{
			for (string dv : DvNames)
				value[dv] = fVarValue[dv][n];
			tout_reg->Fill();
		}

		TFile fout(Form("%s/%s_%d.%03d.root", out_dir, prefix, run, s), "recreate");
		fout.cd();
		tout_reg->Write();
		tout_slope->Write();
		fout.Close();
		delete tout_reg;
		delete tout_slope;
	}
}

void TRegress::RegressRuns()
{
	set<int> runs = fRuns;
	for (int r : runs)
	{
		cout << INFO << "regress run: " << r << ENDL;
		SetRegRun(r);
		GetValues();
		Regress();
	}
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
