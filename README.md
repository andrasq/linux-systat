systat
======

BSD-like `systat(1)` utility for Linux similar to `vmstat -vm`.  Displays memory,
process, interrupt, disk and network activity.

Usage: `systat [interval]`

To build, check out repo and

    cc -Os -o systat systat.c -lncurses
    sudo install ./systat /usr/local/bin/


Sample output:

    v0.10.3     1 users    Load  0.34  0.08  0.03                    Sun Oct 15 23:21:53.918
                Cpus 1p/6c/12t  4.750 GHz
                                                                     VN PAGER  SWAP PAGER
    Mem  15994M   Share   VM Tot    Share    Free                    in out      in out
    Act                                    812120  Mem
    All  765224   48052   11261M   344488          in kb
                                                                        cow   Interrupts
    Proc:r   p   d   s   w  Csw  New  Int  Sof  Flt              546141 zfod  4625 total
         1          0k 17550870 8320 4625 563k                          daefr      0: timer
                                                                 546304 prcfr      8: rtc0
     87.0%s   0.0%q  16.0%u   0.0%n  91.3%i   0.0%w     0.0%t           react      9: acpi
    |     |     |     |     |     |     |     |     |     |     |       pdwak    9 43: xhci_hcd
    ====>                                                               pdpgs    1 46: ahci[0000:03:00.1]
                                                                        intrn      53: nvme0q0
          lo:  Rx: 0/0 Tx: 0/0                                     3652 wire       54:56:57:58:59:61:63:66: nvme1q0
     enp34s0:  Rx: 126/2 Tx: 190/1                              4373668 act      3 79: enp34s0
      virbr1:  Rx: 0/0 Tx: 0/0                                  9038332 inact      88:89:90:91:92:93:94: nvme0q1
                                                               11249008 cache      95:104: xhci_hcd
                                                                 812120 free       116: snd_hda_intel:card2
                                                                1771760 buf        118: radeon
                                                                                   NMI: Non-maskable interrupts
                                                                              2653 LOC: Local timer interrupts
    Disks  nvme0 nvme1   sda   sdc   sr0                                           PMI: Performance monitoring interrupt
      tps                                                                          IWI: IRQ work interrupts
    xfers                                                                     1712 RES: Rescheduling interrupts
     blks                                                                      177 CAL: Function call interrupts
    %busy                                                                       61 TLB: TLB shootdowns
                                                                                   MCP: Machine check polls
                                                                                   ERR: 1
                                                                                   MIS: 0


Change Log
----------

- 0.9.34 - fix num disks displayed
- 0.9.33 - abbreviate nvme dev names, display %s and %u times as core-%, not system-%
- 0.9.31 - first github version; fix disk stats when lots of network interfaces


Related
-------

- `top` - shows a little 
- `systat` - still available on BSD
