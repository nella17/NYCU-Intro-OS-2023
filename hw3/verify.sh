#!/bin/bash
set -ex
for i in {1..8}; do
  diff -Z output.txt "output_$i.txt"
done
