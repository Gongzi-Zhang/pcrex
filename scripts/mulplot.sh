#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
pushd "$DIR/.."
for target in C Ca40 Ca48 Pb; do
  for method in dit reg; do
    ./mulplot -c conf/mul_at_${target}_${method}.conf -n mul_at_${target}_${method} -f png
    ./mulplot -c conf/mul_at_${target}_${method}.conf -n mul_at_${target}_${method}_log -l -f png
  done
done
popd
