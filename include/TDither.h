#ifndef TDITHER_H
#define TDITHER_H

#include "TFile.h"
#include "TTree.h"
#include "TMatrixD.h"
#include "TMatrixDSym.h"
#include "TMatrixDSymEigen.h"

#include "TRSbase.h"

/* Calculate the sensitivity and dithering slope */

typedef struct _cov_ 
{
	double cov = 0;
	double xmean = 0;
	double ymean = 0;
	double n = 0;

	void update(double x, double y) 
	{
		cov += n/(n+1)*(x - xmean)*(y-ymean);
		n++;
		xmean += (x-xmean)/n;
		ymean += (y-ymean)/n;
	}
} COVARIANCE;

class TDither : public TRSbase
{
	private:
		int run;
		const char* fCycleVar = "bmwcycnum";
		const char* fBmwObjVar = "bmwobj";
		const char* fPrefix = "dit_slope";
		vector<string> fCoilNames;
		vector<string> fDetNames;
		vector<string> fMonNames;
		int fNcoil, fNdet, fNmon;
		vector<pair<int, int>> fCycleRange;

	public:
		TDither();
		~TDither() { cout << INFO << "end of dithering analysis" << ENDL; };
		void GetConfig(TConfig &fConf);
		void SetDitRun(int r);
		void CheckVars();
		void Correct();
		void CalcSlope();
		void DitherRuns();

		void	 GetCycleRange(const int);
};

TDither::TDither() :
	TRSbase()
{}

void TDither::GetConfig(TConfig &fConf)
{
	TBase::GetConfig(fConf);

	fDetNames.clear();
	fMonNames.clear();
	fCoilNames.clear();
	fDetNames = fConf.GetVector("dets");
	fMonNames = fConf.GetVector("mons");
	fCoilNames = fConf.GetVector("coils");
	fNdet = fDetNames.size();
	fNmon = fMonNames.size();
	fNcoil = fCoilNames.size();
}

void TDither::SetDitRun(int r) 
{ 
  run=r; 
  fRuns.clear(); 
  fSlugs.clear(); 
  SetRuns({run}); 
  CheckRuns();
  nSlugs = 0;
}

void TDither::GetCycleRange(const int s=0)
{
	fCycleRange.clear();
	int offset = 0;
	for (int i=0; i<s; i++)
		offset += fEntryNumber[run][i].size();
	
	int cycle_start = offset;
	const int N = offset + fEntryNumber[run][s].size();
	fCycleRange.push_back({offset, N-1});
	int cycle = fVarValue[fCycleVar][offset];
	for (int n=offset; n<N; n++)
	{
		if (cycle != fVarValue[fCycleVar][n])
		{
			fCycleRange.push_back({cycle_start, n-1});
			cycle_start = n;
			cycle = fVarValue[fCycleVar][n];
		}
	}
	fCycleRange.push_back({cycle_start, N-1});
}

void TDither::CheckVars()
{
	fVars.insert(fCycleVar);	// make sure it has the cycle number
	fVars.insert(fBmwObjVar);	// make sure it has the cycle number
	TBase::CheckVars();
	cout << INFO << fNdet << " Det:" << endl;
	for (string det : fDetNames)
		cout << "\t" << det << endl;
	cout << ENDL;
	cout << INFO << fNmon << " Mon:" << endl;
	for (string mon : fMonNames)
		cout << "\t" << mon << endl;
	cout << ENDL;
	cout << INFO << fNcoil << " Coil:" << endl;
	for (string coil : fCoilNames)
		cout << "\t" << coil << endl;
	cout << ENDL;
}

void TDither::CalcSlope()
{
	const int sessions = fRootFile[run].size();
	for (int s=0; s<sessions; s++)
	{
		const int N = fEntryNumber[run][s].size();
		if (0 == N)
		{
			cerr << INFO << "no valid bmod data in the run: " << run << ENDL;
			continue;
		}

		TTree *tout = new TTree("dit", "dither slopes and sensitivities");

		int cycle;
		int nentries;
		TMatrixD slope(fNdet, fNmon);
		TMatrixD det_coil_sen(fNdet, fNcoil);	// det sensitivity: ∂D/∂C
		TMatrixD det_coil_sen_err(fNdet, fNcoil);	
		TMatrixD mon_coil_sen(fNmon, fNcoil);	// mon sensitivity: ∂B/∂C
		TMatrixD mon_sen_err(fNmon, fNcoil);	
		TMatrixD coil_mon_sen(fNcoil, fNmon);	// ∂C/∂B
		TMatrixD coil_mon_sen_err(fNcoil, fNmon);	

		tout->Branch("cycle", &cycle, "cycle/I");
		tout->Branch("nentries", &nentries, "nentries/I");
		for (int i=0; i<fNdet; i++)
		{
			string det = fDetNames[i];
			for (int j=0; j<fNmon; j++)
			{
				string var = det + "_" + fMonNames[j];
				tout->Branch(var.c_str(), &slope[i][j], "mean/D");	
				// tout->Branch(Form("%s_err", var.c_str()), &slope_err[i][j], "err/D");	
			}
		}
		for (int i=0; i<fNdet; i++)
		{
			string det = fDetNames[i];
			for (int j=0; j<fNcoil; j++)
			{
				string var = det + "_coil" + fCoilNames[j].substr(9, 1);
				tout->Branch(var.c_str(), &det_coil_sen[i][j], "mean/D");	
			}
		}
		for (int i=0; i<fNmon; i++)
		{
			string mon = fMonNames[i];
			for (int j=0; j<fNcoil; j++)
			{
				string var = mon + "_coil" + fCoilNames[j].substr(9, 1);
				tout->Branch(var.c_str(), &mon_coil_sen[i][j], "mean/D");	
			}
		}

		GetCycleRange(s);
		const int Ncycle = fCycleRange.size();

		// calculate the matrix: X_ij and Y_ij
		for (int ci=0; ci<Ncycle; ci++)
		{
			cycle = fVarValue[fCycleVar][fCycleRange[ci].first];
			if (0 == ci)
				cycle = 0;	// run-wise average
			nentries = fCycleRange[ci].second - fCycleRange[ci].first + 1;

			COVARIANCE det_cov[fNdet][7];			// det covariance w.r.t. coils
			COVARIANCE mon_cov[fNmon][7];			// mon covariance
			COVARIANCE det_var[fNdet][7];			// det variance
			COVARIANCE mon_var[fNmon][7];			// mon variance
			COVARIANCE coil_var[7];						// coil variance

			for (int n=fCycleRange[ci].first; n<=fCycleRange[ci].second; n++)
			{
				// FIXME: device_error_code cut
				int bmwobj = fVarValue[fBmwObjVar][n];
				string coil = Form("bmod_trim%d", bmwobj);
				if (find(fCoilNames.begin(), fCoilNames.end(), coil) == fCoilNames.end())
					continue;
				coil_var[bmwobj-1].update(fVarValue[coil][n], fVarValue[coil][n]);
				for (int i=0; i<fNdet; i++)
				{
					string det = fDetNames[i];
					det_var[i][bmwobj-1].update(fVarValue[det][n], fVarValue[det][n]);
					det_cov[i][bmwobj-1].update(fVarValue[det][n], fVarValue[coil][n]);
				}
				for (int i=0; i<fNmon; i++)
				{
					string mon = fMonNames[i];
					mon_var[i][bmwobj-1].update(fVarValue[mon][n], fVarValue[mon][n]);
					mon_cov[i][bmwobj-1].update(fVarValue[mon][n], fVarValue[coil][n]);
				}
			}

			// calculate sensitivity: ∂D/∂C
			for (int r=0; r<fNdet; r++)
			{
				for (int c=0; c<fNcoil; c++)
				{
					int bmwobj = stoi(fCoilNames[c].substr(9, 1));
					int icoil = bmwobj - 1;
					double det_mean = det_var[r][icoil].xmean;
					int nevent = coil_var[icoil].n;

					if (nevent <= 50 || coil_var[icoil].cov/nevent < 200 || det_mean <= 0)
					{
						det_coil_sen[r][c] = 0;
						det_coil_sen_err[r][c] = 0;
						continue;
					}
					double cov = det_cov[r][icoil].cov;
					double dvar = det_var[r][icoil].cov;
					double cvar = coil_var[icoil].cov;
					det_coil_sen[r][c] = cov/(det_mean*cvar);
					det_coil_sen_err[r][c] = sqrt((dvar - pow(cov, 2)/cvar)/cvar) / (nevent-2)/det_mean;
				}
			}
			// calculate sensitivity: ∂B/∂C
			for (int r=0; r<fNmon; r++)
			{
				for (int c=0; c<fNcoil; c++)
				{
					int bmwobj = stoi(fCoilNames[c].substr(9, 1));
					int icoil = bmwobj - 1;
					int nevent = coil_var[icoil].n;

					if (nevent <= 50 || coil_var[icoil].cov/nevent < 200)
					{
						mon_coil_sen[r][c] = 0;
						mon_sen_err[r][c] = 0;
						continue;
					}
					double cov = mon_cov[r][icoil].cov;
					double mvar = mon_var[r][icoil].cov;
					double cvar = coil_var[icoil].cov;
					mon_coil_sen[r][c] = cov/cvar;
					mon_sen_err[r][c] = sqrt((mvar - pow(cov, 2)/cvar)/cvar) / (nevent-2);
					coil_mon_sen[c][r] = cov/mvar;
					coil_mon_sen_err[c][r] = sqrt((cvar - pow(cov, 2)/mvar)/mvar) / (nevent-2);
				}
			}

			/* calculate slope
			 * We can't calculate slope using matrix multiplication, because not all coils
			 * have sensitivity; which results in det(mon)_sen as a singular matrix; so
			 * we can't invert it. We have to calculate slope one by one
			 */
			TMatrixD mon_coil_sen_inv(fNcoil, fNmon);
			if (fNmon == fNcoil)
			{
				mon_coil_sen_inv = mon_coil_sen;
				mon_coil_sen_inv.Invert();
			} 
			else
			{
				TMatrixD mon_coil_sen_T(mon_coil_sen);
				mon_coil_sen_T.T();
				mon_coil_sen_inv = mon_coil_sen_T * (mon_coil_sen * mon_coil_sen_T).Invert();
			}
			slope = det_coil_sen * mon_coil_sen_inv;
			// for (int r=0; r<fNdet; r++)
			// {
			// 	TMatrixD det_sub = det_coil_sen.GetSub(r, r, 0, fNcoil-1);
			// 	for (int c=0; c<fNmon; c++)
			// 	{
			// 		TMatrixD mon_sub = mon_coil_sen.GetSub(c, c, 0, fNcoil-1);
			// 		TMatrixD mon_subT(mon_sub);
			// 		mon_subT.T();
			// 		TMatrixD mon_sub_inv = mon_subT*(mon_sub*mon_subT).Invert();
			// 		slope[r][c] = (det_sub*mon_sub_inv)(0, 0);
			// 		mon_sub_inv.Print();
			// 	}
			// }

			// fill the slope tree
			tout->Fill();
		}

		TFile fout(Form("%s/%s_%d.%03d.root", out_dir, fPrefix, run, s), "recreate");
		fout.cd();
		tout->Write();
		fout.Close();
		delete tout;
	}
}

void TDither::DitherRuns()
{
	set<int> runs = fRuns;
	for (int r : runs)
	{
		cout << INFO << "correct run: " << r << ENDL;
		SetDitRun(r);
		GetValues();
		CalcSlope();
	}
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
