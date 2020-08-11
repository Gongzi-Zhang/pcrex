SHELL			 	:= /bin/bash
CXXFLAGS		:= -std=c++11 -g -Iinclude -I.
root_libs 	 = `root-config --libs --glibs --cflags`
mysql_libs   = `mysql_config --libs --include`

TConfig			:= include/TConfig.h
TBase				:= include/TBase.h
rcdb				:= include/rcdb.h
line				:= include/line.h
const				:= include/const.h
TRun				:= include/TRun.h
math_eval		:= include/math_eval.h
io					:= include/io.h

all: check mulplot checkruns runwise runinfo
	@echo "compiling ... "

check: src/Check.cxx include/TCheckStat.h $(TConfig) $(rcdb) $(line) $(const) $(io)
	g++ $(CXXFLAGS) -o $@ src/Check.cxx $(root_libs) $(mysql_libs)

checkruns: src/CheckRuns.cxx include/TCheckRuns.h $(TConfig) $(rcdb) $(line) $(const) $(io)
	g++ $(CXXFLAGS) -o $@ src/CheckRuns.cxx $(root_libs) $(mysql_libs)

mulplot: src/MulPlot.cxx include/TMulPlot.h $(TConfig) $(rcdb) $(line) $(const) $(io)
	g++ $(CXXFLAGS) -o $@ src/MulPlot.cxx $(root_libs) $(mysql_libs)

runwise: src/RunWise.cxx include/TRunWise.h $(TConfig) $(line) $(const) $(io)
	g++ $(CXXFLAGS) -o $@ src/RunWise.cxx $(root_libs)

aggregate: src/Aggregate.cxx include/TAggregate.h $(TBase) $(TConfig) $(line) $(const) $(io)
	g++ $(CXXFLAGS) -o $@ src/Aggregate.cxx $(root_libs) $(mysql_libs)

runinfo: src/RunInfo.cxx $(rcdb) $(TRun) $(line) $(const) $(io)
	g++ $(CXXFLAGS) -o $@ src/RunInfo.cxx $(root_libs) $(mysql_libs)

dit_agg: src/dit_agg.cxx $(rcdb) $(line) $(const) $(io)
	g++ $(CXXFLAGS) -o $@ src/dit_agg.cxx $(root_libs) $(mysql_libs)

math_eval: src/math_eval.cxx $(math_eval) $(line) $(io)
	g++ $(CXXFLAGS) -o $@ src/math_eval.cxx

test: src/test.cxx $(line) $(rcdb) $(TConfig) $(io)
	g++ $(CXXFLAGS) -o $@ src/test.cxx $(root_libs) $(mysql_libs)

plot:
	tar czvf plots.tar.gz *.png 
clean:
	rm -f check mulplot checkrun
# vim: set shiftwidth=2 softtabstop=2 tabstop=2:
