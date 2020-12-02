#ifndef rcdb_H
#define rcdb_H

#include <iostream>
#include <algorithm>
#include <glob.h>
#include <assert.h>
#include <cstring>
#include <set>
#include <map>

#include "mysql.h"

#include "line.h"
#include "const.h"

#define botharms 0
#define rightarm 1
#define leftarm  2

vector<string> armflags = {
	"both",
	"right",
	"left",
};
set<string> wienflips = {
	"FLIP-LEFT",
	"FLIP-RIGHT",
	"Vertical(UP)",
	"Vertical(DOWN)",
};
set<string> ihwps = {
	"IN",
	"OUT",
};
set<string> runflags = {
	"Bad",
	"Suspicious",
	"Good",
	"NeedCut",
	"Suspicous",
};
set<string> runtypes = {
	"Test",
	"Junk",
	"Harp",
	"Calibration",
	"Optics",
	"Production",
	"Parityscan",
	"Cosmics",
	"Pedestal",
	"A_T",
	"Q2scan",
	"Production",
};
set<string> targets = {
	"Home",
	"Halo",
	"Carbon Hole",
	"Carbon 0.25%",
	"WaterCell 2.77%",
	"Carbon Hole (Cold)",
	"D-Pb-D",
	"C-Pb-C",
	"40Ca 6%",
	"Carbon 1%",
	"D-208Pb2-D",
	"D-208Pb3-D",
	"D-208Pb4-D",
	"D-208Pb5-D",
	"D-208Pb6-D",
	"D-208Pb7-D",
	"D-208Pb8-D",
	"D-208Pb9-D",
	"D-208Pb10-D",
	"40Ca",
	"48Ca",
};
set<string> exps = {
	"PREX2",
	"CREX",
};

/* condition_type_id
 * 1    float_value   event_rate  
 * 2      int_value   event_count
 * 3     text_value   run_type
 * 4     text_value   run_config    (config file: ALL_PREX/CH_INJ/CntHouse/Injector...)
 * 5     text_value   session
 * 6     text_value   user_comment
 *
 * 10    bool_value   is_valid_run_end
 * 11     int_value   run_length    (how many seconds)
 * 
 * 15   float_value   beam_energy
 * 16   float_value   beam_current
 * 17   float_value   total_charge
 * 18    text_value   target_type
 * 
 * 20    text_value   ihwp    (IN/OUT)
 * 21   float_value   rhwp
 * 22   float_value   vertical_wien
 * 23   float_value   horizontal_wien   
 * 24    text_value   helicity_pattern  (Quartet/Octet)
 * 25   float_value   helicity_frequency
 * 26    text_value   experiment  (CREX/PREX2)
 * 27    text_value   wac_note
 * 28    text_value   run_flag    (Good/Suspecious/NeedCut...)
 * 
 * 30   float_value   target_45encoder  (-1...)
 * 31   float_value   target_90encoder  (13163100...) 
 *
 * 34     int_value   slug
 * 35    text_value   bmw
 * 36    text_value   feedback
 *
 * 38    text_value   flip_state  (FLIP-LEFT/FLIP-RIGHT/Vertical(UP)/Vertical(DOWN)/Longitudinal)
 * 39     int_value   arm_flag
 */

using namespace std;

string garmflag;
string gihwp;
string gwienflip;
string gexp = "CREX";
string gruntype = "Production";
string grunflag = "Good";
string gtarget = "48Ca";

MYSQL *con;
MYSQL_RES  *res;
MYSQL_ROW   row;
char query[256];
static char rcdb_none[5] = "none";

void StartConnection();
void EndConnection();

void SetArmFlag(const char *f);
void SetIHWP(const char *ip);
void SetWienFlip(const char *wf);
void SetExp(const char *exp);
void SetRunType(const char *rt);
void SetRunFlag(const char *rf);
void SetTarget(const char *t);
set<int>	GetRuns();
set<int>  GetRunsFromSlug(const int slug);
void    GetValidRuns(set<int> &runs);

char *  GetRunExperiment(const int run);
char *  GetRunType(const int run);
// float   GetRunCurrent(const int run);
char *  GetRunFlag(const int run);
char *  GetRunTarget(const int run);
int     GetRunSlugNumber(const int run);
int     GetRunArmFlag(const int run);
char *  GetRunIHWP(const int run);
char *  GetRunWienFlip(const int run);
float	  GetRunHelicityHz(const int run);
char *  GetRunUserComment(const int run);
char *  GetRunWacNote(const int run);
int			GetRunSign(const int run);

void    RunTests();


void StartConnection() {
  con = mysql_init(NULL);
  if (!con) {
    cerr << FATAL << "MySQL Initialization failed!" << ENDL;
    exit(10);
  }
  con = mysql_real_connect(con, "hallcdb.jlab.org", "rcdb", "", "a-rcdb", 3306, NULL, 0);
  if (con)
    cerr << INFO << "Connection to database succeeded" << ENDL;
  else {
    cerr << FATAL << "Connection to database failed" << ENDL;
    exit(11);
  }
}

void EndConnection() {
  if (con) {
    cerr << INFO << "Close Connection to database." << ENDL;
    mysql_close(con);
  }
  con = NULL;
  res = NULL;
  row = NULL;
}

void SetArmFlag(const char *fs) {
	if (!fs)
		return;

	garmflag.clear();
	for (char *f : Split(fs, ',')) {
		char af[Size(f) + 1];
		strcpy(af, f);
		StripSpaces(af);
		string af_tmp(af);
		vector<string>::iterator it = find(armflags.begin(), armflags.end(), af_tmp);
		if (it == armflags.end()) {
			cerr << FATAL << "Invalid arm flag: " << f << ENDL;
			cerr << "Valid arm flag: " << endl;
			for (string v : armflags)
				cerr << "\t" << v << endl;
			exit(25);
		}
		if (garmflag.size())
			garmflag += "|";
		garmflag += to_string(it - armflags.begin());
	}
}

void SetWienFlip(const char *fs) {
	if (!fs) 
		return;

	gwienflip.clear();
	for (char *f : Split(fs, ',')) {
		char wf[Size(f) + 1];
		strcpy(wf, f);
		StripSpaces(wf);
		if (wienflips.find(wf) == wienflips.end()) {
			cerr << FATAL << "Invalid wien flip: " << f << ENDL;
			cerr << "Valid wien flip: " << endl;
			for (string v : wienflips)
				cerr << "\t" << v << endl;
			exit(25);
		}
		if (gwienflip.size())
			gwienflip += "|";
		gwienflip += wf;
	}
}

void SetIHWP(const char *ps) {
	if (!ps)
		return;

	gihwp.clear();
	for (char *p : Split(ps, ',')) {
		char ip[Size(p) + 1];
		strcpy(ip, p);
		StripSpaces(ip);
		if (ihwps.find(ip) == ihwps.end()) {
			cerr << FATAL << "Invalid IHWP: " << p << ENDL;
			cerr << "Valid IHWP: " << endl;
			for (string v : ihwps)
				cerr << "\t" << v << endl;
			exit(25);
		}
		if (gihwp.size())
			gihwp += "|";
		gihwp += ip;
	}
}

void SetExp(const char *es) {
	if (!es)
		return;

	gexp.clear();
	for (char *e : Split(es, ',')) {
		char exp[Size(e) + 1];
		strcpy(exp, e);
		StripSpaces(exp);
		if (exps.find(exp) == exps.end()) {
			cerr << FATAL << "Invalid exp: " << e << ENDL;
			cerr << "Valid exp: " << endl;
			for (string v : exps)
				cerr << "\t" << v << endl;
			exit(25);
		}
		if (gexp.size())
			gexp += "|";
		gexp += exp;
	}
}

void SetRunType(const char *ts) {
	if (!ts) 
		return;

	gruntype.clear();
	for (char *t : Split(ts, ',')) {
		char tp[Size(t) + 1];
		strcpy(tp, t);
		StripSpaces(tp);
		if (runtypes.find(tp) == runtypes.end()) {
			cerr << FATAL << "Invalid run type: " << t << ENDL;
			cerr << "Valid run type: " << endl;
			for (string v : runtypes)
				cerr << "\t" << v << endl;
			exit(25);
		}
		if (gruntype.size())
			gruntype += "|";
		gruntype += tp;
	}
}

void SetRunFlag(const char *fs) {
	if (!fs)
		return;

	grunflag.clear();
	for (char *f : Split(fs, ',')) {
		char rf[Size(f) + 1];
		strcpy(rf, f);
		StripSpaces(rf);
		if (runflags.find(rf) == runflags.end()) {
			cerr << FATAL << "Invalid run flag: " << f << ENDL;
			cerr << "Valid run flag: " << endl;
			for (string v : runflags)
				cerr << "\t" << v << endl;
			exit(25);
		}
		if (grunflag.size())
			grunflag += "|";
		grunflag += rf;
	}
}

void SetTarget(const char *ts) {
	if (!ts)
		return;

	gtarget.clear();
	for (char *t : Split(ts, ',')) {
		char tg[Size(t) + 1];
		strcpy(tg, t);
		StripSpaces(tg);
		if (runtypes.find(tg) == runtypes.end()) {
			cerr << FATAL << "Invalid target: " << t << ENDL;
			cerr << "Valid target: " << endl;
			for (string v : targets)
				cerr << "\t" << v << endl;
			exit(25);
		}
		if (gtarget.size())
			gtarget += "|";
		gtarget += tg;
	}
}

set<int> GetRuns() {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return {};
  }

	cerr << BINFO;
		if (gexp.size())
			fprintf(stderr, "\n\t%9s:\t%s", "exp", gexp.c_str());
		if (gruntype.size())
			fprintf(stderr, "\n\t%9s:\t%s", "run type", gruntype.c_str());
		if (grunflag.size())
			fprintf(stderr, "\n\t%9s:\t%s", "run flag", grunflag.c_str());
		if (gtarget.size())
			fprintf(stderr, "\n\t%9s:\t%s", "target", gtarget.c_str());
		if (garmflag.size())
			fprintf(stderr, "\n\t%9s:\t%s", "arm flag", garmflag.c_str());
		if (gihwp.size())
			fprintf(stderr, "\n\t%9s:\t%s", "ihwp", gihwp.c_str());
		if (gwienflip.size())
			fprintf(stderr, "\n\t%9s:\t%s", "wien flip", gwienflip.c_str());
	cerr << ENDL;

  set<int> runs;
	char q_exp[128], q_runtype[128*2], q_runflag[128*3], q_target[128*4], q_armflag[128*5], q_ihwp[128*6], q_wienflip[128*7];
	if (gexp.size())
		sprintf(q_exp, "SELECT run_number FROM conditions WHERE condition_type_id=26 AND text_value regexp '%s'", gexp.c_str());

	if (gruntype.size())
		sprintf(q_runtype, "SELECT run_number FROM conditions WHERE condition_type_id=3 AND text_value regexp '%s' AND run_number in (%s)", gruntype.c_str(), q_exp);
	else 
		strcpy(q_runtype, q_exp);

	if (grunflag.size())
		sprintf(q_runflag, "SELECT run_number FROM conditions WHERE condition_type_id=28 AND text_value regexp '%s' AND run_number in (%s)", grunflag.c_str(), q_runtype);
	else
		strcpy(q_runflag, q_runtype);

	if (gtarget.size())
		sprintf(q_target, "SELECT run_number FROM conditions WHERE condition_type_id=18 AND text_value regexp '%s' AND run_number in (%s)", gtarget.c_str(), q_runflag);
	else
		strcpy(q_target, q_runflag);

	if (garmflag.size()) 
		sprintf(q_armflag, "SELECT run_number FROM conditions WHERE condition_type_id=39 AND int_value regexp '%s' AND run_number in (%s)", garmflag.c_str(), q_target);
	else 
		strcpy(q_armflag, q_target);

	if (gihwp.size())
		sprintf(q_ihwp, "SELECT run_number FROM conditions WHERE condition_type_id=20 AND text_value regexp '%s' AND run_number in (%s)", gihwp.c_str(), q_armflag);
	else
		strcpy(q_ihwp, q_armflag);

	if (gwienflip.size())
		sprintf(q_wienflip, "SELECT run_number FROM conditions WHERE condition_type_id=38 AND text_value regexp '%s' AND run_number in (%s)", gwienflip.c_str(), q_ihwp);
	else
		sprintf(q_wienflip, q_ihwp);
	// cerr << DEBUG << q_wienflip << ENDL;

  mysql_query(con, q_wienflip);
  res = mysql_store_result(con);
  while (row = mysql_fetch_row(res)) {
    runs.insert(atoi(row[0]));
  }
  // GetValidRuns(runs);
  return runs;
}

set<int> GetRunsFromSlug(const int slug) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return {};
  }

  set<int> runs;

  sprintf(query, "SELECT run_number FROM conditions WHERE condition_type_id=34 AND int_value=%d", slug);
  mysql_query(con, query);
  res = mysql_store_result(con);
  while (row = mysql_fetch_row(res)) {
    runs.insert(atoi(row[0]));
  }
  // GetValidRuns(runs);
  return runs;
}

char * GetRunExperiment(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=26", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch run type for run " << run << ENDL;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

char * GetRunType(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=3", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch run type for run " << run << ENDL;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

/*
float GetRunCurrent(const int run) {  // this is inaccurate
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return -1;
  }
  sprintf(query, "SELECT float_value FROM conditions WHERE run_number=%d AND condition_type_id=16", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch run current for run " << run << ENDL;
    return -1;
  }
  return atof(row[0]);
}
*/

char * GetRunFlag(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=28", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch run flag for run " << run << ENDL;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

char * GetRunTarget(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=18", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch target type for run " << run << ENDL;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

int GetRunSlugNumber(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return -1;
  }
  sprintf(query, "SELECT int_value FROM conditions WHERE run_number=%d AND condition_type_id=34", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch slug number for run " << run << ENDL;
    return -1;
  }
  return atoi(row[0]);
}

int GetRunArmFlag(const int run) {
  // 0: both
  // 1: right
  // 2: left
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return -1;
  }
  sprintf(query, "SELECT int_value FROM conditions WHERE run_number=%d AND condition_type_id=39", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch arm flag for run " << run << ENDL;
    return -1;
  }
  return atoi(row[0]);
}

char * GetRunIHWP(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=20", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch ihwp for run " << run << ENDL;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

char * GetRunWienFlip(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=38", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch wien flip for run " << run << ENDL;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

float GetRunHelicityHz(const int run) {  // helicity frequency
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return -1;
  }
  sprintf(query, "SELECT float_value FROM conditions WHERE run_number=%d AND condition_type_id=25", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << WARNING << "can't fetch helicity frequency for run " << run << ENDL;
    return -1;
  }
  return atof(row[0]);
}

char * GetRunUserComment(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=6", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    return rcdb_none;
  }
  StripSpaces(row[0]);
  return row[0];
}

char * GetRunWacNote(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=27", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    return rcdb_none;
  }
  StripSpaces(row[0]);
  return row[0];
}

void GetValidRuns(set<int> &runs) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else" << ENDL;
    return;
  }

	set<int> vruns = GetRuns();
  for(set<int>::const_iterator it=runs.cbegin(); it!=runs.cend(); ) {
    int run = *it;
		if (vruns.find(run) == vruns.end())
			it = runs.erase(it);
		else 
			it++;
  }
}

int GetRunSign(const int run) {
  if (!con) {
    cerr << ERROR << "please StartConnection before anything else." << ENDL;
    return 0;
  }

	int sign = 1;
  char * wien_flip;
  char * ihwp;

	sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=38", run);
	mysql_query(con, query);
	res = mysql_store_result(con);
	row = mysql_fetch_row(res);
	if (row == NULL) {
		cerr << WARNING << "can't fetch wien flip for run " << run << ENDL;
		return 0;
	}
	wien_flip = row[0];
	sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=20", run);
	mysql_query(con, query);
	res = mysql_store_result(con);
	row = mysql_fetch_row(res);
	if (row == NULL) {
		cerr << WARNING << "can't fetch ihwp for run " << run << ENDL;
		return 0;
	}
	ihwp = row[0];

	if (strcmp(wien_flip, "FLIP-LEFT") == 0) 
		sign = 1;
	else if (strcmp(wien_flip, "FLIP-RIGHT") == 0)
		sign = -2;
	else if (strcmp(wien_flip, "Vertical(UP)") == 0)
		sign = 3;
	else if (strcmp(wien_flip, "Vertical(DOWN)") == 0)
		sign = -4;
	else {
		cerr << WARNING << "unknow flip state: " << wien_flip << ENDL;
		return 0;
	}

	if (strcmp(ihwp, "IN") == 0)
		sign *= -1;
	else if (strcmp(ihwp, "OUT") == 0)
		sign *= 1;
	else {
		cerr << WARNING << "unknow ihwp state: " << ihwp << ENDL;
		return 0;
	}

  return sign;
}

void RunTests() {
  const int run = 6666;
  StartConnection();
  assert(strcmp(GetRunType(run), "Production") == 0);
  assert(GetRunSlugNumber(run) == 145);
  assert(GetRunArmFlag(run) == 0);
  assert(strcmp(GetRunIHWP(run), "OUT") == 0);
  assert(strcmp(GetRunWienFlip(run), "FLIP-LEFT") == 0);
  EndConnection();
  cerr << INFO << "Pass all tests." << ENDL;
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
