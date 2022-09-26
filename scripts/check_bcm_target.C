void check_bcm_target(const int run)
{
    string fname = Form("/lustre19/expphy/volatile/halla/parity/crex-respin2/japanOutput/prexPrompt_pass2_%d.000.root", run);
    TFile *f = new TFile(fname.c_str(), "read");
    if (!f->IsOpen())
    {
	cerr << "no rootfile for run: " << run << endl;
	exit(4);
    }
    TTree *t = (TTree*) f->Get("evt");
    if (!t)
    {
	cerr << "can't receive tree: evt from: " << fname << endl;
	exit(14);
    }
    t->SetBranchStatus("*", 0);
    map<string, TLeaf*> leaf;
    map<string, double> diff;
    string device[] = {"bcm_an_us", "bcm_an_ds", "bcm_an_ds3", "bcm_dg_us", "bcm_dg_ds"};

    t->SetBranchStatus("bcm_target", 1);
    leaf["bcm_target"] = t->GetBranch("bcm_target")->GetLeaf("hw_sum");
    for (string var : device)
    {
	t->SetBranchStatus(var.c_str(), 1);
	leaf[var] = t->GetBranch(var.c_str())->GetLeaf("hw_sum");
	diff[var] = 0.;
    }

    for (int i=0; i<5; i++)
    {
	t->GetEntry(i);

	leaf["bcm_target"]->GetBranch()->GetEntry(i);
	double bcm_target = leaf["bcm_target"]->GetValue();
	for (string var : device)
	{
	    leaf[var]->GetBranch()->GetEntry(i);
	    diff[var] += (leaf[var]->GetValue() - bcm_target);
	}
    }

    for (string var : device)
    {
        if (0 == diff[var])
        {
            cerr << run << "\t" << var << endl;
            return;
        }
    }

    return;
}
