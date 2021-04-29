#!/usr/bin/env bash

filepath="open-test-cases/sysy/section1/functional_test"
outpath="logs"
test_files=$(ls $filepath | grep .sy)

for file in $test_files
do
  i_path=$filepath/$file
  o_path=$outpath/$file.eeyore
  #printf "TEST CASE: %s %s""$i_path""$o_path""\n"
  ./cmake-build-debug/sysyc "$i_path" "$o_path"
done