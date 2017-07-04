#!/bin/sh

grep "^SIMULACRUM(" $1 | \
    awk -F"(" '{print $2}' | \
    awk -F"," '{print $2}' | \
    sed 's/^[ \t]*/--wrap=/' | \
    tr "\n" ", " |\
    sed 's/,$/ /;s/)//g;s/^/-Wl,/'
