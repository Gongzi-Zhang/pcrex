# usage
## check
    the program ./check is used for checking statistics of each minirun of specified runs; which support 4 kinds of variables:
    * solos: 
    * comparisons
    * slopes
    * correlations
    see check.conf to how to setup each of them

  e.x.
    ./check   # use the default configuration: check.conf
    ./check -c myconf.conf -l 6666    # check 'latest' run: which will compared it to previous 10 production runs
    ./check -c myconf.conf -R runs.list   # specified runs in seperated run list file, instead of in config file
  all runs specified with different approaches will be checked together, unless you use '-l' option, which will
  ignore all other runs, and check only 'latest' run and previous 10 runs

### plot explanation:
  bold runs will be marked with blue and larger size dot, which bad miniruns (fail the cut) will be marked
  with red (normal size) dots.

  For comparisons, the two different variables will be marked with different colored dots, but for bold runs
  and bad miniruns, they will have the same color, though different marker styles.

## mulplot
    mulplot has similar usage to check

# compilation
  > make check
  > make mulplot


# todo
* single arm slug

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
