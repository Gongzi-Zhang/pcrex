#include <iostream>

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"

#include "evtcut.h"

typedef struct {double mean, err, nsample;} STAT;

const char* dir = "/lustre19/expphy/volatile/halla/parity/crex-respin1/japanOutput/";
const char* dvs[] = {
    "bcm_an_us", "bcm_an_ds", "bcm_an_ds3", "bcm_an_ds10", "bcm_dg_us", "bcm_dg_ds",
    // "bpm1X", "bpm4aX", "bpm4aY", "bpm4eX", "bpm4eY", "bpm11X", "bpm11Y", "bpm12X", "bpm16X",
    "cav4bQ","cav4cQ","cav4dQ","bcm_dg_usc","bcm_dg_dsc",
    "cav4bXI","cav4bYI","cav4cXI","cav4cYI","cav4dXI","cav4dYI",
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
    gROOT->SetBatch(1);
    init_evtcut();

    vector<TCut> cuts = beam_evtcut_all.count(run) ? beam_evtcut_all[run] : beam_evtcut[run];
    const int ncut = cuts.size();
    // const int npcut = pedestal_evtcut[run].size();
    if (0 == ncut)
    {
	fprintf(stderr, "Error--run: %d\tno beam cut. \
		Are you sure it is a pedestal run? if yes, please update evtcut.h\n", run);
	return;
    }
    /*
    if (ncut != npcut)
    {
	fprintf(stderr, "Error--run: %d\tUnmatched size of beam_evtcut: %d vs. pedestal_evtcut: %d.\n", 
	    run, ncut, npcut);
	return;
    }
    */
    const char* fname = Form("%s/prexPrompt_pass2_%d.000.root", dir, run);
    TFile f(fname, "read");
    if (!f.IsOpen())
    {
	fprintf(stderr, "Error--run: %d\tCan't open file: %s\n", run, fname);
	return;
    }

    TTree *t = (TTree*)f.Get("evt");
    if (!t)
    {
	fprintf(stderr, "Error--run: %d\tCan't read tree: evt in file: %s\n", run, fname);
	f.Close();
	return;
    }

    TFile fout("ped.root", "update");
    if (!fout.IsOpen())
    {
	cerr << "Error--Can't open fine: ped.root" << endl;
	return;
    }
    map<string, STAT> ped;
    map<string, STAT> yield;

    TTree *tout = (TTree*) fout.Get("ped");
    if (!tout)	// create it
	tout = new TTree("ped", "pedestals");
    if(tout->GetEntries(Form("%d==run", run)))
    {
	printf("run %d is already there, if you want to update it, please delete ped.root and then rerun this program.\n", run);
	fout.Close();
	f.Close();
	return;
    }

    tout->Branch("run", &run, "run/I");
    for (const char *dv : dvs)
    {
	tout->Branch(dv, &ped[dv], "mean/D:err/D:nsample/D");
	const char *dv_yield = Form("%s_yield", dv);
	tout->Branch(dv_yield, &yield[dv], "mean/D:err/D:nsample/D");
    }

    const char *bcm = "bcm_an_us";  // default bcm
    if (*user_bcm)
	bcm = user_bcm;
    else if (spc_bcm.count(run))
	bcm = spc_bcm[run];

    if (spc_cut.count(run) || *user_cut)
    {
	TCut cut;
	if (*user_cut)
	    cut += user_cut;
	if (spc_cut.count(run))
	    cut += spc_cut[run];

	for (int i=0; i<ncut; i++)
	{
	    cuts[i] += cut;
	    // pedestal_evtcut[run][i] += cut;
	}
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
    int n;
    /*
    {	// bcm pedestal
	double unser_ped[ncut], unser_ped_err[ncut];
	double unser_yield[ncut], unser_yield_err[ncut];
	double bcm_yield[ncut], bcm_yield_err[ncut];
	for (int i=0; i<ncut; i++)
	{
	    // unser
	    n = t->Draw("unser", pedestal_evtcut[run][i], "goff");
	    htemp = (TH1F*)gDirectory->Get("htemp");
	    unser_ped[i] = htemp->GetMean();
	    unser_ped_err[i] = htemp->GetRMS()/sqrt(n);
	    htemp->Delete();

	    n = t->Draw("unser", beam_evtcut[run][i], "goff");
	    htemp = (TH1F*)gDirectory->Get("htemp");
	    unser_yield[i] = htemp->GetMean() - unser_ped[i];
	    double err = htemp->GetRMS()/sqrt(n);
	    unser_yield_err[i] = sqrt(err*2 + pow(unser_ped_err[i], 2));
	    htemp->Delete();

	    // bcm
	    n = t->Draw(Form("%s.hw_sum_raw/%s.num_samples", bcm, bcm), beam_evtcut[run][i], "goff");
	    htemp = (TH1F*)gDirectory->Get("htemp");
	    bcm_yield[i] = htemp->GetMean();
	    bcm_yield_err[i] = htemp->GetRMS()/sqrt(n);
	    htemp->Delete();
	}

	TGraphErrors* g = new TGraphErrors(ncut, unser_yield, bcm_yield, unser_yield_err, bcm_yield_err);
	g->SetMarkerStyle(20);
	g->Fit("pol1", "AP", "", xmin, xmax);
	TF1 *f = (TF1 *)g->GetFunction("pol1");
	f->SetLineColor(kRed);
	ped[bcm].mean = f->GetParameter(0);
	ped_err[bcm] = f->GetParError(0);
	double slope = f->GetParameter(1);
	double slope_err = f->GetParError(1);
	// double gain = 1/slope;
	// double gain_err = slope_err/pow(slope, 2);
	cerr << "Info--run: " << run << "\t" 
	    << bcm << "\t" << ped[bcm] << " ± " << ped_err[bcm] << "\t"
	    << slope << " ± " << slope_err << endl;

	// bcm residual
	double res[ncut], res_err[ncut];
	for (int i=0; i<ncut; i++)
	{
	    res[i] = (bcm_yield[i] - ped[bcm])/slope - unser_yield[i];
	    double a = bcm_yield[i] - ped[bcm];
	    double a_err2 = pow(bcm_yield_err[i], 2) + pow(ped[bcm], 2);
	    res_err[i] = sqrt((a_err2 + pow(a*slope_err/slope, 2))/pow(slope, 2)  + pow(unser_yield_err[i], 2));
	}
	TGraphErrors* g_res = new TGraphErrors(ncut, unser_yield, res, unser_yield_err, res_err);
	g_res->SetMarkerStyle(20);
	g_res->Draw("AP");
    }
    */
    ped[bcm].mean = -613.258885;
    ped[bcm].err = 4.071672;
    double bcm_slope = 111.583401;
    double bcm_slope_err = 0.045032;

    // other devices
    double bcm_yield[ncut], bcm_yield_err[ncut];
    double dv_yield[ncut], dv_yield_err[ncut];
    double res[ncut];
    for (int i=0; i<ncut; i++)
    {
	n = t->Draw(Form("%s.hw_sum_raw/%s.num_samples", bcm, bcm), cuts[i], "goff");
	htemp = (TH1F*)gDirectory->Get("htemp");
	// bcm_yield[i] = htemp->GetMean();	// FIXME should I correct this value with ped[bcm]?
	// bcm_yield_err[i] = htemp->GetRMS()/sqrt(n);
	bcm_yield[i] = (htemp->GetMean() - ped[bcm].mean)/bcm_slope;	
	bcm_yield_err[i] = sqrt((pow(htemp->GetRMS()/sqrt(n), 2) + pow(ped[bcm].err, 2))+ pow(bcm_yield[i]*bcm_slope_err, 2))/abs(bcm_slope);
	htemp->Delete();
    }
    for (const char* dv : dvs)
    {
	if (strcmp(dv, bcm) == 0)
	    continue;
	printf("Info--processing dv: %s\n", dv);
	if (dv_bl.count(run) && find(dv_bl[run].begin(), dv_bl[run].end(), dv) != dv_bl[run].end())
	{
	    cerr << "Warning--blacklisted dv: " << dv << " in run: " << run << endl;
	    continue;
	}

	for (int i=0; i<ncut; i++)
	{
	    n = t->Draw(Form("%s.hw_sum_raw/%s.num_samples>>htemp", dv, dv), cuts[i], "goff");
	    htemp = (TH1F*) gDirectory->Get("htemp");
	    dv_yield[i] = htemp->GetMean();
	    dv_yield_err[i] = htemp->GetRMS()/sqrt(n);
	    htemp->Delete();
	}

	TGraphErrors* g = new TGraphErrors(ncut, bcm_yield, dv_yield, bcm_yield_err, dv_yield_err);
	g->SetMarkerStyle(20);
	g->Fit("pol1", "QR", "", xmin, xmax);
	// g->Fit("pol1");
	TF1 *f = g->GetFunction("pol1");
	f->SetLineColor(kRed);
	ped[dv].mean = f->GetParameter(0);
	ped[dv].err = f->GetParError(0);
	double slope = f->GetParameter(1);
	double slope_err = f->GetParError(1);
	printf("Info--run: %d\tdv: %s\tped: %f ± %f\t slope: %f ± %f\n", 
		run, dv, ped[dv].mean, ped[dv].err, slope, slope_err);

	for (int i=0; i<ncut; i++)
	    res[i] = dv_yield[i] - f->Eval(bcm_yield[i]);
	TGraphErrors* g_res = new TGraphErrors(ncut, bcm_yield, res, bcm_yield_err, dv_yield_err);
	g_res->SetMarkerStyle(20);
	g_res->Draw("AP");
    }
}

void beamoff_ped(int run)
{
    const char* fname = Form("%s/prexPrompt_pass2_%d.000.root", dir, run);
    TFile fin(fname, "read");
    if (!fin.IsOpen())
    {
	cerr << "Error--Can't open file: " << fname << " for run: " << run << endl;
	return;
    }

    TTree *t = (TTree*)fin.Get("evt");
    if (!t)
    {
	cerr << "Error--Can't read tree: evt in file: " << fname << endl;
	fin.Close();
	return;
    }

    TFile fout("ped.root", "update");
    if (!fout.IsOpen())
    {
	cerr << "Error--Can't open fine: ped.root" << endl;
	return;
    }

    map<string, STAT> ped;
    map<string, STAT> yield;
    map<string, TBranch*> br;

    TTree *tout = (TTree*) fout.Get("ped");
    if (!tout)	// create it
	tout = new TTree("beamoff_ped", "beamoff pedestals");
    if(tout->GetEntries(Form("run==%d", run)))
    {
	printf("run %d is already there, if you want to update it, please delete ped.root and then rerun this program.\n", run);
	fout.Close();
	return;
    }
    TBranch *brun = tout->Branch("run", &run, "run/I");
    for (const char *dv : dvs)
    {
	br[dv] = tout->Branch(dv, &ped[dv], "mean/D:err/D:nsample/D");
	const char *dv_yield = Form("%s_yield", dv);
	br[dv_yield] = tout->Branch(dv_yield, &yield[dv], "mean/D:err/D:nsample/D");
    }

    TCut cut1 = "unser < 0";
    TCut cut2 = "ErrorFlag == 0";
    for (const char *dv : dvs)
    {
	long n1 = t->Draw(Form("%s.hw_sum_raw/%s.num_samples", dv, dv), cut1, "goff");
	TH1F* htemp = (TH1F*)gDirectory->Get("htemp");
	if (htemp)
	{
	    ped[dv].mean = htemp->GetMean();
	    ped[dv].err = htemp->GetRMS()/sqrt(n1);  
	    ped[dv].nsample = n1;
	    cout << run << "\t" << dv << "\t" 
		<< ped[dv].mean << "±" << ped[dv].err << "\t" << ped[dv].nsample << endl;
	}

	const char *dv_yield = Form("%s_yield", dv);
	long n2 = t->Draw(Form("%s.hw_sum_raw/%s.num_samples", dv, dv), cut2, "goff");
	htemp = (TH1F*)gDirectory->Get("htemp");
	if (htemp)
	{
	    yield[dv].mean = htemp->GetMean();
	    yield[dv].err = htemp->GetRMS()/sqrt(n2);  
	    yield[dv].nsample = n2;
	    cout << run << "\t" << dv << "_yield\t" 
		<< yield[dv].mean << "±" << yield[dv].err << "\t" << yield[dv].nsample << endl;
	}
    }
    tout->Fill();

    t->Delete();
    fin.Close();

    fout.cd();
    tout->Write("", TObject::kOverwrite);
    tout->Delete();
    fout.Close();
}

void global_bcm_ped()
{
    init_evtcut();
    int unser_runs[] = {5434, /* 5566, */ 5576, /* 6049, */ 6176, /* 6233, */ 6242, /* 6442, */ 6560, 7121,
	7279, 7407, 7710, 8108, 8351, 8464, };
    const int nruns = sizeof(unser_runs)/sizeof(int);

    TGraphErrors * gped = new TGraphErrors();
    TGraphErrors * gslope = new TGraphErrors();
    FILE *out = fopen("global_bcm_ped.txt", "w");
    const char *bcm = "bcm_an_us";
    TH1F *htemp;
    int n;
    for (int r=0; r<nruns; r++)
    {
	int run = unser_runs[r];
	printf("Info--Processing run: %d\n", run);
	const int npoints = pedestal_evtcut[run].size();
	if (0 == npoints)
	{
	    printf("ERROR--run: %d\tno pedestal cut found, are you sure it is a unser cali. run?\n", run);
	    return;
	}
	if (npoints != beam_evtcut[run].size())
	{
	    printf("ERROR--run: %d\tUnmatched # of pedestal cut: %d vs # of event cut: %zu,  \
		    please resolve it.\n", run, npoints, beam_evtcut[run].size());
	    return;
	}
	
	const char* fname = Form("%s/prexPrompt_pass2_%d.000.root", dir, run);
	TFile fin(fname, "read");
	if (!fin.IsOpen())
	{
	    printf("ERROR--run: %d\tno japan rootfile for run: %s\n", run, fname);
	    return;
	}

	TTree *tin = (TTree *) fin.Get("evt");
	if (!tin)
	{
	    printf("ERROR--run: %d\tno evt tree in file: %s\n", run, fname);
	    fin.Close();
	    return;
	}

	double unser_ped[npoints], unser_ped_err[npoints];
	double unser_yield[npoints], unser_yield_err[npoints];
	double bcm_yield[npoints], bcm_yield_err[npoints];
	for (int i=0; i<npoints; i++)
	{
	    printf("Info--Processing cut: %d\n", i);
	    // unser
	    n = tin->Draw("unser", pedestal_evtcut[run][i], "goff");
	    htemp = (TH1F*) gDirectory->Get("htemp");
	    unser_ped[i] = htemp->GetMean();
	    unser_ped_err[i] = htemp->GetRMS()/sqrt(n);
	    htemp->Delete();

	    n = tin->Draw("unser", beam_evtcut[run][i], "goff");
	    htemp = (TH1F*) gDirectory->Get("htemp");
	    unser_yield[i] = htemp->GetMean() - unser_ped[i];
	    double err = htemp->GetRMS()/sqrt(n);
	    unser_yield_err[i] = sqrt(err*2 + pow(unser_ped_err[i], 2));
	    htemp->Delete();

	    // bcm
	    n = tin->Draw(Form("%s.hw_sum_raw/%s.num_samples", bcm, bcm), beam_evtcut[run][i], "goff");
	    htemp = (TH1F*) gDirectory->Get("htemp");
	    bcm_yield[i] = htemp->GetMean();
	    bcm_yield_err[i] = htemp->GetRMS()/sqrt(n);
	    htemp->Delete();
	}
	tin->Delete();
	fin.Close();

	TGraphErrors * g_tmp = new TGraphErrors(npoints, unser_yield, bcm_yield, unser_yield_err, bcm_yield_err);
	g_tmp->Fit("pol1"); // show I chosse fit range ???
	TF1 * fit = g_tmp->GetFunction("pol1");
	double ped = fit->GetParameter(0);
	double ped_err = fit->GetParError(0);
	double slope = fit->GetParameter(1);
	double slope_err = fit->GetParError(1);
	fprintf(out, "run: %d\t%d\tped: %f ± %f\tslope: %f ± %f\n", run, npoints, ped, ped_err, slope, slope_err);
	gped->SetPoint(r, r+1, ped);
	gped->SetPointError(r, 0, ped_err);
	gslope->SetPoint(r, r+1, slope);
	gslope->SetPointError(r, r+1, slope_err);
	g_tmp->Delete();
    }

    gped->Fit("pol0");
    TF1 *fit = gped->GetFunction("pol0");
    double g_ped = fit->GetParameter(0);
    double g_ped_err = fit->GetParError(0);
    gped->Delete();
    fprintf(out, "global %s pedestal: %f ± %f\n", bcm, g_ped, g_ped_err);
    gslope->Fit("pol0");
    fit = gslope->GetFunction("pol0");
    double g_slope = fit->GetParameter(0);
    double g_slope_err = fit->GetParError(0);
    gslope->Delete();
    fprintf(out, "global %s slope: %f ± %f\n", bcm, g_slope, g_slope_err);
    fclose(out);
}

int main(int argc, char* argv[])
{

}
