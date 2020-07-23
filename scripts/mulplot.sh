#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
pushd "$DIR/.."
for method in dit reg; do
  ./mulplot -c conf/mul_at_${method}.conf -f png -r 6359-6363,6386-6391 -n mul_at_C
  ./mulplot -c conf/mul_at_${method}.conf -f png -r 6359-6363,6386-6391 -n mul_at_C_log -l

  ./mulplot -c conf/mul_at_${method}.conf -f png -r 6349-6352,6394-6396,6398-6404 -n mul_at_Ca40
  ./mulplot -c conf/mul_at_${method}.conf -f png -r 6349-6352,6393-6396,6398-6404 -n mul_at_Ca40_log -l

  ./mulplot -c conf/mul_at_${method}.conf -f png -r 6344-6348,6354-6357,6380-6385,6405-6408 -n mul_at_Ca48
  ./mulplot -c conf/mul_at_${method}.conf -f png -r 6344-6348,6354-6357,6380-6385,6405-6408 -n mul_at_Ca48_log -l

  ./mulplot -c conf/mul_at_${method}.conf -f png -r 6367-6378 -n mul_at_Pb
  ./mulplot -c conf/mul_at_${method}.conf -f png -r 6367-6378 -n mul_at_Pb_log -l
done
popd
