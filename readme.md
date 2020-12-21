# github: https://github.com/Gongzi-Zhang/pcrex.git

# usage
  1. produce aggregated files (run/minirun wise and slug wise)
  2. then used these files to plots 
  Note: of course, you can use other aggregated results, but that will not work
        with slope, because slopes are stored as an array in reg/japan output
        With aggregate, you can transfer them into solo variables

* The default experiment type is CREX, if you want to use it with PREX2, you can modify it
  in rcdb.h; (the SetExp() function is there, but I think it is needless for current analysis,
  so I there is no -e option to set exp type)

## common options
  -c config_file 
  -r 1234-2345,4356,2345-4566   :: specify runs, seperated by comma, support range
  -s 123-124,125,136-148        :: specify slugs, same syntax as runs
  -S                            :: do sign correction
  -a both,left,right            :: wanted arm flags (default all values)
  -w FLIP-LEFT,FLIT-RIGHT       :: wanted wien flips (default all states)
  -i IN,OUT                     :: wanted IHWP state (default all states)
  -f png                        :: output format (default pdf)
  -n name                       :: output name (prefix)

## checkmini: used for checking statistics of miniruns
  ./checkmini -h
## checkruns: check every event/pattern 
  ./checkruns -h
## mulplot: draw pair/mul plots 
  ./mulplot -h


# compilation
  > make check
  > make checkrun
  > make mulplot


# idea

      TBase.h
        |
        V
     TRSbase.h  TConfig.h
        |         /
        V        /
      TAgg*  TCheck*  TMulplot

* TConfig.h: read config file
* TBase.h: it does 3 things: 
  * check runs/slugs (by checking rcdb and their root files)
  * check variables
  * read values (store it in a map variable that can be used by derived classes)
* TRSbase.h: 
* draw.h: plots related stuff
  * output name/format
  * unit
* aggregation
  > keep only one rootfile (merge sessions) for one run
    >> run-level slope will be calculated as average of each session (weighted by 1/err²)
  > two types of aggregation:
    >> average (mean,err,rms)
    >> sum

# todo
* run 6366: how much good data there? should we recover it?
* how to cut on slow tree? e.g.  IGL1I00OD16_16 (row 83) in /chafs2/work1/apar/japanOutput/prexPrompt_pass2_6400.000.root is wrong
* TCheckRuns: bold point cut
* TCheckRuns: should be able to fetch glitch
* TCheckRuns: sudden change in a run? (glitch, gain change, trip, ...)
* TCheckRuns: check the diff between bcms and bpms, maybe one see something while others not
* TCheckRuns: cor? how to use it?
* TCheckRuns: Correlation, two (more) variables along time

# problem
* munmap_chunk(): invalid pointer
* end of run: double free or corruption
  -- if you have more than 2 lines in the config file that has only variable name and no ending semicomment,
  -- then you will find this bug. Which is caused by this line:
      vector<char *> fields = Split(line, ';');
  -- in ParseComp() and ParseCor() functions.

  -- I do not know why? Though by googling, i can know that is caused by invalid pointer passed
  -- to free() function.

  -- looks like no invalid-variable-name-chars (like ',', ':') are allowed in the variable name
