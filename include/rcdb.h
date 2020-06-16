#ifndef rcdb_H
#define rcdb_H

#include <iostream>
#include <glob.h>
#include <assert.h>
#include <cstring>
#include <set>
#include <map>
#include "line.h"

#include "mysql.h"

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
 * 38    text_value   flip_state  (FLIP-LEFT/FLIP-RIGHT)
 * 39     int_value   arm_flag
 */

using namespace std;

MYSQL *con;
MYSQL_RES  *res;
MYSQL_ROW   row;
char query[256];

void    StartConnection();
char *  GetExperiment(const int run);
char *  GetRunType(const int run);
// float   GetRunCurrent(const int run);
char *  GetRunFlag(const int run);
char *  GetTarget(const int run);
int     GetSlugNumber(const int run);
int     GetArmFlag(const int run);
char *  GetIHWP(const int run);
char *  GetWienFlip(const int run);
void    GetValidRuns(set<int> &runs);
set<int>  GetRunsFromSlug(const int slug);
map<int, int> GetSign(set<int> runs);
void    EndConnection();
void    RunTests();



void StartConnection() {
  con = mysql_init(NULL);
  if (!con) {
    cerr << "MySQL Initialization failed!\n";
    exit(10);
  }
  con = mysql_real_connect(con, "hallcdb.jlab.org", "rcdb", "", "a-rcdb", 3306, NULL, 0);
  if (con)
    cerr << __PRETTY_FUNCTION__ << ":INFO\t Connection to database succeeded\n";
  else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t Connection to database failed\n";
    exit(11);
  }
}

char * GetExperiment(const int run) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=26", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch run type for run " << run << endl;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

char * GetRunType(const int run) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=3", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch run type for run " << run << endl;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

/*
float GetRunCurrent(const int run) {  // this is inaccurate
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return -1;
  }
  sprintf(query, "SELECT float_value FROM conditions WHERE run_number=%d AND condition_type_id=16", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch run current for run " << run << endl;
    return -1;
  }
  return atof(row[0]);
}
*/

char * GetRunFlag(const int run) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=28", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch run flag for run " << run << endl;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

char * GetTarget(const int run) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=18", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch target type for run " << run << endl;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

int GetSlugNumber(const int run) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return -1;
  }
  sprintf(query, "SELECT int_value FROM conditions WHERE run_number=%d AND condition_type_id=34", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch slug number for run " << run << endl;
    return -1;
  }
  return atoi(row[0]);
}

int GetArmFlag(const int run) {
  // 0: both
  // 1: right
  // 2: left
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return -1;
  }
  sprintf(query, "SELECT int_value FROM conditions WHERE run_number=%d AND condition_type_id=39", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch arm flag for run " << run << endl;
    return -1;
  }
  return atoi(row[0]);
}

char * GetIHWP(const int run) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=20", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch ihwp for run " << run << endl;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

char * GetWienFlip(const int run) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return NULL;
  }
  sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=38", run);
  mysql_query(con, query);
  res = mysql_store_result(con);
  row = mysql_fetch_row(res);
  if (row == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch slug number for run " << run << endl;
    return NULL;
  }
  StripSpaces(row[0]);
  return row[0];
}

void GetValidRuns(set<int> &runs) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return;
  }

  for(set<int>::const_iterator it=runs.cbegin(); it!=runs.cend(); ) {
    int run = *it;
    char * type = GetRunType(run);
    char * flag = GetRunFlag(run);
    if (!type || strcmp(type, "Production") != 0 || !flag || strcmp(flag, "Good") != 0) {
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t run " << run << " is not a good production run, ignore it.\n";
      it = runs.erase(it);
    } else
      it++;
  }
}

set<int> GetRunsFromSlug(const int slug) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return {};
  }

  set<int> runs;

  sprintf(query, "SELECT run_number FROM conditions WHERE condition_type_id=34 AND int_value=%d", slug);
  mysql_query(con, query);
  res = mysql_store_result(con);
  while (row = mysql_fetch_row(res)) {
    runs.insert(atoi(row[0]));
  }
  GetValidRuns(runs);
  return runs;
}

map<int, int> GetSign(set<int> runs) {
  if (!con) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t please StartConnection before anything else.\n";
    return map<int, int>();
  }

  map<int, int> signs;
  char * wien_flip;
  char * ihwp;

  for(set<int>::const_iterator it=runs.cbegin(); it!=runs.cend(); it++) {
    int run = *it;
    signs[run] = 1; // initialization
    sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=38", run);
    mysql_query(con, query);
    res = mysql_store_result(con);
    row = mysql_fetch_row(res);
    if (row == NULL) {
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch wien flip for run " << run << endl;
      signs[run] = 0;
      continue;
    }
    wien_flip = row[0];
    sprintf(query, "SELECT text_value FROM conditions WHERE run_number=%d AND condition_type_id=20", run);
    mysql_query(con, query);
    res = mysql_store_result(con);
    row = mysql_fetch_row(res);
    if (row == NULL) {
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't fetch ihwp for run " << run << endl;
      signs[run] = 0;
      continue;
    }
    ihwp = row[0];

    if (strcmp(wien_flip, "FLIP-LEFT") == 0) 
      signs[run] = 1;
    else if (strcmp(wien_flip, "FLIP-RIGHT") == 0)
      signs[run] = 2;
    else if (strcmp(wien_flip, "Vertical(UP)") == 0)
      signs[run] = 3;
    else
      signs[run] = 0; // unknow flip

    // if (strcmp
    if (strcmp(ihwp, "IN") == 0)
      signs[run] *= 1;
    else if (strcmp(ihwp, "OUT") == 0)
      signs[run] *= -1;
    else
      signs[run] = 0; // unknow flip
  }

  return signs;
}

void EndConnection() {
  if (con) {
    cerr << __PRETTY_FUNCTION__ << ":INFO\t Close Connection to database.\n";
    mysql_close(con);
  }
  con = NULL;
  res = NULL;
  row = NULL;
}


void RunTests() {
  const int run = 6666;
  StartConnection();
  assert(strcmp(GetRunType(run), "Production") == 0);
  assert(GetSlugNumber(run) == 145);
  assert(GetArmFlag(run) == 0);
  assert(strcmp(GetIHWP(run), "OUT") == 0);
  assert(strcmp(GetWienFlip(run), "FLIP-LEFT") == 0);
  EndConnection();
  cerr << __PRETTY_FUNCTION__ << ":INFO\t Pass all tests.\n";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
