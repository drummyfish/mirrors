#!/bin/sh

echo "test 0:"             >  measure_log.txt
./main -m -W0              >> measure_log.txt

echo "test 1:"             >> measure_log.txt
./main -m -W1              >> measure_log.txt

echo "test 2:"             >> measure_log.txt
./main -m -W0 -S1          >> measure_log.txt

echo "test 3:"             >> measure_log.txt
./main -m -W1 -S1          >> measure_log.txt

echo "test 4:"             >> measure_log.txt
./main -m -W0 -n           >> measure_log.txt

echo "test 5:"             >> measure_log.txt
./main -m -W1 -n           >> measure_log.txt

echo "test 6:"             >> measure_log.txt
./main -m -W0 -e           >> measure_log.txt

echo "test 7:"             >> measure_log.txt
./main -m -W1 -e           >> measure_log.txt

echo "test 8:"             >> measure_log.txt
./main -m -W0 -e -n        >> measure_log.txt

echo "test 9:"             >> measure_log.txt
./main -m -W1 -e -n        >> measure_log.txt

echo "test 10:"            >> measure_log.txt
./main -m -C0              >> measure_log.txt

echo "test 11:"            >> measure_log.txt
./main -m -C3              >> measure_log.txt

echo "test 12:"            >> measure_log.txt
./main -m -a -e -C0        >> measure_log.txt

echo "test 13:"            >> measure_log.txt
./main -m -a -C3           >> measure_log.txt

echo "test 14:"            >> measure_log.txt
./main -m -W0 -a           >> measure_log.txt

echo "test 15:"            >> measure_log.txt
./main -m -W1 -a -e        >> measure_log.txt

echo "test 16:"            >> measure_log.txt
./main -m -W1 -a -n        >> measure_log.txt

echo "test 17:"            >> measure_log.txt
./main -m -W1 -a -e -n     >> measure_log.txt

echo "test 18:"            >> measure_log.txt
./main -m -W1 -a -e -s     >> measure_log.txt

echo "test 19:"            >> measure_log.txt
./main -m -W1 -a -e -s -n  >> measure_log.txt

echo "test 20:"            >> measure_log.txt
./main -m -W2 -a -n        >> measure_log.txt
