void runcharge()
{
    gROOT->SetBatch(1);
    ifstream fin("charge.txt");
    int run;
    double total, valid, acc1, acc2;
    TH1F *h1 = new TH1F("h1", "valid charge per run (C)", 100, 0, 0.6);
    TH1F *h2 = new TH1F("h2", "charge effeciency (%)", 100, 0, 100);
    while (fin >> run >> total >> valid >> acc1 >> acc2)
    {
	h1->Fill(valid/1e6);
	h2->Fill(valid/total*100);
    }

    gStyle->SetOptStat(1110);
    TCanvas c("c", "c");
    h1->Draw("HIST");
    // h1->SetXTitle("(min)");
    c.SaveAs("crex_run_charge.png");

    c.Clear();
    h2->Draw("HIST");
    gPad->Update();
    TPaveStats *st = (TPaveStats *)c.GetPrimitive("stats");
    st->SetX1NDC(0.1);
    st->SetX2NDC(0.3);
    st->SetY1NDC(0.75);
    st->SetY2NDC(0.9);
    st->Draw();
    // h1->SetXTitle("(min)");
    c.SaveAs("crex_run_charge_efficiency.png");
}
