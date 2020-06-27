#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
pushd "$DIR/.."

TEMP=$(mktemp 2>/dev/null)
trap 'rm -f $TEMP' EXIT
OUT="outlier_runs"

while [ $# -gt 0 ]; do
  runs=$1
  shift
  start_run=$(echo $runs | cut -d'-' -f1)
  end_run=$(echo $runs | cut -sd'-' -f2)
  [ -z "$end_run" ] && end_run=$start_run
  for ((run=$start_run; run<=$end_run; run++)); do
    ./checkruns -c conf/respin2.conf -r $run 2>&1 1>$TEMP
    if grep -q "outlier" $TEMP; then
      mv $TEMP ${run}_out
      echo $run
      echo $run >> $OUT
    fi
    rm $TEMP
  done
done
popd
