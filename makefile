SHELL			 	:= /bin/bash
CXXFLAGS		:= -std=c++11 -Iinclude 
root_libs 	 = `root-config --libs --glibs --cflags`
mysql_libs   = `mysql_config --libs --include`

io					:= include/io.h
line				:= bin/line.o
rcdb				:= bin/rcdb.o
TConfig			:= bin/TConfig.o
math_eval		:= bin/math_eval.o
TBase				:= bin/TBase.o
TRun				:= bin/TRun.o
TRSbase			:= include/TRSbase.h
const				:= include/const.h

# rcdb
$(rcdb): include/rcdb.c 
	g++ $(CXXFLAGS) -c -o $@ $(mysql_libs) $<
$(TBase): include/TBase.c
	g++ $(CXXFLAGS) -c -o $@ $(root_libs) $<

objects := $(TConfig) $(line) $(math_eval)
$(objects): bin/%.o: include/%.c
	@echo "compiling $@"
	g++ $(CXXFLAGS) -c -o $@ $<
# bin/checkrs.o: include/TCheckRS.c
# 	g++ $(CXXFLAGS) -c -o $@ $(root_libs) $<

targets := checkrs checkmini checkevent mulplot		\
					 aggregate aggslug	\
					 runinfo getrun 
all: $(targets)
$(targets): %: bin/%
	@echo "compiling $@"
checks := bin/checkrs bin/checkmini bin/checkevent bin/mulplot bin/aggregate bin/aggslug
$(checks): bin/%: src/%.cxx $(TRSbase) $(TConfig) $(TBase) $(rcdb) $(line) $(math_eval)
	g++ $(CXXFLAGS) -o $@ $^ $(root_libs) $(mysql_libs)
runs := bin/runinfo bin/getrun
$(runs): bin/%: src/%.cxx $(rcdb) $(line)
	g++ $(CXXFLAGS) -o $@ $^ $(root_libs) $(mysql_libs)


# aggregate: src/Aggregate.cxx include/TAggregate.h 
# 	g++ $(CXXFLAGS) -o $@ $< $(root_libs) $(mysql_libs)
# aggslug: src/AggSlug.cxx include/TAggSlug.h 
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
# vim: set shiftwidth=2 softtabstop=2 tabstop=2:
