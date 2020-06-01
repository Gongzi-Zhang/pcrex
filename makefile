SHELL			 	:= /bin/bash
CXXFLAGS		:= -std=c++11 -g
root_libs 	 = `root-config --libs --glibs --cflags`
mysql_libs   = `mysql_config --libs --include`

all: check mulplot checkrun
	@echo "compiling ... "

check: Check.cxx TCheckStat.h TConfig.h rcdb.h line.h const.h
	g++ $(CXXFLAGS) -o $@ Check.cxx $(root_libs) $(mysql_libs)

checkrun: CheckRun.cxx TCheckRun.h TConfig.h rcdb.h line.h const.h
	g++ $(CXXFLAGS) -o $@ CheckRun.cxx $(root_libs) $(mysql_libs)

mulplot: MulPlot.cxx TMulPlot.h TConfig.h rcdb.h line.h const.h
	g++ $(CXXFLAGS) -o $@ MulPlot.cxx $(root_libs) $(mysql_libs)

runinfo: RunInfo.cxx rcdb.h TRun.h line.h const.h
	g++ $(CXXFLAGS) -o $@ RunInfo.cxx $(root_libs) $(mysql_libs)

math_eval: math_eval.cxx math_eval.h line.h
	g++ $(CXXFLAGS) -o $@ math_eval.cxx

test: test.cxx line.h rcdb.h TConfig.h
	g++ $(CXXFLAGS) -o $@ test.cxx $(root_libs) $(mysql_libs)

tar:
	tar czvf check.tar.gz *.h *.cxx *.conf makefile readme.md

plots:
	tar czvf plots.tar.gz *.png 
clean:
	rm -f check mulplot
# vim: set shiftwidth=2 softtabstop=2 tabstop=2:
