#!/bin/bash

python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool "$2" create + format "$3" ffs
python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool "$2" boot install
python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool "$2" boot show

for d in $1/* ; do
    echo "add $d"
    python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool "$2" write "$d"
done

python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool "$2" list