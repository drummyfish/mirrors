#!/bin/sh

find ./ -type f \( -iname \*.h -o -iname \*.cpp -o -iname \*.fs -o -iname \*.cs -o -iname \*.vs \) | xargs -d '\n' wc -l
