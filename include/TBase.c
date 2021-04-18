#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <glob.h>
#include <libgen.h>

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TCut.h"
#include "TEntryList.h"
#include "TCollection.h"
#include "TList.h"

#include "io.h"
#include "line.h"
#include "TConfig.h"
#include "math_eval.h"
#include "TBase.h"

using namespace std;

// ClassImp(TBase);

TBase::TBase() 
{
	// gROOT->SetBatch(1);	// FIXME: maybe move to something as gsetting.h
}

TBase::~TBase() {
  cerr << INFO << "End of TBase" << ENDL;
	Free();
}

void TBase::GetConfig(const TConfig &fConf)
{
  // fConf.ParseConfFile();
  fVars	= fConf.GetVars();
  nVars = fVars.size();
  fVarAlt = fConf.GetVarAlts();

  fSolos    = fConf.GetSolos();
  fSoloCut	= fConf.GetSoloCut();
  nSolos    = fSolos.size();

  fCustoms    = fConf.GetCustoms();
  fCustomDef	= fConf.GetCustomDef();
  fCustomCut	= fConf.GetCustomCut();
  nCustoms    = fCustoms.size();

  fComps    = fConf.GetComps();
  fCompCut	= fConf.GetCompCut();
  nComps    = fComps.size();

  fSlopes		= fConf.GetSlopes();
  fSlopeCut = fConf.GetSlopeCut();
  nSlopes		= fSlopes.size();

  fCors	    = fConf.GetCors();
  fCorCut		= fConf.GetCorCut();
	nCors			= fCors.size();

  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetPattern())   pattern = fConf.GetPattern();
  if (fConf.GetTreeName())  tree    = fConf.GetTreeName();
  if (fConf.GetTreeCut()) {
		tcut	= fConf.GetTreeCut();
		Node *node = ParseExpression(tcut);
		for (string var : GetVariables(node))
			fCutVars.insert(var);
		DeleteNode(node);
	}

	ftrees = fConf.GetFriendTrees();
  ecuts = fConf.GetEntryCut();
}

void TBase::SetDir(const char * d) {
  struct stat info;
  if (stat(d, &info) != 0) {
    cerr << FATAL << "can't access specified dir: " << dir << ENDL;
    exit(30);
  } else if ( !(info.st_mode & S_IFDIR)) {
    cerr << FATAL << "not a dir: " << dir << ENDL;
    exit(31);
  }
  dir = d;
}

void TBase::CheckGrans() {
  nGrans = fGrans.size();

  for (set<int>::const_iterator it = fGrans.cbegin(); it != fGrans.cend(); ) {
    int g = *it;
    if (fRootFile[g].size()) {  // already checked
      it++;
      continue;
    }

    string p_buf(pattern);
    p_buf.replace(p_buf.find("xxxx"), 4, to_string(g));
    const char * p = Form("%s/%s", dir, p_buf.c_str());

    glob_t globbuf;
    glob(p, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << ALERT << "no root file for " << granularity << " " << g << ". Ignore it." << ENDL;
      it = fGrans.erase(it);
      continue;
    }
    for (int i=0; i<globbuf.gl_pathc; i++)
      fRootFile[g].push_back(globbuf.gl_pathv[i]);

    globfree(&globbuf);
    it++;
  }

  nGrans = fGrans.size();
  if (nGrans == 0) {
    cerr << FATAL << "No valid " << granularity << " specified!" << ENDL;
    exit(10);
  }

  cout << INFO << "" << nGrans << " valid " << granularity << " specified:" << ENDL;
  for(int g : fGrans) {
    cout << "\t" << g << endl;
  }
}

pair<string, string> TBase::ParseVar(const string var) {
  string branch, leaf;
  size_t n = count(var.begin(), var.end(), '.');
  if (n==0) {
    branch = var;
  } else if (n==1) {
    size_t pos = var.find_first_of('.');
    if (real_trees.find(var.substr(0, pos)) != real_trees.end()) {
      // FIXME: is it possible something like: tree.leaf ??? without branch name ???
      // ignore it right now
      branch = var;
    } else {
      branch = var.substr(0, pos);
      leaf = var.substr(pos+1);
    }
  } else if (n==2) {
    size_t pos = var.find_last_of('.');
    branch = var.substr(0, pos);
    leaf = var.substr(pos+1);
  } else {
    cerr << ERROR << "Invalid variable expression: " << var << endl;
    exit(24);
  }
  return make_pair(branch, leaf);
}

void TBase::CheckVars() {
  for (pair<string, string> var : fVarAlt) {
    string ori = var.first;
    string alt = var.second;
    pair<string, string> bl1 = ParseVar(ori);
    pair<string, string> bl2 = ParseVar(alt);
    string b1 = bl1.first;
    string l1 = bl1.second;
    string b2 = bl2.first;
    string l2 = bl2.second;
    if (l1.size() && l2.size() && l1 != l2) {
      cerr << ERROR << "different leave in original variable: " << ori
        << " and alternative: " << alt << ENDL;
      exit(84);
    }
    fBrAlt[b1] = b2;
  }
  for (string var : fCustoms) 
    fVarName[var] = {var, ""};  // no leaf for custom variables

  srand(time(NULL));
  int s = rand() % nGrans;
  set<int>::const_iterator it_g=fGrans.cbegin();
  for(int i=0; i<s; i++)
    it_g++;

  while (it_g != fGrans.cend()) {
		set<string> tmp_vars;
		set<string> used_ftrees;

    int g = *it_g;
    const char * file_name = fRootFile[g][0].c_str();
    TFile * f_rootfile = new TFile(file_name, "read");
    if (!f_rootfile->IsOpen()) {
      cerr << ERROR << granularity << "-" << g << ": can't read root file: " << file_name << ENDL;
			f_rootfile->Close();
      exit(44);
    }
  
    TTree *tin = (TTree*) f_rootfile->Get(tree);
    if (tin == NULL) {
      cerr << ERROR << granularity << "-" << g << ": can't read tree: " << tree 
        << " in root file: " << file_name << ENDL;
			f_rootfile->Close();
      exit(44);
    }

    for (auto const ftree : ftrees) {
      const char *texp = ftree.first.c_str();
      string file_name = ftree.second;
			if (file_name.find("xxxx") != string::npos)
				file_name.replace(file_name.find("xxxx"), 4, to_string(g));
      if (file_name.size()) {
        glob_t globbuf;
        glob(file_name.c_str(), 0, NULL, &globbuf);
        if (globbuf.gl_pathc == 0) {
          cerr << WARNING << granularity << "-" << g << ": can't read friend tree: " << texp
							 << " in root file: " << file_name << ", skip it." << ENDL; 
          globfree(&globbuf);
					goto next;
				}
				file_name = globbuf.gl_pathv[0];
				globfree(&globbuf);
      }
      tin->AddFriend(texp, file_name.c_str());	// FIXME: what if the friend tree doesn't exist
      int pos = Index(texp, '=');
      string alias = pos > 0 ? StripSpaces(Sub(texp, 0, pos)) : texp;
      // const char *old_name = pos > 0 ? StripSpaces(Sub(texp, pos+1)) : texp;
			if (real_trees.find(alias) != real_trees.end()) {
				cerr << FATAL << "repeated tree definition\n"
						 << "\told: " << real_trees[alias] << "\n"
						 << "\tnew: " << texp << ENDL;
				exit(404);
			}
      real_trees[alias] = texp;
      // it is user's responsibility to make sure each tree has an unique name
    }

    cout << INFO << "use the following root files to check vars: \n";
		cout << "\t" << file_name;
		for (auto const ftree : ftrees) {
			if (ftree.second[0])
				cout << "\n\t" << ftree.second;
		}
		cout << ENDL;

		// check variables in cut
		tmp_vars.clear();
		for (string var : fVars)
			tmp_vars.insert(var);
		for (string var : fCutVars)
			tmp_vars.insert(var);

		for (string var : tmp_vars) {
      string branch, leaf;
      pair<string, string> names = ParseVar(var);
      branch = names.first;
      leaf = names.second;

      TBranch * bbuf = tin->GetBranch(branch.c_str());
			if (!bbuf) {  // try leaf directly
				if (Count(var.c_str(), '.') == 0) {	
					TLeaf *l = tin->GetLeaf(branch.c_str());	
					if (l != NULL) {
						leaf = branch;
						bbuf = l->GetBranch();
						branch = bbuf->GetName();
					}
				}
      }
      if (!bbuf) {  // try alternatvie, if there is one
        if (fBrAlt.find(branch) != fBrAlt.end()) {
          // Note: we don't replace leaf here, so leaf must be specified 
          // in original variable if you want a specific leaf 
          bbuf = tin->GetBranch(fBrAlt[branch].c_str());
        }
      }
      if (!bbuf) {
        cerr << ERROR << "no branch (leaf): " << branch << " as in var: " << var << ENDL;
        tin->Delete();
        f_rootfile->Close();
        exit(24);
      }

			TObjArray * l_leaf = bbuf->GetListOfLeaves();
			if (leaf.size() == 0) {
				leaf = l_leaf->At(0)->GetName();  // use the first leaf
			}
			TLeaf * lbuf = (TLeaf *) l_leaf->FindObject(leaf.c_str());
			if (!lbuf) {
				cerr << WARNING << "No such leaf: " << leaf << " in var: " << var << ENDL;
				cout << DEBUG << "List of valid leaves:" << ENDL;
				TIter next(l_leaf);
				TLeaf *l;
				while (l = (TLeaf*) next()) {
					cout << "\t" << l->GetName() << endl;
				}
				tin->Delete();
				f_rootfile->Close();
				exit(24);
			}
			fVarName[var] = make_pair(branch, leaf);
      if (leaf == "mean") { // add corresponding err variables
        string errvar = branch + ".err";  
        fVars.insert(errvar);
        fVarName[errvar] = make_pair(branch, "err");
      }
			used_ftrees.insert(bbuf->GetTree()->GetName());
		}
		tmp_vars.clear();

		if (used_ftrees.find(tree) == used_ftrees.end()) {
			cerr << WARNING << "unsed main tree: " << tree << ENDL;
		} else {
			used_ftrees.erase(used_ftrees.find(tree));
		}
		for (map<string, const char*>::const_iterator it=ftrees.cbegin(); it!=ftrees.cend(); ) {
			bool used = false;
			string expr = it->first;
			string tname = expr;
			if (expr.find('=') != string::npos)
				tname = expr.substr(expr.find('='));
			for (auto const uftree : used_ftrees) {
				if (uftree == tname) {
					used = true;
					break;
				}
			}
			if (used)
				it++;
			else {
				cerr << WARNING << "unused friend tree: " << it->first << ". remove it." << ENDL;
				it = ftrees.erase(it);
			}
		}

		tin->Delete();
		f_rootfile->Close();
		break;
      
next:
		f_rootfile->Close();
		if (tin) {
			tin->Delete();
			tin = NULL;
		}
    it_g = fGrans.erase(it_g);

    if (it_g == fGrans.cend())
      it_g = fGrans.cbegin();
  }

  nGrans = fGrans.size();
  if (nGrans == 0) {
    cerr << FATAL << "no valid runs/slugs, aborting." << ENDL;
    exit(10);
  }

  nVars = fVars.size();
  nSlopes = fSlopes.size();
  if (nVars == 0 && nSlopes == 0) {
    cerr << FATAL << "no valid variables specified, aborting." << ENDL;
    exit(11);
  }

  for (vector<string>::iterator it=fSolos.begin(); it!=fSolos.end();) {
    if (fVars.find(*it) == fVars.cend()) {
			map<string, VarCut>::const_iterator it_c = fSoloCut.find(*it);
			if (it_c != fSoloCut.cend())
				fSoloCut.erase(it_c);

			cerr << WARNING << "Invalid solo variable: " << *it << ENDL;
      it = fSolos.erase(it);
		} else
      it++;
  }

	for (vector<string>::iterator it=fCustoms.begin(); it!=fCustoms.end(); ) {
		if (!CheckCustomVar(fCustomDef[*it])) {
			map<string, VarCut>::const_iterator it_c = fCustomCut.find(*it);
			if (it_c != fCustomCut.cend())
				fCustomCut.erase(it_c);

			cerr << WARNING << "Invalid custom variable: " << *it << ENDL;
      it = fCustoms.erase(it);
		} else
      it++;
	}

  for (vector<pair<string, string>>::iterator it=fComps.begin(); it!=fComps.end(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
				// || strcmp(GetUnit(it->first), GetUnit(it->second)) != 0 ) 
			map<pair<string, string>, VarCut>::const_iterator it_c = fCompCut.find(*it);
			if (it_c != fCompCut.cend())
				fCompCut.erase(it_c);

			cerr << WARNING << "Invalid Comp variable: " << it->first << "\t" << it->second << ENDL;
      it = fComps.erase(it);
		} else 
			it++;
  }

  for (vector<pair<string, string>>::iterator it=fCors.begin(); it!=fCors.end(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
			map<pair<string, string>, VarCut>::const_iterator it_c = fCorCut.find(*it);
			if (it_c != fCorCut.cend())
				fCorCut.erase(it_c);

			cerr << WARNING << "Invalid Cor variable: " << it->first << "\t" << it->second << ENDL;
      it = fCors.erase(it);
		} else
      it++;
  }

  nSolos = fSolos.size();
  nComps = fComps.size();
  nCors  = fCors.size();
  nCustoms = fCustoms.size();

	if (nSolos)
	{
		cout << INFO << "" << nSolos << " valid solo variables specified:" << ENDL;
		for(string solo : fSolos) 
			cout << "\t" << solo << endl;
  }
	if (nComps)
	{
		cout << INFO << "" << nComps << " valid comparisons specified:" << ENDL;
		for(pair<string, string> comp : fComps)
			cout << "\t" << comp.first << " , " << comp.second << endl;
  }
	if (nSlopes)
	{
		cout << INFO << "" << nSlopes << " valid slopes specified:" << ENDL;
		for(pair<string, string> slope : fSlopes)
			cout << "\t" << slope.first << " : " << slope.second << endl;
  }
	if (nCors)
	{
		cout << INFO << "" << nCors << " valid correlations specified:" << ENDL;
		for(pair<string, string> cor : fCors)
			cout << "\t" << cor.first << " : " << cor.second << endl;
  }
	if (nCustoms)
	{
		cout << INFO << "" << nCustoms << " valid customs specified:" << ENDL;
		for(string custom : fCustoms)
			cout << "\t" << custom << endl;
  }
}

bool TBase::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend() && find(fCustoms.cbegin(), fCustoms.cend(), var) == fCustoms.cend()) {
    cerr << WARNING << "Unknown variable: " << var << ENDL;
    return false;
  }
  return true;
}

bool TBase::CheckCustomVar(Node * node) {
  if (node) {
		if (	 node->token.type == variable 
				&& find(fCustoms.cbegin(), fCustoms.cend(), node->token.value) == fCustoms.cend()
				&& fVars.find(node->token.value) == fVars.cend() ) {
			cerr << WARNING << "Unknown variable: " << node->token.value << ENDL;
			return false;
		}

		bool l = CheckCustomVar(node->lchild);
		bool r = CheckCustomVar(node->rchild);
		bool s = CheckCustomVar(node->sibling);
		return l && r && s;
	}
	return true;
}

/* GetValues consideration:
 * 1. auxiliary cut: e.g. cut on number of entries (good patterns < 4500)
 * 5. special variables: bpm11X
 * 7. provide some statistical features of the values
 */
int TBase::GetValues() {  
	// initialization
  for (string var : fVars) {
    fVarValue[var].clear();
    fVarSum[var] = 0;
    // fVarSum2[var] = 0;
    fVarMax[var] = -1e9;
		fVarMin[var] = 1e9;
  }
  for (string custom : fCustoms) {
    fVarValue[custom].clear();
		fVarSum[custom] = 0;
		// fVarSum2[custom] = 0;
		fVarMax[custom] = -1e9;
		fVarMin[custom] = 1e9;
  }

	nTotal = 0;
  nOk = 0;
  for (int g : fGrans) {
    const int sessions = fRootFile[g].size();
    for (int session=0; session < sessions; session++) {
      fEntryNumber[g][session].clear(); // initialization
      const char *file_name = fRootFile[g][session].c_str();
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << ERROR << granularity << "-" << g << ": Can't open root file: " << file_name << ENDL;
        continue;
      }

      cout << INFO << Form("Read %s: %d, session: %03d\t", granularity, g, session)
           << file_name << ENDL;
      TTree * tin = (TTree*) f_rootfile.Get(tree);
      if (! tin) {
        cerr << ERROR << "No such tree: " << tree << " in root file: " << file_name << ENDL;
				f_rootfile.Close();
        continue;
      }

			for (auto const ftree : ftrees) {
				string file_name = ftree.second;
				if (file_name.find("xxxx") != string::npos)
					file_name.replace(file_name.find("xxxx"), 4, to_string(g));
				if (file_name.size()) {
					glob_t globbuf;
					glob(file_name.c_str(), 0, NULL, &globbuf);
					if (globbuf.gl_pathc != sessions) {
						cerr << ERROR << granularity << "-" << g << ": unmatched friend tree root files: " << endl 
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
        TBranch *br = tin->GetBranch(branch.c_str());
        if (!br) {
          if (fBrAlt.find(branch) != fBrAlt.end()) {
            br = tin->GetBranch(fBrAlt[branch].c_str());
            fVarUseAlt[branch].push_back(g);
            cerr << WARNING << "use alternative branch: " << fBrAlt[branch] 
              << " for branch: " << branch << " in var: " << var << ENDL;
          }
        }
        if (!br) {
					cerr << ERROR << "no branch: " << branch << " in tree: " << tree
						<< " of file: " << file_name << ENDL;
					error = true;
					break;
        }
        TLeaf * l = br->GetLeaf(leaf.c_str());
        if (!l) {
          cerr << ERROR << "no leaf: " << leaf << " in branch: " << branch  << "in var: " << var
            << " in tree: " << tree << " of file: " << file_name << ENDL;
          error = true;
          break;
        }
        fVarLeaf[var] = l;
      }

      if (error)
        continue;

      const int N = tin->Draw(">>elist", tcut, "entrylist");
      TEntryList *elist = (TEntryList*) gDirectory->Get("elist");
      cout << INFO << "use cut: " << tcut << ENDL;
      if (N == 0) {
        cerr << WARNING << granularity << "-" << g << " session-" << session << " fails cut: " << tcut << ENDL;
        continue;
      }
      for(int n=0; n<N; n++) { // loop through the events
        if (n % 50000 == 49999) 
          cout << INFO << "read " << n+1 << " entries!" << ENDL;

        const int en = elist->GetEntry(n);
        if (CheckEntryCut(nTotal+en))
          continue;

        nOk++;
        fEntryNumber[g][session].push_back(en);
        for (string var : fVars) {
          double val;
          fVarLeaf[var]->GetBranch()->GetEntry(en);
          val = fVarLeaf[var]->GetValue();

          vars_buf[var] = val;
          fVarValue[var].push_back(val);
          fVarSum[var]	+= val;
          // fVarSum2[var] += val * val;

          if (val > fVarMax[var])
            fVarMax[var] = val;
					if (val < fVarMin[var])
						fVarMin[var] = val;
        }
      }

			fEntries[g][session] = tin->GetEntries();
			nTotal += fEntries[g][session];

      tin->Delete();
      f_rootfile.Close();
    }
  }

  cout << INFO << "with cut: " << tcut <<  "--read " << nOk << "/" << nTotal << " good entries." << ENDL;
	return nOk;
}

void TBase::GetCustomValues() {
  if (fCustoms.size() == 0)
    return;

  set<string> cvars;
  for (string c : fCustoms) {
    for (string var : GetVariables(fCustomDef[c]))
      cvars.insert(var);
  }

	for (int i=0; i<nOk; i++) {
		for (string var : cvars)
			vars_buf[var] = fVarValue[var][i];
		for (string c : fCustoms) {
			double val = get_custom_value(fCustomDef[c]);
			vars_buf[c] = val;
			fVarValue[c].push_back(val);
			// fVarSum[c]  += val;
			// fVarSum2[c] += val * val;

			if (val > fVarMax[c]) 
				fVarMax[c] = val;
			if (val < fVarMin[c]) 
				fVarMin[c] = val;

			fVarSum[c] += val;
    }
  }
}

double TBase::get_custom_value(Node *node) 
{
	if (!node) 
	{
		cerr << ERROR << "Null node" << ENDL;
		return 0./0.;
	}

	const char * val = node->token.value;
	double v=0, vl=0, vr=0;
	if (node->lchild) vl = get_custom_value(node->lchild);
	if (node->rchild) vr = get_custom_value(node->rchild);
	if (std::isnan(vl) || std::isnan(vr))
		return 0./0.;

	switch (node->token.type) 
	{
		case opt:
			switch(val[0]) 
			{
				case '+':
					return vl + vr;
				case '-':
					return vl - vr;
				case '*':
					return vl * vr;
				case '/':
					return vl / vr;
				case '%':
					return ((int)vl) % ((int)vr);
			}
		case function1:
			return Getf1Value(val, vl);
		case function2:
			return Getf2Value(val, vl, get_custom_value(node->lchild->sibling));
		case number:
			return atof(val);
		case variable:
			if (	 fVars.find(val) != fVars.cend()
					|| find(fCustoms.cbegin(), fCustoms.cend(), val) != fCustoms.cend())
				return vars_buf[val];
		default:
			cerr << ERROR << "unkonw token type: " << GetTypeName(node->token.type) << ENDL;
			return 0./0.;
	}
	return 0./0.;
}

bool TBase::CheckEntryCut(const long entry) 
{
  if (ecuts.size() == 0) return false;

  for (pair<long, long> cut : ecuts) {
    long start = cut.first;
    long end = cut.second;
    if (entry >= start) {
      if (end == -1)
        return true;
      else if (entry < end)
        return true;
    }
  }
  return false;
}

const char * TBase::GetUnit (string var) 
{	// must return const char * to distinguish between same value units (e.g. ppm, um)
	if (var.find("asym") != string::npos)
		return "ppm";
	else if (var.find("diff") != string::npos)
		return "um";
	if (var.find("bpm") != string::npos)
		return "mm";
	else if (var.find("bcm") != string::npos)
		return "uA";
	else if (var.find("us") != string::npos || var.find("ds") != string::npos)
		return "mV";  // FIXME
	else
		return "";
}

void TBase::PrintStatistics() 
{
	for (string var : fVars) 
		printf("OUTPUT\t%s\t%13.9g\n", var.c_str(), fVarSum[var]);
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
