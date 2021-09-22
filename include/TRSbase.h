#ifndef TRSBASE_H
#define TRSBASE_H

#include <sys/stat.h>
#include "const.h"
#include "rcdb.h"
#include "TBase.h"

const char *out_dir = "rootfiles";

void CheckOutDir() 
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

void SetOutDir(const char *dir) 
{
	if (!dir) {
		cerr << FATAL << "Null out dir value!" << ENDL;
		exit(104);
	}
	out_dir = dir;
	CheckOutDir();
}

class TRSbase : public TBase {
  protected:
    set<int> fSlugs;
    set<int> fRuns;

    int nSlugs;
    int nRuns;
    map<int, int> fSlugSign;
    map<int, int> fRunSign;
    map<int, int> fRunArm;

  public:
    TRSbase();
    ~TRSbase();
    void SetSlugs(set<int> slugs);
    void SetRuns(set<int> runs);
		void CheckSlugs();
		void CheckRuns();
    void GetSlugInfo();
    void GetRunInfo();
};

TRSbase::TRSbase() : TBase() {
  StartConnection();
}

TRSbase::~TRSbase() {
  EndConnection();
}

void TRSbase::SetSlugs(set<int> slugs) {
  for(int slug : slugs) {
    if (   (CREX_AT_START_SLUG <= slug && slug <= CREX_AT_END_SLUG)
        || (PREX_AT_START_SLUG <= slug && slug <= PREX_AT_END_SLUG)
        || (        START_SLUG <= slug && slug <= END_SLUG) ) { 
      fSlugs.insert(slug);
    } else {
      cerr << ERROR << "Invalid slug number (" << START_SLUG << "-" << END_SLUG << "): " << slug << ENDL;
      continue;
    }
  }
  nSlugs = fSlugs.size();
}

void TRSbase::SetRuns(set<int> runs) {
  for(int run : runs) {
    if (run < START_RUN || run > END_RUN) {
      cerr << ERROR << "Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << ENDL;
      continue;
    }
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
}

void TRSbase::CheckRuns() {
  // check runs against database
  for (int slug : fSlugs)
    for (int run : GetRunsFromSlug(slug))
      fRuns.insert(run);
	GetValidRuns(fRuns);
	fGrans = fRuns;
	CheckGrans();
	fRuns = fGrans;
  nRuns = fRuns.size();
}

void TRSbase::CheckSlugs() {
  GetValidSlugs(fSlugs);
	fGrans = fSlugs;
	CheckGrans();
	fSlugs = fGrans;
  nSlugs = fSlugs.size();
}

void TRSbase::GetRunInfo() {
	for (int run : fRuns) {
		fRunSign[run] = GetRunSign(run);
		fRunArm[run]	= GetRunArmFlag(run);
	}
}

void TRSbase::GetSlugInfo() {
  for (int slug : fSlugs) 
    fSlugSign[slug] = GetSlugSign(slug);
}

#endif	// TRSBASE_H
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
