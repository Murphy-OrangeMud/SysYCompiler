#!/usr/bin/env bash

filepath="../open-test-cases/sysy/section1/functional_test"

cd cmake-build-debug || exit

for (( i = 0; i < 100; i++ ))
do
  i_path=$filepath/$(printf "%02d_*.sy" $i)
  o_path=$filepath/$(printf "%02d.eeyore" $i)
  ./sysyc "$i_path" "$o_path"
  # printf "%s %s\n" "$i_path" "$o_path"
done