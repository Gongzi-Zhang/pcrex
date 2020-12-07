github: https://github.com/Gongzi-Zhang/pcrex.git

# usage
## check: used for checking statistics of each minirun of specified runs
 * it support 4 kinds of variables:
    * solos: 
    * comparisons
    * slopes  (for postpan root file only)
    * correlations
    see check.conf about how to setup each of them

 * default config file: conf/check.conf

  e.x.
    > ./check   # use the default configuration: conf/check.conf
    > ./check -c myconf.conf -R runs.list   # specified runs in seperated run list file, instead of in config file
  all runs specified with different approaches will be checked together


### plot explanation:
  bold runs will be marked with blue and larger size dot, which bad miniruns (fail the cut) will be marked
  with red (normal size) dots.

  For comparisons, the two different variables will be marked with different colored dots, but for bold runs
  and bad miniruns, they will have the same color, though different marker styles.

## checkruns: check every event/pattern along the time
  its usage is similar to check
  > ./checkruns -h       # for help

 * default config file: conf/checkruns.conf

## mulplot: draw pair/mul plots 
  mulplot has similar usage to check
  > ./mulplot -h        # for help

 * default config file: conf/mul_plot.conf

## 
# compilation
  > make check
  > make checkrun
  > make mulplot


# todo
* run 6366: how much good data there? should we recover it?
* single arm slug
* how to cut on slow tree? e.g.  IGL1I00OD16_16 (row 83) in /chafs2/work1/apar/japanOutput/prexPrompt_pass2_6400.000.root is wrong
* TCheckStat: how to deal with slope in reg result and dit result?
* TCheckRuns: bold point cut
* TCheckRuns: should be able to fetch glitch
* TCheckRuns: sudden change in a run? (glitch, gain change, trip, ...)
* TCheckRuns: check the diff between bcms and bpms, maybe one see something while others not
* TCheckRuns: cor? how to use it?
* TCheckRuns: Correlation, two (more) variables along time

# idea
* aggregation
  > keep only one rootfile (merge sessions) for one run
    >> run-level slope will be calculated as average of each session (weighted by 1/err²)

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
