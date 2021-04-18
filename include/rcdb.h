#ifndef rcdb_H
#define rcdb_H

#include <set>

#define botharms 0
#define rightarm 1
#define leftarm  2

using namespace std;

/* condition_type_id
 * 1		float_value   event_rate  
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
void GetValidRuns(set<int> &runs);
void GetValidSlugs(set<int> &slugs);

char *  GetRunExperiment(const int run);
char *  GetRunType(const int run);
// float   GetRunCurrent(const int run);
char *  GetRunFlag(const int run);
char *  GetRunTarget(const int run);
int     GetRunSlugNumber(const int run);
int     GetRunArmFlag(const int run);
int			GetRunSign(const int run);
char *  GetRunIHWP(const int run);
char *  GetRunWienFlip(const int run);
float	  GetRunHelicityHz(const int run);
char *  GetRunUserComment(const int run);
char *  GetRunWacNote(const int run);

char *  GetSlugIHWP(const int slug);
char *  GetSlugWienFlip(const int slug);
char *	GetSlugTarget(const int slug);
int     GetSlugSign(const int slug);
int     GetSlugArmFlag(const int slug);

void    RunTests();
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
