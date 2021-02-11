#include <iostream>

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"

#if defined(__GCC__)
#include "io.h"
#else
#include "../include/io.h"
#endif

#include "evtcut.h"

const char* dir = "/lustre19/expphy/volatile/halla/parity/crex-respin1/japanOutput/";
const char* dvs[] = {
    // "bcm_an_us", "bcm_an_ds", "bcm_an_ds3", "bcm_an_ds10", "bcm_dg_us", "bcm_dg_ds",
    // "bpm1X", "bpm4aX", "bpm4aY", "bpm4eX", "bpm4eY", "bpm11X", "bpm11Y", "bpm12X", "bpm16X",
    // "cav4bQ","cav4cQ","cav4dQ","bcm_dg_usc","bcm_dg_dsc",
    // "cav4bXI","cav4bYI","cav4cXI","cav4cYI","cav4dXI","cav4dYI",
    "usl", "usr", "dsl", "dsr",
    "atl1", "atl2", "atr1", "atr2",
    "sam1", "sam2", "sam3", "sam4", "sam5", "sam6", "sam7", "sam8",
};
const int ndv = sizeof(dvs)/sizeof(*dvs);
void pedestal(	int run, 
		const char* user_cut = "",
		const char* user_bcm = "",
		double low = -1, double high = -1)
{
    init_evtcut();
    if (beam_evtcut.count(run) == 0 || pedestal_evtcut.count(run) == 0)
    {
	cerr << ERROR << "no beam/pedestal cut for run: " << run
	    << ". Are you sure it is a pedestal run? if yes, please update evtcut.h"
	    << ENDL;
	exit(4);
    }
    const char* fname = Form("%s/prexPrompt_pass2_%d.000.root", dir, run);
    TFile f(fname, "read");
    if (!f.IsOpen())
    {
	cerr << ERROR << "Can't open file: " << fname << " for run: " << run << ENDL;
	exit(4);
    }

    TTree *t = (TTree*)f.Get("evt");
    if (!t)
    {
	cerr << ERROR << "Can't read tree: evt in file: " << fname << ENDL;
	f.Close();
	exit(5);
    }

    const char *bcm = "bcm_an_us";  // default bcm
    if (*user_bcm)
	bcm = user_bcm;
    else if (spc_bcm.count(run))
	bcm = spc_bcm[run];

    int ncut = beam_evtcut[run].size();
    int npcut = pedestal_evtcut[run].size();
    if (ncut != npcut)
    {
	cerr << ERROR << "Unequal size of beam_evtcut and pedestal_evtcut for run: " << run << ENDL;
	f.Close();
	exit(14);
    }
    if (spc_cut.count(run) || *user_cut)
    {
	TCut cut;
	if (*user_cut)
	    cut += user_cut;
	if (spc_cut.count(run))
	    cut += spc_cut[run];

	for (int i=0; i<ncut; i++)
	{
	    beam_evtcut[run][i] += cut;
	    pedestal_evtcut[run][i] += cut;
	}

	if (beam_evtcut_all.count(run))
	    for (int i=0; i<beam_evtcut_all[run].size(); i++)
		beam_evtcut_all[run][i] += cut;
    }

    double xmin = 5, xmax = 200;
    if (low != -1)
	xmin = low;
    else if (low_limit.count(run))
	xmin = low_limit[run];

    if (high != -1)
	xmax = high;
    else if (up_limit.count(run))
	xmax = up_limit[run];

    TH1F *htemp;
    map<string, double> ped, ped_err;
    // bcm pedestal
    {
	double unser_ped[ncut], unser_ped_err[ncut];
	double unser_mean[ncut], unser_err[ncut];
	double bcm_mean[ncut], bcm_err[ncut];
	for (int i=0; i<ncut; i++)
	{
	    // unser
	    t->Draw("user", pedestal_evtcut[run][i], "goff");
	    htemp = (TH1F*)gDirectory->Get("htemp");
	    unser_ped[i] = htemp->GetMean();
	    unser_ped_err[i] = htemp->GetRMS()/sqrt(htemp->GetEntries());

	    t->Draw("user", beam_evtcut[run][i], "goff");
	    htemp = (TH1F*)gDirectory->Get("htemp");
	    unser_mean[i] = htemp->GetMean() - unser_ped[i];
	    double err = htemp->GetRMS()/sqrt(htemp->GetEntries());
	    unser_err[i] = sqrt(err*2 + pow(unser_ped_err[i], 2));

	    // bcm
	    t->Draw(Form("%s.hw_sum_raw/%s.num_samples", bcm, bcm), beam_evtcut[run][i], "goff");
	    htemp = (TH1F*)gDirectory->Get("htemp");
	    bcm_mean[i] = htemp->GetMean();
	    bcm_err[i] = htemp->GetRMS()/sqrt(htemp->GetEntries());
	}

	TGraphErrors* g = new TGraphErrors(ncut, unser_mean, bcm_mean, unser_err, bcm_err);
	g->SetMarkerStyle(20);
	g->Fit("pol1", "AP", "", xmin, xmax);
	TF1 *f = (TF1 *)g->GetFunction("pol1");
	f->SetLineColor(kRed);
	ped[bcm] = f->GetParameter(0);
	ped_err[bcm] = f->GetParError(0);
	double slope = f->GetParameter(1);
	double slope_err = f->GetParError(1);
	// double gain = 1/slope;
	// double gain_err = slope_err/pow(slope, 2);
	cerr << INFO << "run: " << run << "\t" 
	    << bcm << "\t" << ped[bcm] << " ± " << ped_err[bcm] << "\t"
	    << slope << " ± " << slope_err << ENDL;

	// bcm residual
	double res[ncut], res_err[ncut];
	for (int i=0; i<ncut; i++)
	{
	    res[i] = (bcm_mean[i] - ped[bcm])/slope - unser_mean[i];
	    double a = bcm_mean[i] - ped[bcm];
	    double a_err2 = pow(bcm_err[i], 2) + pow(ped[bcm], 2);
	    res_err[i] = sqrt((a_err2 + pow(a*slope_err/slope, 2))/pow(slope, 2)  + pow(unser_err[i], 2));
	}
	TGraphErrors* g_res = new TGraphErrors(ncut, unser_mean, res, unser_err, res_err);
	g_res->SetMarkerStyle(20);
	g_res->Draw("AP");
    }

    // other devices
    vector<TCut> cuts = beam_evtcut_all.count(run) ? beam_evtcut_all[run] : beam_evtcut[run];
    ncut = cuts.size();
    double bcm_mean[ncut], bcm_err[ncut];
    double dv_mean[ncut], dv_err[ncut];
    double res[ncut];
    for (int i=0; i<ncut; i++)
    {
	t->Draw(Form("%s.hw_sum_raw/%s.num_samples", bcm, bcm), cuts[i], "goff");
	htemp = (TH1F*)gDirectory->Get("htemp");
	bcm_mean[i] = htemp->GetMean();	// FIXME should I correct this value with ped[bcm]?
	bcm_err[i] = htemp->GetRMS()/sqrt(htemp->GetEntries());
    }
    for (const char* dv : dvs)
    {
	if (dv_bl.count(run) && find(dv_bl[run].begin(), dv_bl[run].end(), dv) != dv_bl[run].end())
	{
	    cerr << WARNING << "blacklisted dv: " << dv << " in run: " << run << ENDL;
	    continue;
	}

	for (int i=0; i<ncut; i++)
	{
	    t->Draw(Form("%s.hw_sum_raw/%s.num_samples>>htemp", dv, dv), cuts[i], "goff");
	    htemp = (TH1F*) gDirectory->Get("htemp");
	    dv_mean[i] = htemp->GetMean();
	    dv_err[i] = htemp->GetRMS()/sqrt(htemp->GetEntries());
	}

	TGraphErrors* g = new TGraphErrors(ncut, bcm_mean, dv_mean, bcm_err, dv_err);
	g->SetMarkerStyle(20);
	g->Fit("pol1", "QR", "", xmin, xmax);
	TF1 *f = (TF1 *)g->GetFunction("pol1");
	f->SetLineColor(kRed);
	ped[dv] = f->GetParameter(0);
	ped_err[dv] = f->GetParError(0);
	double slope = f->GetParameter(1);
	double slope_err = f->GetParError(1);
	cerr << INFO << "run: " << run << "\t" 
	    << dv << "\t" << ped[dv] << " ± " << ped_err[dv] << "\t"
	    << slope << " ± " << slope_err << ENDL;

	for (int i=0; i<ncut; i++)
	    res[i] = dv_mean[i] - f->Eval(bcm_mean[i]);
	TGraphErrors* g_res = new TGraphErrors(ncut, bcm_mean, res, bcm_err, dv_err);
	g_res->SetMarkerStyle(20);
	g_res->Draw("AP");
    }
}

void beamoff_ped(int run, const char *user_cut = "")
{
    init_evtcut();
    if (pedestal_evtcut.count(run) == 0)
    {
	cerr << ERROR << "no pedestal cut for run: " << run
	    << ". Are you sure it is a pedestal run? if yes, please update evtcut.h"
	    << ENDL;
	exit(4);
    }

    const char* fname = Form("%s/prexPrompt_pass2_%d.000.root", dir, run);
    TFile f(fname, "read");
    if (!f.IsOpen())
    {
	cerr << ERROR << "Can't open file: " << fname << " for run: " << run << ENDL;
	f.Close();
	exit(4);
    }

    TTree *t = (TTree*)f.Get("evt");
    if (!t)
    {
	cerr << ERROR << "Can't read tree: evt in file: " << fname << ENDL;
	f.Close();
	exit(5);
    }

    vector<TCut> cuts = pedestal_evtcut[run];
    map<string, double> ped;
    map<string, double> ped_err;
    for (const char *dv : dvs)
    {
	double sum = 0, sum_weight = 0;
	for (TCut cut : cuts)
	{
	    cut += user_cut;
	    t->Draw(dv, cut, "goff");
	    TH1F* htemp = (TH1F*)gDirectory->Get("htemp");
	    double weight = htemp->GetEntries()/pow(htemp->GetRMS(), 2);
	    sum += htemp->GetMean()*weight;
	    sum_weight += weight;
	}
	ped[dv] = sum/sum_weight;
	ped_err[dv] = 1/sqrt(sum_weight);
	cout << INFO << "device: " << dv << "\t" << ped[dv] << " ± " << ped_err[dv] << ENDL;
    }
}

int main(int argc, char* argv[])
{

}
