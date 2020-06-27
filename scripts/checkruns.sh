#!/bin/bash

if [ $# -eq 0 ]; then
    echo "At least one parameter needed"
    echo "Usage: $0 run run-ranges ..."
fi

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
pushd "$DIR/.."

TEMP=$(mktemp 2>/dev/null)
trap 'rm -f $TEMP' EXIT
OUT="outlier_runs"

conf="respin2.conf"

while [ $# -gt 0 ]; do
  runs=$1
  shift
  start_run=$(echo $runs | cut -d'-' -f1)
  end_run=$(echo $runs | cut -sd'-' -f2)
  [ -z "$end_run" ] && end_run=$start_run
  for ((run=$start_run; run<=$end_run; run++)); do
    echo "Processing run: $run"
    ./checkruns -c $conf -r $run -f png &>$TEMP
    if grep -q "OUTLIER" $TEMP; then
      cp $TEMP ${run}_out
      mv "checkruns_bcm_an_us-bcm_an_ds.png" "${run}.png"
      echo $run
      echo $run >> $OUT
    fi
    rm -f $TEMP
  done
done
popd
