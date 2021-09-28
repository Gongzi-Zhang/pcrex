#ifndef TCORRECT_H
#define TCORRECT_H

#include "TFile.h"
#include "TTree.h"
#include "TMatrixD.h"
#include "TMatrixDSym.h"
#include "TMatrixDSymEigen.h"

#include "TRSbase.h"

/* Correct raw asymmetry/difference using different methods:
 * regress
 * eigen-regress
 * dither
 * lagrange
*/
class TCorrect : public TRSbase
{
	private:
		int run;
		vector<string> fIvNames;
		vector<string> fDvNames;
		int fNiv, fNdv;
		const char *fMethod = "regress";
		const char *fBurstVar = "BurstCounter";
		map<string, const char *> fPrefix = {
			{"regress", "reg"},
			{"eigen-regress", "eigen_reg"},
			{"dither", "dit"},
			{"lagrange", "lag"},
		};
		vector<pair<int, int>> fMiniRange;

	public:
		TCorrect();
		~TCorrect() { cout << INFO << "end of regression" << ENDL; };
		void GetConfig(TConfig &fConf);
		void SetCorRun(int r);
		void CheckVars();
		void Correct();
		void Regress();
		void EigenRegress();
		void Dither() {}
		void Lagrange() {}
		void CorrectRuns();

		void GetMiniRange();
		double GetCovariance(const string, const string, const int);
		double GetCovError(const string, const string, const int);
};

TCorrect::TCorrect() :
	TRSbase()
{}

void TCorrect::GetConfig(TConfig &fConf)
{
	TBase::GetConfig(fConf);

	fIvNames.clear();
	fDvNames.clear();
	fIvNames = fConf.GetVector("iv");
	fDvNames = fConf.GetVector("dv");
	fNiv = fIvNames.size();
	fNdv = fDvNames.size();

	if (const char *m = fConf.GetScalar("method"))
	{
		cout << INFO << "use method: " << m;
		fMethod = m;
	}
	else
		cout << INFO << "use default method: " << fMethod;
}

void TCorrect::SetCorRun(int r) 
{ 
  run=r; 
  fRuns.clear(); 
  fSlugs.clear(); 
  SetRuns({run}); 
  // CheckRuns();
  nSlugs = 0;
}

void TCorrect::CheckVars()
{
	fVars.insert(fBurstVar);	// make sure it has the fBurstVar branch
	TBase::CheckVars();
	cout << INFO << fNiv << " iv variables:" << endl;
	for (string iv : fIvNames)
		cout << "\t" << iv << endl;
	cout << ENDL;
	cout << INFO << fNdv << " dv variables:" << endl;
	for (string dv : fDvNames)
		cout << "\t" << dv << endl;
	cout << ENDL;
}

void TCorrect::Correct()
{
	if (strcmp(fMethod, "regress") == 0)
		Regress();
	else if (strcmp(fMethod, "eigen-regress") == 0)
		EigenRegress();
	else if (strcmp(fMethod, "dither") == 0)
		Dither();
	else if (strcmp(fMethod, "lagrange") == 0)
		Lagrange();
}

void TCorrect::GetMiniRange()
{
	int mini = 0;	// always start from 0
	int mini_start = 0;
	fMiniRange.clear();
	const int N = fVarValue[fBurstVar].size();

	fMiniRange.push_back({0, N-1});	// the first one is whole run
	for (int n=0; n<N; n++)
	{
		if (mini != fVarValue[fBurstVar][n])	
		{	// next burst
			fMiniRange.push_back({mini_start, n-1});
			mini_start = n;
			mini++;
		}
	}
	// for the last minirun
	fMiniRange.push_back({mini_start, N-1});
}

double TCorrect::GetCovariance(const string x, const string y, const int mini)
{
	/* The definition of covariance is:
	 *		Cov(x, y) = ∑ (xi - xbar)(yi - ybar)
	 * where xbar (ybar) is the mean value of the sample set. Following this 
	 * formula, we need to loop through the sample twice, the first path to 
	 * get the mean value, and the second path to calculate the covariance;
	 * But actually we can do it with only one loop: to use to iteration method:
	 *		Cov(x, y)_n+1 = Cov_n + n/(n+1) (x_n+1 - xbar_n)(y_n+1 - ybar_n)
	 */
	if (fVarValue[x].size() != fVarValue[y].size())
	{
		cerr << ERROR << "inequal number of entries for var: " << x << " and " << y << ENDL;
		exit(44);
	}

	if (mini < 0 || mini >= fMiniRange.size())
	{
		cerr << ERROR << "Invalid mini number: " << mini 
				 << " (must be integer between 0 and " << fMiniRange.size() - 1 << ")" << ENDL;
		exit(54);
	}
	double cov = 0;
	double xval, xmean = 0;
	double yval, ymean = 0;
	double i=0;	// use double here because for int: 2/3=0
	for (int n=fMiniRange[mini].first; n<=fMiniRange[mini].second; n++)
	{
		xval = fVarValue[x][n];
		yval = fVarValue[y][n];
		cov += i/(i+1)*(xval - xmean)*(yval - ymean);
		i++;
		xmean += (xval-xmean)/i;
		ymean += (yval-ymean)/i;
	}
	return cov;
}

double TCorrect::GetCovError(string x, string y, const int mini)
{	/* the corresponding error of covariance is defined as:
	 *		Cov_err(x, y) = sqrt( (∑y² - n*xbar²) / n)
	 * where x is the independent variable, while y is the dependent variable
	 */

	if (fVarValue[x].size() != fVarValue[y].size())
	{
		cerr << ERROR << "inequal number of entries for var: " << x << " and " << y << ENDL;
		exit(44);
	}

	if (mini < 0 || mini >= fMiniRange.size())
	{
		cerr << ERROR << "Invalid mini number: " << mini 
				 << " (must be integer between 0 and " << fMiniRange.size() - 1 << ")" << ENDL;
		exit(54);
	}
	double xval, xmean = 0, xsum = 0;
	double yval, ymean = 0, ysum2 = 0;
	int i=0;	// use double here because for int: 2/3=0
	for (int n=fMiniRange[mini].first; n<=fMiniRange[mini].second; n++)
	{
		xval = fVarValue[x][n];
		yval = fVarValue[y][n];
		xsum += xval;
		ysum2 += yval*yval;
		i++;
	}
	xmean = xsum/i;
	return sqrt((ysum2 - i*xmean*xmean)/i);
}

void TCorrect::Regress()
{
	const int sessions = fRootFile[run].size();
	for (int s=0; s<sessions; s++)
	{
		const int N = fEntryNumber[run][s].size();
		if (0 == N)
			continue;

		TTree *tout_reg = new TTree("reg", "regressed variables");
		TTree *tout_slope = new TTree("reg_slope", "regressed slopes");

		int mini = 0;
		int npattern = 0;
		TMatrixD slope(fNdv, fNiv);
		// TMatrixD slope_err(fNdv, fNiv);
		map<string, double> value;

		tout_slope->Branch("mini", &mini, "mini/I");
		tout_slope->Branch("npattern", &npattern, "npattern/I");
		for (int i=0; i<fNdv; i++)
		{
			const char *pre = fPrefix[fMethod];
			string dv = fDvNames[i];
			tout_reg->Branch(Form("%s_%s", pre, dv.c_str()), &value[dv], Form("%s_%s/D", pre, dv.c_str()));

			for (int j=0; j<fNiv; j++)
			{
				string var = dv + "_" + fIvNames[j];
				tout_slope->Branch(Form("%s_%s", pre, var.c_str()), &slope[i][j], "mean/D");	
				// tout_slope->Branch(Form("%s_err", var.c_str()), &slope_err[i][j], "err/D");	
			}
		}

		GetMiniRange();
		const int Nmini = fMiniRange.size();
		TMatrixD X(fNiv, fNiv);
		TMatrixD Y(fNdv, fNiv);

		// calculate the matrix: X_ij and Y_ij
		for (mini=0; mini<Nmini; mini++)
		{
			for (int r=0; r<fNiv; r++)
			{
				for (int c=r; c<fNiv; c++)
				{
					X[r][c] = GetCovariance(fIvNames[r], fIvNames[c], mini);
					X[c][r] = X[r][c];	// Symmetric
				}
			}

			TMatrixD Xinv(fNiv, fNiv);
			double det = X.Determinant();
			if (abs(det) < 1e-38)
			{
				cerr << ERROR << "singular X matrix in minirun: " << mini << ", 0 value will be used" << ENDL;
				for (int i=0; i<fNiv; i++)
					cerr << fIvNames[i] << "\t";
				cerr << endl;
				X.Print();

				for (int i=0; i<fNiv; i++)
					for (int j=0; j<fNiv; j++)
						Xinv(i, j)  = 0;
			}
			else
			{
				Xinv = X;
				Xinv.Invert();
			}

			// Y matrix
			for (int r=0; r<fNdv; r++)
			{
				for (int c=0; c<fNiv; c++)
				{
					Y[r][c] = GetCovariance(fDvNames[r], fIvNames[c], mini);
				}
			}

			slope = Y * Xinv;
			// for (int r=0; r<fNdv; r++)
			// {
			// 	for (int c=0; c<fNiv; c++)
			// 	{
			// 		slope_err[r][c] = GetCovError(fIvNames[c], fDvNames[r], mini)*sqrt(Xinv[c][c]);
			// 	}
			// }
			// fill the slope tree
      npattern = fMiniRange[mini].second - fMiniRange[mini].first + 1;
			tout_slope->Fill();

			if (0 == mini)	// run-wise slope
				continue;
			
			// Correction: use minirun-wise slope
			for(int n=fMiniRange[mini].first; n <= fMiniRange[mini].second; n++)
			{
				for (int r=0; r<fNdv; r++)
				{
					string dv = fDvNames[r];
					double cor = 0;
					for (int c=0; c<fNiv; c++)
					{
						cor += slope(r, c)*(fVarValue[fIvNames[c]][n]/*-iv_mean(c)*/);
					}
					fVarValue[dv][n] -= cor;
					value[dv] = fVarValue[dv][n];
				}
				tout_reg->Fill();
			}
		}

		TFile fout(Form("%s/%s_%d.%03d.root", out_dir, fPrefix[fMethod], run, s), "recreate");
		fout.cd();
		tout_reg->Write();
		tout_slope->Write();
		fout.Close();
		delete tout_reg;
		delete tout_slope;
	}
}

void TCorrect::EigenRegress()
{
	const int sessions = fRootFile[run].size();
	for (int s=0; s<sessions; s++)
	{
		const int N = fEntryNumber[run][s].size();
		if (0 == N)
			continue;

		TTree *tout_reg = new TTree("eigen_reg", "eigen regressed variables");
		TTree *tout_slope = new TTree("eigen_reg_slope", "eigen regressed slopes");

		int mini = 0;
		int npattern = 0;
		TMatrixD slope(fNdv, fNiv);
		TMatrixD slope_tr(fNdv, fNiv);
		map<string, double> value;
		map<string, double> value_tr;

		tout_slope->Branch("mini", &mini, "mini/I");
		tout_slope->Branch("npattern", &npattern, "npattern/I");
		for (int i=0; i<fNdv; i++)
		{
			string dv = fDvNames[i];
			const char *pre = fPrefix[fMethod];
			tout_reg->Branch(Form("%s_%s", pre, dv.c_str()), &value[dv], Form("%s_%s/D", pre,  dv.c_str()));
			if (fNiv > 5)
				tout_reg->Branch(Form("%s_%s_tr", pre, dv.c_str()), &value_tr[dv], Form("%s_%s_tr/D", pre, dv.c_str()));

			for (int j=0; j<fNiv; j++)
			{
				string var = dv + "_" + fIvNames[j];
				tout_slope->Branch(Form("%s_%s", pre, var.c_str()), &slope[i][j], "mean/D");	
				// tout_slope->Branch(Form("%s_err", var.c_str()), &slope_err[i][j], "err/D");	
			}
		}

		TMatrixDSym X(fNiv);
		TMatrixD Y(fNdv, fNiv);

		GetMiniRange();
		const int Nmini = fMiniRange.size();
		for (mini=0; mini<Nmini; mini++)
		{
			// X Matrix
			for (int r=0; r<fNiv; r++)
			{
				for (int c=r; c<fNiv; c++)
				{
					X[r][c] = GetCovariance(fIvNames[r], fIvNames[c], mini);
					X[c][r] = X[r][c];	// Symmetric
				}
			}

			TMatrixD Xinv(fNiv, fNiv);
			double det = X.Determinant();
			if (abs(det) < 1e-38)
			{
				cerr << ERROR << "singular X matrix in minirun: " << mini << ", 0 value will be used" << ENDL;
				for (int i=0; i<fNiv; i++)
					cerr << fIvNames[i] << "\t";
				cerr << endl;
				X.Print();

				for (int i=0; i<fNiv; i++)
					for (int j=0; j<fNiv; j++)
						Xinv(i, j)  = 0;
			}
			else
			{
				Xinv = X;
				Xinv.Invert();
			}
			TMatrixDSymEigen X_eigen(X);
			TMatrixD eigen_vectors = X_eigen.GetEigenVectors();
			TVectorD eigen_values = X_eigen.GetEigenValues();

			// Y matrix
			for (int r=0; r<fNdv; r++)
			{
				for (int c=0; c<fNiv; c++)
				{
					Y[r][c] = GetCovariance(fDvNames[r], fIvNames[c], mini);
				}
			}

			slope = Y * (Xinv * eigen_vectors);
			if (fNiv > 5)
			{
				slope_tr = slope;
				for (int r=0; r<fNdv; r++)
				{
					double top5[5] = {-1};
					int top5_index[5] = {0};
					for (int c=0; c<fNiv; c++)
					{
						double cor = abs(slope[r][c])*sqrt(eigen_values[c]);
						for (int i=0; i<5; i++)
						{
							if (cor > top5[i])
							{
								for (int j=4; j>i; j--)
								{
									top5[j] = top5[j-1];
									top5_index[j] = top5_index[j-1];
								}
								top5[i] = cor;
								top5_index[i] = c;
								break;
							}
						}
					}
					bool keep_it[fNiv] = {false};
					for (int i=0; i<5; i++)
						keep_it[top5_index[i]] = true;

					for (int c=0; c<fNiv; c++)
					{
						if (! keep_it[c])
							slope_tr[r][c] = 0;
					}
				}
			}

			// fill the slope tree
      npattern = fMiniRange[mini].second - fMiniRange[mini].first + 1;
			tout_slope->Fill();

			if (0 == mini)
				continue;
			
			// Correction
			TMatrixD eigen_vectors_T = eigen_vectors.T();
			for(int n=fMiniRange[mini].first; n <= fMiniRange[mini].second; n++)
			{
				for (int r=0; r<fNdv; r++)
				{
					double cor = 0;
					double cor_tr = 0;	// truncated correction
					TVectorD iv_vector(fNiv);
					for (int i=0; i<fNiv; i++)
						iv_vector[i] = fVarValue[fIvNames[i]][n];

					TVectorD eigen_vector(fNiv);
					eigen_vector = eigen_vectors_T * iv_vector;
					for (int c=0; c<fNiv; c++)
					{
						cor += slope[r][c] * eigen_vector[c];
						cor_tr += slope_tr[r][c] * eigen_vector[c];
					}
					// fVarValue[fDvNames[r]][n] -= cor;
					value[fDvNames[r]] = fVarValue[fDvNames[r]][n] - cor;
					value_tr[fDvNames[r]] = fVarValue[fDvNames[r]][n] - cor_tr;
				}
				tout_reg->Fill();
			}
		}

		TFile fout(Form("%s/%s_%d.%03d.root", out_dir, fPrefix[fMethod], run, s), "recreate");
		fout.cd();
		tout_reg->Write();
		tout_slope->Write();
		fout.Close();
		delete tout_reg;
		delete tout_slope;
	}
}

void TCorrect::CorrectRuns()
{
	set<int> runs = fRuns;
	for (int r : runs)
	{
		cout << INFO << "correct run: " << r << ENDL;
		SetCorRun(r);
		GetValues();
		Correct();
	}
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
