#!/bin/sh

./main -m -W0              >  measure_log.txt
./main -m -W1              >> measure_log.txt
./main -m -W0 -S1          >> measure_log.txt
./main -m -W1 -S1          >> measure_log.txt
./main -m -W0 -n           >> measure_log.txt
./main -m -W1 -n           >> measure_log.txt
./main -m -W0 -e           >> measure_log.txt
./main -m -W1 -e           >> measure_log.txt
./main -m -W0 -e -n        >> measure_log.txt
./main -m -W1 -e -n        >> measure_log.txt

./main -m -W0 -a           >> measure_log.txt
./main -m -W1 -a -e        >> measure_log.txt
./main -m -W1 -a -n        >> measure_log.txt
./main -m -W1 -a -e -n     >> measure_log.txt

./main -m -W1 -a -e -s     >> measure_log.txt
./main -m -W1 -a -e -s -n  >> measure_log.txt