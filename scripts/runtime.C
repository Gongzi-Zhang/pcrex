void runtime()
{
    gROOT->SetBatch(1);
    ifstream fin("runtime.txt");
    int run;
    double time;
    TH1F *h1 = new TH1F("h1", "run time (min)", 100, 0, 100);
    while (fin >> run >> time)
	h1->Fill(time);

    gStyle->SetOptStat(1110);
    TCanvas c("c", "c");
    h1->Draw("HIST");
    // h1->SetXTitle("(min)");
    c.SaveAs("crex_run_time.png");
}
