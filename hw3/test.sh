#!/bin/bash
set -ex
make
for i in {1..8}; do
  diff output.txt "output_$i.txt"
done
