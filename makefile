SHELL			 	:= /bin/bash
CXXFLAGS		:= -std=c++11 -Iinclude 
root_libs 	 = `root-config --libs --glibs --cflags`
mysql_libs   = `mysql_config --libs --include`

line_obj				:= bin/line.so
rcdb_obj				:= bin/rcdb.so
TConfig_obj			:= bin/TConfig.so
math_eval_obj		:= bin/math_eval.so
TBase_obj				:= bin/TBase.so
TRun_obj				:= bin/TRun.so

io					:= io.h
TRSbase			:= TRSbase.h
const				:= const.h
checkrs			:= TCheckRS.h
checkmini		:= TCheckMini.h
checkevent	:= TCheckEvent.h
mulplot			:= TMulPlot.h
aggregate		:= TAggregate.h
aggslug			:= TAggSlug.h
correct			:= TCorrect.h
dither			:= TDither.h

VPATH := include:src
# rcdb
$(rcdb_obj): rcdb.c $(io) 
	g++ $(CXXFLAGS) -fPIC --shared -o $@ $< $(mysql_libs)
$(TBase_obj): TBase.c $(io) 
	g++ $(CXXFLAGS) -fPIC --shared -o $@ $< $(root_libs)
$(TConfig_obj): TConfig.c $(const) 
	g++ $(CXXFLAGS) -fPIC --shared -o $@ $<

assist_obj := $(line_obj) $(math_eval_obj)
$(assist_obj): bin/%.so: %.c
	@echo "compiling $@"
	g++ $(CXXFLAGS) -fPIC --shared -o $@ $<

check := checkrs checkmini checkevent mulplot
agg		:= aggregate aggslug
run		:= runinfo getrun runtime
ana		:= reg dither
check_obj := $(addprefix bin/, $(check))
agg_obj := $(addprefix bin/, $(agg))
run_obj := $(addprefix bin/, $(run))
ana_obj := $(addprefix bin/, $(ana))

all: $(check_obj) $(agg_obj) $(run_ojb) $(anaobj) | bin

bin:
	mkdir bin

.SECONDEXPANSION:
$(check_obj) $(agg_obj) $(ana_obj): bin/%: %.cxx $$(%) $(TRSbase) $(TConfig_obj) $(TBase_obj) $(rcdb_obj) $(line_obj) $(math_eval_obj)
	g++ $(CXXFLAGS) -o $@ $^ $(root_libs) $(mysql_libs)
$(run_obj): bin/%: %.cxx $(rcdb_obj) $(line_obj)
	g++ $(CXXFLAGS) -o $@ $^ $(root_libs) $(mysql_libs)

bin/delbranch: delbranch.cxx $(line_obj) 
	g++ $(CXXFLAGS) -o $@ $^ $(root_libs)
test: test.cxx $(TConfig_obj) $(line_obj) $(math_eval_obj)
	g++ $(CXXFLAGS) -o $@ $^ $(root_libs) 

# aggregate: src/Aggregate.cxx TAggregate.h 
# 	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)
# aggslug: src/AggSlug.cxx TAggSlug.h 
# 	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)
# 
# runinfo: src/RunInfo.cxx $(rcdb) $(TRun) 
# 	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)
# getrun: src/GetRun.cxx $(rcdb) $(TRun) 
# 	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)
# 
# dit_agg: src/dit_agg.cxx $(rcdb) $(line) 
# 	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)
# 
# math_eval: src/math_eval.cxx $(math_eval) $(line) $(io)
# 	g++ $(CXXFLAGS) -o $@ $<
# 
# test: src/test.cxx 
# 	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)
# 
# tmp: src/tmp.cxx 
# 	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)

plot:
	tar czvf plots.tar.gz *.png 
clean:
	rm -f checkrs checkmini checkevent mulplot aggregate aggslug runinfo getrun

temp: scripts/agg_count.C $(rcdb) $(line) $(const) $(io)
	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)
# vim: set shiftwidth=2 softtabstop=2 tabstop=2: #
