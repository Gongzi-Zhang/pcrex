SHELL			 	:= /bin/bash
CXXFLAGS		:= -std=c++11
root_libs 	 = `root-config --libs --glibs --cflags`
mysql_libs   = `mysql_config --libs --include`

all: check mulplot
	@echo "compiling ... "

check: Check.cxx TCheckStat.h TConfig.h TRun.h line.h const.h
	g++ $(CXXFLAGS) -o $@ Check.cxx $(root_libs) $(mysql_libs)

mulplot: MulPlot.cxx TMulPlot.h TConfig.h TRun.h line.h const.h
	g++ $(CXXFLAGS) -o $@ MulPlot.cxx $(root_libs) $(mysql_libs)

runinfo: RunInfo.cxx TRun.h line.h const.h
	g++ $(CXXFLAGS) -o $@ RunInfo.cxx $(root_libs) $(mysql_libs)

test: test.cxx line.h TRun.h TConfig.h
	g++ $(CXXFLAGS) -o $@ test.cxx $(root_libs) $(mysql_libs)

tar:
	tar czvf check.tar.gz *.h *.cxx *.conf makefile readme.md

plots:
	tar czvf plots.tar.gz *.png *.pdf
clean:
	rm -f check mulplot
