// const and universal definitions
#ifndef CONST_H
#define CONST_H

#include <string>
#include <map>
#include <vector>

// run infos
#define START_RUN   3000
// #define PREX_END_RUN   4980
// #define CREX_START_RUN   5000
#define END_RUN     8888
#define START_SLUG  0
#define END_SLUG    223
#define PREX_AT_START_RUN 4102
#define PREX_AT_END_RUN 4135
#define PREX_AT_START_SLUG 501
#define PREX_AT_END_SLUG 510
#define CREX_AT_START_SLUG 4000
#define CREX_AT_END_SLUG 4019
// #define ROWS      69	// number of dv in slopes
// #define COLS      5	// number of iv in slopes

// units
const double ppb = 1e-9;
const double ppm = 1e-6;
const double mm = 1e-3;
const double um = 1e-6;
const double nm = 1e-9;
const double mA = 1e-3;
const double uA = 1e-6;
const double nA = 1e-9;
const double mV = 1e-3;
const double uV = 1e-6;
const double nV = 1e-9;

std::map<std::string, const double> UNITS = {
	{"",		1},
  {"ppb", ppb},
  {"ppm", ppm},
  {"mm",  mm},
  {"um",  um},
  {"nm",  nm},
  {"mA",  mA},
  {"uA",  uA},
  {"nA",  nA},
  {"mV",  mV},
  {"uV",  uV},
  {"nV",  nV},
  {"1/mm", 1/mm},
  {"1/um", 1/um},
  {"ppm/um", ppm/um},
  {"C",   1},
};

std::vector<std::string> BURST_DV1 = {  // for runs <= 6464
	"asym_sam1",
	"asym_sam2",
	"asym_sam3",
	"asym_sam4",
	"asym_sam5",
	"asym_sam6",
	"asym_sam7",
	"asym_sam8",
	"asym_sam_15_avg",
	"asym_sam_15_dd",
	"asym_sam_37_avg",
	"asym_sam_37_dd",
	"asym_sam_26_avg",
	"asym_sam_26_dd",
	"asym_sam_48_avg",
	"asym_sam_48_dd",
	"asym_sam_13_57_dd",
	"asym_sam_15_37_dd",
	"asym_sam_24_68_dd",
	"asym_sam_26_48_dd",
	"asym_sam_1548_avg",
	"asym_sam_3748_avg",
	"asym_sam_1526_avg",
	"asym_sam_3726_avg",
	"asym_sam_15_48_dd",
	"asym_sam_37_48_dd",
	"asym_sam_15_26_dd",
	"asym_sam_37_26_dd",
	"asym_sam_1357_avg",
	"asym_sam_2468_avg",
	"asym_sam_1357_2468_dd",
	"asym_sam_12345678_avg",
	"asym_atl1",
	"asym_atl2",
	"asym_atr1",
	"asym_atr2",
	"asym_atl_avg",
	"asym_atl_dd",
	"asym_atr_avg",
	"asym_atr_dd",
	"asym_at1_avg",
	"asym_at1_dd",
	"asym_at2_avg",
	"asym_at2_dd",
	"asym_usl",
	"asym_usr",
	"asym_us_avg",
	"asym_us_dd",
};
std::vector<std::string> BURST_DV2 = {  // for runs >= 6514
	"asym_sam1",
	"asym_sam2",
	"asym_sam3",
	"asym_sam4",
	"asym_sam5",
	"asym_sam6",
	"asym_sam7",
	"asym_sam8",
	"asym_sam_15_avg",
	"asym_sam_15_dd",
	"asym_sam_37_avg",
	"asym_sam_37_dd",
	"asym_sam_26_avg",
	"asym_sam_26_dd",
	"asym_sam_48_avg",
	"asym_sam_48_dd",
	"asym_sam_13_57_dd",
	"asym_sam_15_37_dd",
	"asym_sam_24_68_dd",
	"asym_sam_26_48_dd",
	"asym_sam_1548_avg",
	"asym_sam_3748_avg",
	"asym_sam_1526_avg",
	"asym_sam_3726_avg",
	"asym_sam_15_48_dd",
	"asym_sam_37_48_dd",
	"asym_sam_15_26_dd",
	"asym_sam_37_26_dd",
	"asym_sam_1357_avg",
	"asym_sam_2468_avg",
	"asym_sam_1357_2468_dd",
	"asym_sam_12345678_avg",
	"asym_atl1",
	"asym_atl2",
	"asym_atr1",
	"asym_atr2",
	"asym_atl_avg",
	"asym_atl_dd",
	"asym_atr_avg",
	"asym_atr_dd",
	"asym_at1_avg",
	"asym_at1_dd",
	"asym_at2_avg",
	"asym_at2_dd",
  "asym_atl1r2_avg",
  "asym_atl1r2_dd",
  "asym_atr1l2_avg",
  "asym_atr1l2_dd",
  "asym_atl_dd_atr_dd_avg",
  "asym_atl_dd_atr_dd_dd",
  "asym_atl_avg_atr_avg_avg",
  "asym_atl_avg_atr_avg_dd",
	"asym_usl",
	"asym_usr",
	"asym_us_avg",
	"asym_us_dd",
};
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
