#!/usr/bin/env bash

filepath="open-test-cases/sysy/section1/functional_test"
test_files=$(ls $filepath | grep .sy)

for file in $test_files
do
  i_path=$filepath/$file
  printf "TEST CASE: %s""$file""\n"
  ./cmake-build-debug/sysyc "$i_path"
done