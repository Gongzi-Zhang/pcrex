#include <iostream>
#include <set>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TList.h"

#include "io.h"
#include "line.h"

using namespace std;
void usage() {
  cout << "Delete tree branches in a rootfile" << endl
       << "  Options:" << endl
       << "\t -h: print this help message" << endl
       << "\t -f: which rootfile to operate" << endl
       << "\t -t: which tree to operate" << endl
       << "\t -b: branches to be deleted, separated by comma" << endl
       << endl
       << "  Example:" << endl
       << "\t ./delbranch -f rootfiles/agg_minirun_6666.root -t mini -b asym_usl,asym_usr" << endl;
}

int main(int argc, char* argv[])
{
	const char *file = NULL, *tree = NULL;
	set<const char *> branches;
  char opt;
  while((opt = getopt(argc, argv, "hf:t:b:")) != -1)
  {
		switch(opt)
		{
			case 'h':
				usage();
				exit(0);
			case 'f':
				file = optarg;
				break;
			case 't':
				tree = optarg;
				break;
			case 'b':
				for (char* b : Split(optarg, ','))
					branches.insert(b);
				break;
			default:
				usage();
				exit(1);
		}
  }
	if (!file)
	{
		cerr << FATAL << "no rootfile specified." << ENDL;
		usage();
		exit(2);
	}
	if (!tree)
	{
		cerr << FATAL << "no tree specified." << ENDL;
		usage();
		exit(3);
	}
	if (!branches.size())
	{
		cerr << FATAL << "no branches specified." << ENDL;
		usage();
		exit(4);
	}
    
	TFile *f = new TFile(file, "update");
	if (!f->IsOpen())
	{
		cerr << FATAL << "unable to open rootfile: " << file << ENDL;
		exit(5);
	}
	TTree* t = (TTree*) f->Get(tree);
	if (!tree)
	{
		cerr << FATAL << "no tree: " << tree << " in rootfile" << ENDL;
		f->Close();
		exit(6);
	}

	set<string> all_branches;
	TObjArray* blist = t->GetListOfBranches();
	TIter next(blist);
	TObject *it;
	while (it = next())
		all_branches.insert(it->GetName());
	// TObjArray* llist = t->GetListOfLeaves();
	for (auto b : branches)
	{
		if (all_branches.find(b) == all_branches.end())
		{
			cerr << ERROR << "no branch: " << b << ENDL;
			cerr << DEBUG << "List of valid branches: " << endl;
			for (auto br : all_branches)
				cerr << "\t" << br << endl;
			cerr << ENDL;
			f->Close();
			exit(7);
		}
		t->SetBranchStatus(b, 0);
		// TBranch* br = t->GetBranch(b);
		// blist->Remove(br);
		// TObjArray* list = br->GetListOfLeaves();
		// TIter lnext(list);
		// while (it = lnext())
		// 	llist->Remove(it);
	}
	auto tout = t->CloneTree();
	tout->SetDirectory(0);
	f->cd();
	// loop through the tree version
	f->Delete(Form("%s;*", tree));
	f->Flush();

	tout->Write("", TObject::kOverwrite);
	f->Flush();
	f->Close();
	// delete t;
	delete tout;
	delete f;
}

/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
