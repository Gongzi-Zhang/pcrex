#ifndef TAGGREGATE_H
#define TAGGREGATE_H

#include <iostream>
#include "TBase.h"

struct STAT { double mean, err, rms; };

class TAggregate : public TBase 
{
	private:
		const char * out_dir = "rootfiles";
		// const char * out_pattern = "agg_minirun_xxxx_???";
	public:
		TAggregate(const char * config_file, const char * run_list = NULL);
		~TAggregate() { cout << INFO << "end of TAggregate"; };
		void SetOutDir(const char *dir);
		void CheckOutDir();
		void Aggregate();
};

TAggregate::TAggregate(const char * config_file, const char * run_list) :
	TBase(config_file, run_list)
{}

void TAggregate::SetOutDir(const char *dir) 
{
	if (!dir) {
		cerr << FATAL << "Null out dir value!" << ENDL;
		exit(104);
	}
	out_dir = dir;
}

void TAggregate::CheckOutDir() 
{
	struct stat info;
	if (stat(out_dir, &info) != 0) {
		cerr << FATAL << "can't access specified dir: " << out_dir << ENDL;
		exit(104);
	} else if ( !(info.st_mode & S_IFDIR)) {
		cerr << FATAL << "not a dir: " << out_dir << ENDL;
		exit(104);
	}
	/*
	if (stat(out_dir, &info) != 0) {
		cerr << WARNING << "Out dir doesn't exist, create it." << ENDL;
		int status = mkdir(out_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (status != 0) {
			cerr << ERROR << "Can't create specified dir: " << out_dir << ENDL;
			exit(44);
		}
	}
	*/
}

void TAggregate::Aggregate()
{
	unsigned int num_samples = 0;
	map<string, double> sum;
	map<string, double> sum2;
	map<string, STAT> stat;
	unsigned int mini;
	unsigned int N;

	for (unsigned int run : fRuns) {
		const size_t sessions = fRootFile[run].size();
		for (size_t session=0; session < sessions; session++) {
			const char *file_name = fRootFile[run][session].c_str();
      TFile fin(file_name, "read");
      if (!fin.IsOpen()) {
        cerr << ERROR << "run-" << run << " ^^^^ Can't open root file: " << file_name << ENDL;
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03ld: ", run, session)
           << file_name << ENDL;
      TTree * tin = (TTree*) fin.Get(tree);
      if (! tin) {
        cerr << ERROR << "No such tree: " << tree << " in root file: " << file_name << ENDL;
        continue;
      }

			for (auto const ftree : ftrees) {
				string file_name = ftree.second;
				if (file_name.find("xxxx") != string::npos)
					file_name.replace(file_name.find("xxxx"), 4, to_string(run));
				if (file_name.size()) {
					glob_t globbuf;
					glob(file_name.c_str(), 0, NULL, &globbuf);
					if (globbuf.gl_pathc != sessions) {
						cerr << ERROR << "run-" << run << " ^^^^ unmatched friend tree root files: " << endl 
								 << "\t" << sessions << "main root files vs " 
								 << globbuf.gl_pathc << " friend tree root files" << ENDL; 
						continue;
					}
					file_name = globbuf.gl_pathv[session];
					globfree(&globbuf);
				}
				tin->AddFriend(ftree.first.c_str(), file_name.c_str());	// FIXME: what if the friend tree doesn't exist
			}

      bool error = false;
      for (string var : fVars) {
        string branch = fVarName[var].first;
        string leaf   = fVarName[var].second;
        TBranch * br = tin->GetBranch(branch.c_str());
        if (!br) {
					// special branches -- stupid
					if (branch.find("diff_bpm11X") != string::npos && run < 3390) {	
						// no bpm11X in early runs, replace with bpm12X
						cout << WARNING << "run-" << run << " ^^^^ No branch diff_bpm11X, replace with 0.6*diff_bpm12X" << ENDL;
						br = tin->GetBranch(branch.replace(branch.find("bpm11X"), 6, "bpm12X").c_str());
					} else if (branch.find("diff_bpmE") != string::npos && run < 3390) {	
						cout << WARNING << "run-" << run << " ^^^^ No branch diff_bpmE, replace with diff_bpm12X" << ENDL;
						br = tin->GetBranch(branch.replace(branch.find("bpmE"), 4, "bpm12X").c_str());
					} 
					if (!br) {
						cerr << ERROR << "no branch: " << branch << " in tree: " << tree
							<< " of file: " << file_name << ENDL;
						error = true;
						break;
					}
        }
        TLeaf * l = br->GetLeaf(leaf.c_str());
        if (!l) {
					if (leaf.find("diff_bpm11X") != string::npos && run < 3390) {		// lagrange tree
						l = br->GetLeaf(leaf.replace(leaf.find("bpm11X"), 6, "bpm12X").c_str());
					} else if (leaf.find("diff_bpmE") != string::npos && run < 3390) {		// reg tree
						l = br->GetLeaf(leaf.replace(leaf.find("bpmE"), 4, "bpm12X").c_str());
					} 
					if (!l) {
						cerr << ERROR << "no leaf: " << leaf << " in branch: " << branch  << "in var: " << var
							<< " in tree: " << tree << " of file: " << file_name << ENDL;
						error = true;
						break;
					}
        }
        fVarLeaf[var] = l;
      }

      if (error)
        continue;

			TFile fout(Form("%s/agg_minirun_%d_%03ld.root", out_dir, run, session), "update");
			TTree * tout = (TTree*) fout.Get("mini");
			bool update=true;	// update tree: add new branches
			vector<TBranch *> brs;
			if (!tout) {
				tout = new TTree("mini", "mini");
				tout->Branch("run", &run, "run/i");
				tout->Branch("minirun", &mini, "minirun/i");
				tout->Branch("num_samples", &num_samples, "num_samples/i");
				update=false;
			} 

			for (string var : fVars) {
				if (tout->GetBranch(var.c_str())) {
					fVars.erase(var);
					continue;
				} 
				TBranch *b = tout->Branch(var.c_str(), &stat[var], "mean/D:err/D:rms/D");
				if (update)
					brs.push_back(b);
			}
			for (string custom : fCustoms) {
				if (tout->GetBranch(custom.c_str())) {
					fVars.erase(custom);
					continue;
				} 
				TBranch *b = tout->Branch(custom.c_str(), &stat[custom], "mean/D:err/D:rms/D");
				if (update)
					brs.push_back(b);
			}

			mini = 0;
			N	= tin->Draw(">>elist", mCut, "entrylist");
			TEntryList *elist = (TEntryList*) gDirectory->Get("elist");
			const int Nmini = N>9000 ? N/9000 : 1;
			if (update && Nmini != tout->GetEntries()) {
				cerr << ERROR << "Unmatch # of miniruns: "
					<< "\told: " << tout->GetEntries() 
					<< "\tnew: " << Nmini << ENDL;
			}
			for (mini=0; mini<Nmini; mini++) {
				cout << INFO << "aggregate minirun: " << mini << " of run: " << run << ENDL;
				num_samples = (mini == Nmini-1) ? N-9000*mini : 9000;
				for (int n=0; n<num_samples; n++) {
					const int en = elist->GetEntry(n+9000*mini);
					for (string var : fVars) {
						if (var.find("bpm11X") != string::npos && run < 3390)
							continue;
						fVarLeaf[var]->GetBranch()->GetEntry(en);
						double val = fVarLeaf[var]->GetValue();
						vars_buf[var] = val;
						sum[var] += val;
						sum2[var] += val * val;
					}
					for (string custom : fCustoms) {
						double val = get_custom_value(fCustomDef[custom]);
						vars_buf[custom] = val;
						sum[custom] += val;
						sum2[custom] += val * val;
					}
				}
				for (string var : fVars) {
					stat[var].mean = sum[var]/num_samples;
					stat[var].rms = sqrt((sum2[var] - sum[var]*sum[var]/num_samples)/(num_samples-1));
					stat[var].err = stat[var].rms / sqrt(num_samples);
					sum[var] = 0;
					sum2[var] = 0;
				}
				for (string custom : fCustoms) {
					stat[custom].mean = sum[custom]/num_samples;
					stat[custom].rms = sqrt((sum2[custom] - sum[custom]*sum[custom]/num_samples)/(num_samples-1));
					stat[custom].err = stat[custom].rms / sqrt(num_samples);
					sum[custom] = 0;
					sum2[custom] = 0;
				}
				if (update) {
					for (TBranch * b : brs)
						b->Fill();
				} else 
					tout->Fill();
			}

			tin->Delete();
			fin.Close();
			fout.cd();
			if (update)
				tout->Write("", TObject::kOverwrite);
			else 
				tout->Write();
			tout->Delete();
			fout.Close();
		}
	}
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
