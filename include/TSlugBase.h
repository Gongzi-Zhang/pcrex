#ifndef TRUNBAS3
#define TSLUGBASE_H
#include "TBase.h"

class TSlugBase : public TBase {
  protected:
    set<int> fSlugs;

    int nSlugs;
  public:
    TSlugBase();
    ~TSlugBase() {}
    void SetSlugs(set<int> slugs);
		void CheckSlugs();
};

TSlugBase::TSlugBase() : TBase() {
}

void TSlugBase::SetSlugs(set<int> slugs) {
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

void TSlugBase::CheckSlugs() {
	fGrans = fSlugs;
	CheckGrans();
	fSlugs = fGrans;
  nSlugs = fSlugs.size();
}

#endif	// TSLUGBASE_H
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
