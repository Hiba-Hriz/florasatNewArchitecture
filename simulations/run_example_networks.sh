#!/bin/bash

parentdir=$(dirname $0)

for ini in $parentdir/examples/*; do
  if [ -f "$ini" ]; then
    echo "Started network $ini:"
    $parentdir/../src/run_flora -u Cmdenv -f $ini
  fi
done

