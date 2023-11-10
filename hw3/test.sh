#!/bin/bash
set -ex
while true; do
  ./sort-gen.exe $(shuf -i 1-1000000 -n 1)
  ./sort.exe
  ./verify.sh 2>/dev/null
done
