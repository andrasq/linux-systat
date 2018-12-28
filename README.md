systat
======

BSD-like `systat(1)` utility for Linux similar to `vmstat -vm`.  Displays memory,
process, interrupt, disk and network activity.

Usage: `systat [interval]`

To build, check out repo and

    cc -Os -o systat systat.c -lncurses
    sudo install ./systat /usr/local/bin/


Change Log
----------

- 0.9.31 - first github version; fix disk stats when lots of network interfaces


Related
-------

- `top` - shows a little 
- `systat` - still available on BSD
