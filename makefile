SHELL			 	:= /bin/bash
CXXFLAGS		:= -std=c++11 -g -Iinclude -I.
root_libs 	 = `root-config --libs --glibs --cflags`
mysql_libs   = `mysql_config --libs --include`

TConfig			:= include/TConfig.h
rcdb				:= include/rcdb.h
line				:= include/line.h
const				:= include/const.h

all: check mulplot checkruns runwise runinfo
	@echo "compiling ... "

check: Check.cxx TCheckStat.h $(TConfig) $(rcdb) $(line) $(const)
	g++ $(CXXFLAGS) -o $@ Check.cxx $(root_libs) $(mysql_libs)

checkruns: CheckRuns.cxx TCheckRuns.h $(TConfig) $(rcdb) $(line) $(const)
	g++ $(CXXFLAGS) -o $@ CheckRuns.cxx $(root_libs) $(mysql_libs)

mulplot: MulPlot.cxx TMulPlot.h $(TConfig) $(rcdb) $(line) $(const)
	g++ $(CXXFLAGS) -o $@ MulPlot.cxx $(root_libs) $(mysql_libs)

runwise: RunWise.cxx TRunWise.h $(TConfig) $(line) $(const)
	g++ $(CXXFLAGS) -o $@ RunWise.cxx $(root_libs)

runinfo: RunInfo.cxx $(rcdb) TRun.h $(line) $(const)
	g++ $(CXXFLAGS) -o $@ RunInfo.cxx $(root_libs) $(mysql_libs)

dit_agg: dit_agg.cxx $(rcdb) $(line) $(const)
	g++ $(CXXFLAGS) -o $@ dit_agg.cxx $(root_libs) $(mysql_libs)

math_eval: math_eval.cxx math_eval.h $(line)
	g++ $(CXXFLAGS) -o $@ math_eval.cxx

test: test.cxx $(line) $(rcdb) $(TConfig)
	g++ $(CXXFLAGS) -o $@ test.cxx $(root_libs) $(mysql_libs)

tar:
	tar czvf check.tar.gz *.h *.cxx *.conf makefile readme.md

plot:
	tar czvf plots.tar.gz *.png 
clean:
	rm -f check mulplot checkrun
# vim: set shiftwidth=2 softtabstop=2 tabstop=2:
