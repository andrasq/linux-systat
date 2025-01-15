/*
 * systat -- BSD-like systat(1) for Linux
 * Copyright (C) 2007-2025 Andras Radics
 *
 * Licensed under the Apache License, Version 2.0
 */

/*
 * NOTE: compile w/ gcc-4.1; ecc and gcc-2.7.2.3 crash on curses update
 * Gcc-3.2 and Gcc-2.95.3 also work.  Build w/ -Os -s to minimize size.
 */

#define VERSION "v0.13.0"

/**

AR: 0.9.0 todo changes:
+ include iowait, steal time
+ fill out daefr, react etc (with kswapd?)
+ size info to window
+ anchor right two columns to right edge
+ define PAGER_COL and float it with right margin (until all drives and cpu fields
  have been exposed, then leave it)
+ display more stats if fits in larger window (show more drives!)
+ extend the cpu bar by units of 10 chars at a time (one per division)
+ more network devices?
+ scale and suffix-ize output [KMGTP for 1024, kmgtp for 1000] to not glom columns
+ extend auto-scaling to G, T, P
+ accept update interval on cmdline
+ if space available, spread out the process counts (3 ch / col is not enough)
- should change all "unsigned long" to "ullong" to for 64 bits of stats
2020-04-18:
+ systat: add sysinfo: num cores, total dram, total swap, num cores, avg core mhz (read from /proc/cpuinfo)
+ systat: add "Tot" row to mem: tot REAL (phys dram installed), tot VIRTUAL (swap installed)
+ systat: scale REAL/VIRTUAL mem sooner, eg no more than 3.1 digits before switching units (KB -> MB -> GB -> TB -> PB -> EB)
+ combine nvme%dq%d interrupts (two ssds have 16 total)
+ combine eth%d-rx-%d and eth%d-tx-%d interrupts (2 each)
+ combine xhci_hcd interrupts (2 identically named)
2025-01-11
- fix occasional crash
  (gdb) where
  #0  0x00007ffff7e114a7 in ?? () from /lib/x86_64-linux-gnu/libc.so.6
  #1  0x0000000000401aea in scan_ullong ()
  #2  0x00000000004035e0 in gather_stats ()
  #3  0x00000000004017e9 in main ()
+ maybe support ' ' spacebar to refresh immediately (and correctly scale the time period denominator)
+ very first set of stats are divided by the sampling interval, should show them as absolute
- if window is small, network devices overwrite top disks rows
- cpu usage missing? eg 80% busy, but only 550% of 1200% accounted for: (%q always zero??)
              Cpus 2p/12c/12t  2.629 GHz
   130.4%s   0.0%q 404.6%u  17.7%n  19.7%i   5.9%w   0.0%t        862 react      16: ehci_hcd:usb1
  |     |     |     |     |     |     |     |     |     |     |       pdwak      23: ehci_hcd:usb2
  ===========>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>--                471 pdpgs  304 39: ahci[0000:00:1f.2]

**/

static char id[] = "@(#)systat " VERSION " -- display system activity";

/*----------------------------------------------------------------
 * The FreeBSD 2.2 systat screen (tweaked slightly by AR):
 *----------------------------------------------------------------
|---------|---------|---------|---------|---------|---------|---------|-------
    2 users    Load  0.36  0.26  0.18                  Thu Dec  5 18:44

Mem:KB    REAL            VIRTUAL                     VN PAGER  SWAP PAGER
        Tot   Share      Tot    Share    Free         in  out     in  out
Act   35220    1948    58216     2628   51244 count
All  102592    2988   510352     3940         pages
                                                          cow    Interrupts
Proc:r  p  d  s  w   Csw  Trp   Sys  Int  Sof  Flt        zfod    231 total
             11       33    2   359  231   19    4  17276 wire    100 clk0 irq0
                                                    42680 act     128 rtc0 irq8
 0.8%Sys   0.0%Intr  0.0%User  0.0%Nice 99.2%Idle   16416 inact     3 pci irq10
|    |    |    |    |    |    |    |    |    |       9860 cache       pci irq9
                                                    41384 free        pci irq15
                                                          daefr       fdc0 irq6
Namei         Name-cache    Dir-cache                     prcfr       sc0 irq1
    Calls     hits    %     hits    %                     react       sio0 irq4
                                                          pdwak       psm0 irq12
                                                          pdpgs       sb0 irq5
Discs  sd0  sd1  cd0                                      intrn
seeks                                                8358 buf
xfers
 blks
 msps

 *----------------------------------------------------------------
 * The FreeBSD 7.2 systat screen (4/2010)
 *----------------------------------------------------------------
    3 users    Load  0.38  0.71  0.79                  Apr 20 10:08

Mem:KB    REAL            VIRTUAL                       VN PAGER   SWAP PAGER
        Tot   Share      Tot    Share    Free           in   out     in   out
Act  383452   16632  1340972    39308  531748  count      
All  555216   62612  3660364   189404          pages      
Proc:                                                            Interrupts
  r   p   d   s   w   Csw  Trp  Sys  Int  Sof  Flt    425 cow    1276 total
          3 240      4122 1346  24k  476  119 1303    531 zfod        atkbd0 1
                                                          ozfod       fdc0 irq6
 2.6%Sys   0.0%Intr  2.9%User  0.0%Nice 94.5%Idle        %ozfod    29 ata0 irq14
|    |    |    |    |    |    |    |    |    |    |       daefr       ata1 irq15
=>>                                                   519 prcfr   400 cpu0: time
                                       354 dtbuf      916 totfr   447 em0 irq256
Namei     Name-cache   Dir-cache    100000 desvn          react   400 cpu1: time
   Calls    hits   %    hits   %     66461 numvn          pdwak
    1454    1412  97                 24867 frevn          pdpgs
                                                          intrn
Disks   ad0   ad2                                  231636 wire
KB/t  10.69  0.00                                  480532 act
tps      25     0                                 2092376 inact
MB/s   0.26  0.00                                  119248 cache
%busy     0     0                                  412500 free
                                                   114880 buf

 *----------------------------------------------------------------
 * The FreeBSD 8.2 systat screen (1/2018)
 *----------------------------------------------------------------
    6 users    Load  0.69  0.59  0.59                  Jan  6 20:32

Mem:KB    REAL            VIRTUAL                       VN PAGER   SWAP PAGER
        Tot   Share      Tot    Share    Free           in   out     in   out
Act 1169124   96684 12230740   248396 1115740  count
All  15096k  119152 1086600k   503044          pages
Proc:                                                            Interrupts
  r   p   d   s   w   Csw  Trp  Sys  Int  Sof  Flt    457 cow    5728 total
  2   4     490      7572 2125 5962 2528  140 2058   1359 zfod        atkbd0 1
                                                          ozfod   400 cpu0: time
 1.9%Sys   0.4%Intr  1.7%User  0.0%Nice 96.1%Idle        %ozfod  2410 em0:rx 0 2
|    |    |    |    |    |    |    |    |    |    |       daefr   117 em0:tx 0 2
=>                                                    138 prcfr       em0:link 2
                                        83 dtbuf     6342 totfr     1 ahci0 262
Namei     Name-cache   Dir-cache    460791 desvn          react   400 cpu1: time
   Calls    hits   %    hits   %    314207 numvn          pdwak   400 cpu6: time
    7202    7201 100                115185 frevn          pdpgs   400 cpu2: time
                                                          intrn   400 cpu5: time
Disks  ada0  ada1  ada2 pass0 pass1 pass2        20759648 wire    400 cpu7: time
KB/t   0.00  0.00  0.00  0.00  0.00  0.00         1069868 act     400 cpu4: time
tps       0     0     0     0     0     0         1419856 inact   400 cpu3: time
MB/s   0.00  0.00  0.00  0.00  0.00  0.00          534052 cache
%busy     0     0     0     0     0     0          587456 free
                                                  2524656 buf


bsd8.2% man systat(1)

The upper left quadrant of the screen shows the number of
users logged in and the load average over the last one, five,
and fifteen minute intervals.  Below this line are statistics
on memory utilization.  The first row of the table reports
memory usage only among active processes, that is processes
that have run in the previous twenty seconds.  The second row
reports on memory usage of all processes.  The first column
reports on the number of kilobytes in physical pages claimed
by processes.  The second column reports the number of kilo-
bytes in physical pages that are devoted to read only text
pages.  The third and fourth columns report the same two fig-
ures for virtual pages, that is the number of kilobytes in
pages that would be needed if all processes had all of their
pages.  Finally the last column shows the number of kilobytes
in physical pages on the free list.

Below the memory display is a list of the average number of
processes (over the last refresh interval) that are runnable
(`r'), in page wait (`p'), in disk wait other than paging
(`d'), sleeping (`s'), and swapped out but desiring to run
(`w').  The row also shows the average number of context
switches (`Csw'), traps (`Trp'; includes page faults), system
calls (`Sys'), interrupts (`Int'), network software inter-
rupts (`Sof'), and page faults (`Flt').

bsd7.2% man systat(1)

Under the date in the upper right hand quadrant are statis- tics on paging
and swapping activity.  The first two columns report the average number
of pages brought in and out per second over the last refresh interval
due to page faults and the paging daemon.  The third and fourth columns
report the average number of pages brought in and out per second over the
last refresh interval due to swap requests initiated by the scheduler.
The first row of the display shows the aver- age number of disk transfers
per second over the last refresh interval; the second row of the display
shows the average number of pages transferred per second over the last
refresh interval.

Below the paging statistics is a column of lines regarding the virtual
memory system.  The first few lines describe, in units (except as noted
below) of pages per second averaged over the sampling interval, pages
copied on write (`cow'), pages zero filled on demand (`zfod'), pages
optimally zero filled on demand (`ozfod'), the ratio of the (average)
ozfod / zfod as a percentage (`%ozfod'), pages freed by the page daemon
(`daefr'), pages freed by exiting processes (`prcfr'), total pages freed
(`totfr'), pages reactivated from the free list (`react'), the average
number of times per second that the page daemon was awakened (`pdwak'),
pages analyzed by the page daemon (`pdpgs'), and in-transit blocking
page faults (`intrn').  Note that the units are special for `%ozfod' and
`pdwak'.  The next few lines describe, as amounts of memory in kilobytes,
pages wired down (`wire'), active pages (`act'), inactive pages (`inact'),
pages on the cache queue (`cache'), and free pages (`free').  Note that
the values displayed are the current transient ones; they are not aver-
ages.

At the bottom of this column is a line showing the amount of
virtual memory, in kilobytes, mapped into the buffer cache (`buf').
This statistic is not useful.  It exists only as a placeholder for
the corresponding useful statistic (the amount of real memory used to
cache disks).  The most impor- tant component of the latter (the amount
of real memory used by the vm system to cache disks) is not available,
but can be guessed from the `inact' amount under some system loads.


 *----------------------------------------------------------------
 */

/*
Act: memory use by "active" processes, that have run in the last 20 sec.
All: memory use by all processes
RealTot=physical pages loaded, RealShare=text(r/o pages) pages loaded,
VirtualTot=total pages mapped, VirtualShare=total text pages mapped
Free: count of pages on the free list.
(NB: "count pages" is misnomer, it`s actually shown in megabytes)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <dirent.h>
#include <curses.h>
#include <utmp.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <math.h>
#include <ctype.h>

#include <asm/param.h>  // HZ

#define CONTROL(c)  ((c) &  ~0x60)
#define MAXCORES 4096

#define ulong unsigned long
#define uint unsigned int
#define ullong unsigned long long

ullong scan_ullong( char *buf, char *patt ) {
    if (!buf) return 0;
    char *p = strstr(buf, patt);
    if (p) return strtoull(p + strlen(patt) + 1, NULL, 10);
    else return 0;
}

static ulong _linuxver;		/* set by gather_version(), called from main */
int _pageKB = 4;                /* set by main */
time_t _btime = 0;              /* set by main */
double _exec_start_delta = 0.0; /* computed by main */
ullong _pow10[11] = { 1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10 };   /* int bound of value of N digits */

static WINDOW *_win;		/* initialized by main */

static char _intrnames[100][40];
static char _intrnums[100][40];
static char _disknames[16][16];
static char _netname[16][40];
static char _nnet = 0;

#define lengthof( arr )         ( sizeof(arr) / sizeof(*(arr)) )

double _INTERVAL = 0;
double _LAST_SAMPLE = 0;
int PAGER_COL = 50;
int INTR_COL = 50 + 13;
int DISKS_ROW = 18;
int LINES = 24;
int COLUMNS = 80;

struct winsize _win_sz;
void on_winch( )
{
    char strbuf[40];

    ioctl(1, TIOCGWINSZ, &_win_sz);  // ws_row, ws_col, ws_xpixel, ws_ypixel
    LINES = _win_sz.ws_row;
    COLUMNS = _win_sz.ws_col;

    PAGER_COL = COLUMNS - 30;
    if (PAGER_COL > 60) PAGER_COL = 60;
    INTR_COL = PAGER_COL + 13;
    DISKS_ROW = (LINES >= 28) ? 22 : (LINES >= 26) ? 20 : 18;
}

static int _had_hup = 0;
void on_sighup( )
{
    _had_hup = 1;
}

static int _had_sigterm = 0;
void on_sigterm( )
{
    _had_sigterm = 1;
}

struct system_stats_s {
    struct {
	/* top line */
	int		nusers;
	int		nprocs, ntotalprocs, nlastpid;
	float		loadavg[3];
	float		temps[3];
	time_t		date;
        int             ms;
	/* memory */
// TODO: make meminfo two flat arrays of 4
	ullong          meminfo[2][2][2];
	ullong          memfree;
	ullong          memstats[14];
        /* system hardware */
        struct {
            ulong ncpus;
            ulong ncores;
            ulong nthreads;
            ulong memtotalkb;
            double mhz;
        } sysinfo;
	/* cpu usage */
	unsigned long cputime[7];	/* sys, intr, user, nice, idle, iowait, steal */
	unsigned long pager[2];		/* in, out */
	unsigned long swapper[2];	/* in, out */
	unsigned long faults[2];	/* Linux: minflt majflt */
	unsigned long ctxt[2];		/* csw, trp */
	ullong        totintr;		/* total interrupts since boot */
	ullong        intr[100];	/* interrupts from irq 0 .. 15 */
	unsigned long vm[6];		/* vss rss data stk exe lib */
	unsigned long procs[5];		/* r p d s w */
        /* network activity */
	unsigned long long net[lengthof(_netname)*4];   /* eth0 and ppp0 rx/tx byt/pkt */
        /* disk activity */
	ullong        diskinfo[16][6];	/* seeks, xfers, blks, msps, rblk, wblk */
    }
    counts;

// FIXME: make _systat_0, _systat_1 and _deltas 3 instances of the same struct!
// TODO: rename _systat.counts[0] to _nprevious, _systat.counts[1] to _ncurrent, _systat.deltas to _deltas
// FIXME: to guarantee that array sizes remain in sync
// TODO: read stats into _systat[1] by symbolic name eg VMSTAT_PGPGIN,
//       compute values for display by symbolic name later.
    struct {
	float cputime[7];
	float cpuuse[7];
	ullong newprocs;
	ullong pager[2];
	ullong swapper[2];
	ullong faults[2];
	ullong ctxt[2];
	ullong totintr;
	ullong intr[100];
	ullong net[lengthof(_netname)*4];
	ullong diskinfo[16][6];
	ullong memstats[14];
    }
    deltas;
};
struct system_stats_s _systat[2];


/* current time as a float */
static double fptime( void )
{
    double now;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    now = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
    return now;
}

/* sleep for a floating-point number of seconds */
static void fpsleep( double pause )
{
    struct timeval tv;
    if (pause < 0) return;
    tv.tv_sec = pause;
    tv.tv_usec = (pause - tv.tv_sec) * 1000000;
    usleep(tv.tv_usec);
    sleep(tv.tv_sec);
}

struct scaletab {
    unsigned long long bound;
    double scale;
    const char *fmt;
};
char *iscale( unsigned long long value, int wid, struct scaletab *fmt )
{
    static char bufs[16][100] = { 0 };
    static int bufid = 0;
    static struct scaletab _myfmt[] = {
        // TODO: maybe scale to maintain eg 4 digits precision
	{ 10000, 1, "%*.*lf" },			/* 4567 => 4567 */
	{ 1000000, 1e3, "%*.*lfk" },		/* 45678 => 45.7k */
	{ 1000000000, 1e6, "%*.*lfm" },		/* 4567890 => 4.57m */
	{ 1000000000000, 1e9, "%*.*lfg" },	/* 4567890000 => 4.57g */
	{ 1000000000000000, 1e12, "%*.*lft" },	/* 4567890000000 => 4.57t */
	{ 1000000000000000000, 1e15, "%*.*lfp" },
	{ 0, 0, 0 }
    };
    char valbuf[40];
    char *buf;
    double v;
    int fi;
    /* drop decimals if tight on space */
    int decimals = wid >= 6 ? 2 : wid >= 5 ? 1 : 0;
    int fw;

    /* avoid overwriting same buf so can compose multi-value lines */
    buf = bufs[bufid++];
    if (bufid >= lengthof(bufs)) bufid = 0;

    if (!fmt) fmt = _myfmt;
    fi = 0;

    /* find the appropriate scale to use */
    while (value >= fmt[fi].bound && fmt[fi+1].fmt) {
        fi++;
    }

    /* do not show .00 decimals for integer values */
    v = value / fmt[fi].scale;
    if (v == (long long)v) decimals = 0;

    /* use the full width, but leave room for the units letter */
    fw = wid;
    if (fi > 0) fw -= 1;

    sprintf(buf, fmt[fi].fmt, fw, decimals, value / fmt[fi].scale);

    /* use blanks for zeroes, much easier to scan visually */
    if (value == 0) buf[wid - 1] = ' ';

    return buf;
}

/* render the value val into exactly fieldwidth chars */
char * _showit( int fieldwidth, int frac, ullong val, char *units, ullong bases[] )
{
    static char bufs[16][100] = { 0 };
    static int bufid = 0;

    /* rotate through 16 buffers to allow multiple _showit() in a single printw */
    char *p;
    ullong *b;
    char *buf = bufs[bufid++];
    if (bufid >= lengthof(bufs)) bufid = 0;

    /* zero maps to '    ' blanks */
    if (!val) {
	memset(buf, ' ', fieldwidth);
        buf[fieldwidth] = '\0';
        return buf;
    }

    snprintf(buf, sizeof(*bufs), "%*llu", fieldwidth, val);

    /* try to fit field overflow by changing the units */
    /* if decimals are allowed do not typeset more than 4 digits */
    for (p = units, b = bases; *p && (strlen(buf) > fieldwidth || (frac > 0 && (val / *b >= 10000))); p++, b++) {
        // TODO: test fit numerically not with strlen
        if (frac) {
            snprintf(buf, sizeof(*bufs), "%*.*f%c", fieldwidth-1, frac, (double)(val) / *b, *p);
            if (strlen(buf) > fieldwidth && fieldwidth - frac <= 4) {
                /* if the decimals don't fit, try to omit the decimals */
                /* beware the degenerate case of never fits thus typesets 0E */
                int toomany = strlen(buf) - fieldwidth;
                if (toomany <= frac && (val / *b < 10000 || !*(p+1))) {
                    snprintf(buf, sizeof(*bufs), "%*.*f%c", fieldwidth-1, frac - toomany, (double)(val) / *b, *p);
                }
            }
        } else {
            snprintf(buf, sizeof(*bufs), "%*llu%c", fieldwidth-1, (val + *b/2) / *b, *p);
        }
    }

    /* if still overflow, map to '****' */
    /* sometimes ***** is better than 178M (expecting 178423), and definitely better than 0E */
    if (strlen(buf) > fieldwidth || (buf[0] == '0' && !isdigit(buf[1]) && !buf[2])) {
	memset(buf, '*', fieldwidth);
        buf[0] = ' ';
        buf[fieldwidth] = '\0';
    }

    return buf;
}

/* typeset a value to fit the field width, scale by powers of 1024 */
char * showmem( int fieldwidth, ullong val )
{
    /* memory is reported in units of 1024 kilobytes, so 1 is 1K and 1024 is 1M */
    const ullong K = 1024, M = K*K, G = M*K, T = G*K, P = T*K, E = P*K;
    ullong bases[] = { 1, K, M, G, T, P, E };
    // FIXME: Z, Y not representable in 64 bits
    return _showit(fieldwidth, 2, val, "KMGTPE", bases);
}

/* typeset a float to fit the field width, scale by powers of 1000 */
char * showfnum( int fieldwidth, int decimals, ullong val )
{
    int overflows = fieldwidth-1 < lengthof(_pow10) && val >= _pow10[fieldwidth-1];
    const ullong k = 1000, m = k*k, g = m*k, t = g*k, p = t*k, e = p*k;
    ullong bases[] = { k, m, g, t, p, e };
    // FIXME: z, y not representable in 64 bits
    return _showit(fieldwidth, overflows ? 0 : decimals, val, "kmgtpe", bases);
}

/* typeset an int to fit the field width, scale by powers of 10 */
char * shownum( int fieldwidth, ullong val )
{
    return showfnum(fieldwidth, 0, val);
}

char * showscaledcount( int fieldwidth, ullong val )
{
    if (!val) return "0";
    char *p = shownum(fieldwidth, val);
    while (*p == ' ') p++;
    return p;
}

/* format a floating-point number with the given width and decimals, while handling overflows */
char * showfloat( int fieldwidth, int decimals, double val )
{
    static char bufs[8][100] = { 0 };
    static int bufid = 0;

    char *tmpbuf = bufs[bufid++];
    if (bufid >= lengthof(bufs)) bufid = 0;

    sprintf(tmpbuf, "%*.*f", fieldwidth, decimals, val);
    if (strlen(tmpbuf) > fieldwidth) return showfnum(fieldwidth, 0, val);

    return tmpbuf;
}

/* read nbytes from the file into buf */
int readfile( const char *filename, void *buf, int nbytes )
{
    FILE *fp = fopen(filename, "rb");
    int nb = 0;

    *(char*)buf = '\0';
    if (!fp) return -1;
    nb = (int)fread(buf, 1, nbytes, fp);
    fclose(fp);
    ((char*)buf)[nb-1] = '\0';

    return nb;
}

// TODO: int tailfile( const char *filename, void *buf, int nbytes )

char * startofline( char *buf, char *p ) {
    if (!p) return p;
    while (p > buf && *p != '\n') p--;
    return p > buf ? ++p : p;
}

void finddiskname( char *name, int namelen, int major, int minor )
{
    char buf[10000], *p, *w, *q;
    readfile("/proc/devices", buf, sizeof(buf));
    p = strstr(buf, "Block devices:");
    if (p) {
	while ((w = strsep(&p, "\n")) != NULL) {
	    int n = strtol(w, &w, 10);
	    if (n == major) {
		strncpy(name, w+1, namelen);
		name[namelen - 1] = '\0';
		q = strchr(name, '\n');
		if (q) *q = '\0';
		//sprintf(strchr(name, '\0'), ".%d", minor);
	    }
	}
    }
    else {
	memset(name, ' ', namelen);
    }

    name[namelen] = '\0';
}

int gather_version( )
{
    char buf[200], *p;
    int maj, min, rel;

    readfile("/proc/version", buf, sizeof(buf));
    p = strstr(buf, "Linux version ");

    if (p && sscanf(p+14, "%d.%d.%d", &maj, &min, &rel))
	_linuxver = maj * 1000000 + min * 1000 + rel;
    else
	_linuxver = 1000000;

    return _linuxver;
}

// return the timestamp when the system says it booted
double getbtime( )
{
    unsigned long btime;
    char *p, buf[40000];

    readfile("/proc/stat", buf, sizeof(buf));
    p = strstr(buf, "btime ");
    if (p) btime = strtoul(p+6, NULL, 10);

    return (time_t) btime;
}

// Return the ms offset between exec_start and the actual uptime.
// Call when this program starts running
// Linux can drift, possibly when suspended/hibernated.
unsigned long long get_exec_drift( unsigned long btime )
{
    double now = fptime();
    double delta = 0.0;
    char buf[1000], *p;
    char fname[100];
    snprintf(fname, sizeof(fname), "/proc/%d/sched", getpid());
    if (readfile(fname, buf, sizeof(buf)) > 0) {
        double start;
        p = strstr(buf, "se.exec_start");
        if (p) start = scan_ullong(p+10, ":") / 1000;
        delta = now - (btime + start);
    }
    return delta;
}

unsigned char _pops[4] = {0, 1, 1, 2};
int popcount(unsigned char *bitbuf, int size) {
    int i, count = 0;
    for (i = 0; i < size; i++) {
        int ch = bitbuf[i];
        count += (_pops[ch & 3] + _pops[(ch >> 2) & 3] + _pops[(ch >> 4) & 3] + _pops[(ch >> 6) & 3]);
    }
    return count;
}

int gather_stats(long loop_count)
{
    time_t btime = _btime;
    // 8c/16t /proc/cpuinfo is 24k, size buf
    char *p, *q, buf[100000], fname[80], *nextp, *nextw;
    ullong i, j, n;
    int have_fault_counts = 0;
    double now = fptime();
    char tmpbuf[200];

    _systat[0] = _systat[1];
    memset(&_systat[1], 0, sizeof(_systat[1]));

    _systat[1].counts.date = (time_t) now;
    _systat[1].counts.ms = 1000 * (now - (ullong)now);

    /* syscmdl(NULL, NULL, buf, sizeof(buf),
	       "sensors | grep CPU | grep Temp:"); */
    /* temps[0] = CPU Temp: +25.0 [CF] */

    readfile("/proc/loadavg", buf, sizeof(buf));
    sscanf(buf, "%f %f %f %d/%d %d",
	   &_systat[1].counts.loadavg[0],
	   &_systat[1].counts.loadavg[1],
	   &_systat[1].counts.loadavg[2],
	   &_systat[1].counts.nprocs,
	   &_systat[1].counts.ntotalprocs,
           &_systat[1].counts.nlastpid);

    if (readfile("/proc/cpuinfo", buf, sizeof(buf)) > 0) {
        int ncpus = 0, ncores = 0, nthreads = 0, nmhz = 0, maxmhz = 0, vcores = 0;
        // see /sys/devices/system/cpu/cpu*/topology/thread_siblings_list for pairings 0,8 1,9 etc
        // to disable at runtime: echo 0 > /sys/devices/system/cpu/cpu9/online
        // to disable at boottime: append `noht` kernel flag
// TODO: find a way to distinguish containers from bare metal and report on container limits (eg k8 crdb)

        // TODO: only need to read processor every so often (at start and to catch changes)
        long maxCore = -1, maxCpu = -1;
        unsigned char coremask[MAXCORES/8], cpumask[MAXCORES/8];
        memset(coremask, 0, sizeof(coremask));
        memset(cpumask, 0, sizeof(cpumask));
        for (p = buf; (q = strstr(p, "processor")); p = q + 10) {
            char *line;
            long n;
            double mhz;

            nthreads += 1;

            // cores on each cpu are numbered starting at 0, but not necessarily contiguously (eg Xeon Silver 4214)
            line = strstr(q, "core id");
            if (line && sscanf(line, "core id : %ld", &n) == 1 && n < MAXCORES) coremask[n/8] |= 1 << (n & 7);

            // cpus are numbered starting at 0, unclear whether contiguously
            line = strstr(q, "physical id");
            if (line && sscanf(line, "physical id : %ld", &n) == 1 && n < MAXCORES) {
                cpumask[n/8] |= 1 << (n & 7);
            }

            // the kernel shows the current speed of each cpu, use that to calc the average
            line = strstr(q, "cpu MHz");
            if (line && sscanf(line, "cpu MHz : %lf", &mhz) == 1 && mhz > maxmhz) maxmhz = mhz;
        }
        ncpus = popcount(cpumask, sizeof(cpumask));
        ncores = popcount(coremask, sizeof(coremask));

        // Cores count does not reflect cores added to a VM while running.
        // MHz count in a VM seems to show all physical cores not just virtual.
        _systat[1].counts.sysinfo.ncpus = ncpus;
        // NOTE: we assume all cpus have the same number of cores
        _systat[1].counts.sysinfo.ncores = ncpus * ncores;
        _systat[1].counts.sysinfo.nthreads = nthreads;
        _systat[1].counts.sysinfo.mhz = maxmhz;
    }

    readfile("/proc/interrupts", buf, sizeof(buf));
    _systat[1].counts.totintr = 0;
    nextp = buf;
    p = strsep(&nextp, "\n");
    for (i=0; i<lengthof(_intrnames); i++) {
	char num[100];
        unsigned long int n, m;
        unsigned long count;
        p = strsep(&nextp, "\n");
        if (p && sscanf(p, "%s:", num)) {
            // the colon-terminated tag at the front is the interrupt number (eg 8: or NMI:)
            strncpy(_intrnums[i], num, sizeof(_intrnums[i]-1));
            _intrnums[i][sizeof(_intrnums[i]-1)] = '\0';

            // the double-space delimited string at the end is the interrupt name
            q = strchr(p, '\0');
            while (q[-1] != ' ' || q[-2] != ' ') --q;
            strncpy(_intrnames[i], q, sizeof(_intrnames[i])-1);
            _intrnames[i][sizeof(_intrnames[i])-1] = '\0';
            *q = '\0';

            // total up the numeric columns (per-core interrupt counts) in the middle
            while (*p != ':') p++;
            p++;
            count = 0;
            while (*p == ' ' || *p >= '0' && *p <= '9') {
                while (*p == ' ') p++;
                if (sscanf(p, "%lu", &n)) count += n;
                while (*p >= '0' && *p <= '9') p++;
            }

            _systat[1].counts.intr[i] = count;

            /* collapse many-valued interrupts, eg 8 nvme0q0, q1, q2, ... */
            if (i > 0) {
                char tmpbuf[2000], yeslong;
                tmpbuf[0] = '\0';
                int nchars = 5;
                yeslong = strcmp(_intrnames[i], "xhci_hcd") == 0 && (nchars = 8) ||
                    strcmp(_intrnames[i], "ioat-msix") == 0 && (nchars = 9) ||
                    sscanf(_intrnames[i], "nvme%ldq%ld", &n, &m) == 2 && (nchars = 5) ||
                    sscanf(_intrnames[i], "megasas%ld-msix%ld", &n, &m) == 2 && (nchars = 7) ||
                    sscanf(_intrnames[i], "eno%ld-TxRx-%ld", &n, &m) == 2 && (nchars = 4) ||
                    sscanf(_intrnames[i], "ens%ld-Tx-Rx-%ld", &n, &m) == 2 && (nchars = 4) ||
                    sscanf(_intrnames[i], "virtio%ld", &n) == 1 && (nchars = 7) ||
                    sscanf(_intrnames[i], "eth%ld-TxRx-%ld", &n, &m) == 2 && (nchars = 9) ||
                    sscanf(_intrnames[i], "mpt3sas%ld-msix%ld", &n, &m) == 1 && (nchars = 8);
                if (yeslong && strncmp(_intrnames[i], _intrnames[i-1], nchars) == 0) {
                    if (count > 0) {
                        snprintf(tmpbuf, sizeof(tmpbuf), "%s%s", _intrnums[i-1], num);
                        snprintf(_intrnums[i-1], sizeof(*_intrnums), "%s", tmpbuf);
                        _systat[1].counts.intr[i-1] += count;
                    }
                    i--;
                }
            }
        }
	// compute totintr from the partials, the count in /proc/stat is no longer reliable
	_systat[1].counts.totintr += _systat[1].counts.intr[i];
    }

    /* Mem: usage */
    if (readfile("/proc/meminfo", buf, sizeof(buf)) > 0) {
        p = strstr(buf, "MemTotal:");
        if (p) _systat[1].counts.sysinfo.memtotalkb = strtoul(p+9, NULL, 10);

        /* Active = Active(anon) + Active(file), ie data pages + file pages */
        p = strstr(buf, "Active:");
        //if (p) _systat[1].counts.meminfo[0][0][0] = strtoul(p+7, NULL, 10);
        if (p) _systat[1].counts.memstats[3] = strtoul(p+7, NULL, 10);
        // total up active rss instead
        //if (p) _systat[1].counts.meminfo[0][0][0] = strtoul(p+7, NULL, 10);
        p = strstr(buf, "Inactive:");
        if (p) _systat[1].counts.memstats[4] = strtoul(p+9, NULL, 10);
        p = strstr(buf, "MemFree:");
        if (p) sscanf(p+8, "%llu", &_systat[1].counts.memfree);
        if (p) _systat[1].counts.memstats[6] = strtoul(p+8, NULL, 10);
        //p = strstr(buf, "MemShared:");
        //if (p) sscanf(p+10, "%lu", &_systat[1].counts.meminfo[1][0][1]);
        p = strstr(buf, "Shmem:");
        if (p) _systat[1].counts.meminfo[1][0][1] = strtoul(p+6, NULL, 10);
        // maybe also count ShmemHugePages, ShmemPmdMapped
        // note: not MemTotal, systat reports on mem held by processes, not total in system
        //p = strstr(buf, "MemTotal:");
        //if (p) _systat[1].counts.meminfo[1][0][0] = strtoul(p+10, NULL, 10);

        /* swap activity */
        p = strstr(buf, "SwapTotal:");
        /* increasing swap used indicates data swapped out */
        if (p) _systat[1].counts.swapper[1] = strtoul(p+10, NULL, 10);
        /* increasing free swap indicates data swapped in */
        p = strstr(buf, "SwapFree:");
        if (p) _systat[1].counts.swapper[0] = strtoul(p+9, NULL, 10);

        /* other memory stats */
        p = strstr(buf, "Cached:");
        if (p) _systat[1].counts.memstats[5] = strtoul(p+7, NULL, 10);
        p = strstr(buf, "SwapCached:");
        if (p) _systat[1].counts.memstats[5] += strtoul(p+11, NULL, 10);
        p = strstr(buf, "Buffers:");
        if (p) _systat[1].counts.memstats[13] = strtoul(p+8, NULL, 10);
        p = strstr(buf, "Unevictable:");
        if (p) _systat[1].counts.memstats[2] = strtoul(p+12, NULL, 10);
    }

    /* NOTE: vmstat:nr_shmem is in pages, meminfo:Shmem in kilobytes! (4x more) */
    if (readfile("/proc/vmstat", buf, sizeof(buf)) > 0) {
        // FIXME: units of pages or KB? linux 4.9.0 shows pgpgin in KB (not pages)

	/* total paged in and out */
	p = strstr(buf, "pgpgin");
	if (p) _systat[1].counts.pager[0] = strtoul(p+6, NULL, 10);
	p = strstr(buf, "pgpgout");
	if (p) _systat[1].counts.pager[1] = strtoul(p+7, NULL, 10);
        if (_linuxver < 3000000) {
// FIXME: were the pgpgin counts ever in units of pages?  Or always KB?
            _systat[1].counts.pager[0] *= _pageKB;
            _systat[1].counts.pager[1] *= _pageKB;
        }

	/* swapped in and out */
	p = strstr(buf, "pswpin");
	if (p) _systat[1].counts.swapper[0] = strtoul(p+6, NULL, 10);
	p = strstr(buf, "pswpout");
	if (p) _systat[1].counts.swapper[1] = strtoul(p+7, NULL, 10);
// FIXME: pgpgin is in KB, *not* pages -- maybe so are the others?
        if (_linuxver < 3000000) {
            _systat[1].counts.swapper[0] *= _pageKB;
            _systat[1].counts.swapper[1] *= _pageKB;
        }

	/* additional memstats not available from /proc/meminfo  */
	p = strstr(buf, "pgactivate");
	if (p) _systat[1].counts.memstats[9] = strtoul(p+10, NULL, 10);
	p = strstr(buf, "pgalloc_normal");      // allocations, count as zfod
	if (p) _systat[1].counts.memstats[1] = strtoul(p+14, NULL, 10);
	p = strstr(buf, "pgfree");              // prcfr
	if (p) _systat[1].counts.memstats[8] = strtoull(p+7, NULL, 10);
        // ??? TODO: is refault == page reactivations?
	//p = strstr(buf, "workingset_refault");  // react
        //if (p) _systat[1].counts.memstats[9] = strtoull(p+18, NULL, 10);
        // FIXME: units of pages or KB?

        /* duplicates of meminfo */
        // nr_free_pages
        // nr_inactive_anon, nr_active_anon, nr_inactive_file, nr_active_file
        // nr_anon_pages, nr_mapped, nr_file_pages
        // nr_dirty, nr_writeback, nr_dirtied, nr_written
        // nr_pages_scanned

	/* page fault stats from /proc/vmstat if available */
	p = strstr(buf, "pgfault");
	if (p) _systat[1].counts.faults[0] = strtoul(p+7, NULL, 10);
	p = strstr(buf, "pgmajorfault");
	if (p) _systat[1].counts.faults[1] = strtoul(p+13, NULL, 10);
	if (p) have_fault_counts = 1;
        // FIXME: units of pages or KB?

        /* pgpgs and maybe pdwak */
        p = strstr(buf, "pgsteal_kswapd ");      // daefr
        if (p) _systat[1].counts.memstats[7] = strtoul(p+15, NULL, 10);
        p = strstr(buf, "pgsteal_direct ");
        if (p) _systat[1].counts.memstats[7] += strtoul(p+15, NULL, 10);
        p = strstr(buf, "pgscan_kswapd ");       // pdpgs
        if (p) _systat[1].counts.memstats[11] = strtoul(p+14, NULL, 10);
        p = strstr(buf, "pgscan_direct ");
        if (p) _systat[1].counts.memstats[11] += strtoul(p+14, NULL, 10);
        // FIXME: units of pages or KB?
    }

    readfile("/proc/stat", buf, sizeof(buf));
    p = strstr(buf, "cpu ");
// FIXME: p == NULL
    if (_linuxver >= 2006011) {
        // have available iowait, steal
        // skip irq, softirq (derived below)
        // The counts are kernel jiffies spent in each state since boot, always increasing.
        // The counts should always total seconds_since_boot * HZ (jiffies per second)
        sscanf(p, " cpu %lu %lu %lu %lu %lu %*u %*u %lu",
               &_systat[1].counts.cputime[2],	// user
               &_systat[1].counts.cputime[3],	// nice
               &_systat[1].counts.cputime[0],	// system
               &_systat[1].counts.cputime[4],	// idle
               &_systat[1].counts.cputime[5],	// iowait
               &_systat[1].counts.cputime[6]	// steal
                                                // TODO: guest
                                                // TODO: guest-nice
        );
    }
    else {
        sscanf(p, " cpu %lu %lu %lu %lu",
               &_systat[1].counts.cputime[2],
               &_systat[1].counts.cputime[3],
               &_systat[1].counts.cputime[0],
               &_systat[1].counts.cputime[4]);
    }
    if (_linuxver < 2006000) {
	p = strstr(buf, "\npage ");
	if (p)
	sscanf(p, " page %lu %lu",
	       &_systat[1].counts.pager[0],
	       &_systat[1].counts.pager[1]);
	p = strstr(buf, "\nswap ");
	if (p)
	sscanf(p, " swap %lu %lu",
	       &_systat[1].counts.swapper[0],
	       &_systat[1].counts.swapper[1]);
    }

    // note: as of the 4.x kernels (4.14?), /proc/stat no longer maintains accurate intr counts
    // see /proc/interrupts instead, which keeps per-core counts
    p = strstr(buf, "\nintr ");
    // Even the totintr is does not match a hand-total (totintr is too low)
    //sscanf(p, " intr %llu", &_systat[1].counts.totintr);
/**
    // TODO: use /proc/stat as an alternate to /proc/interrupts (for older kernels)
    sscanf(p, " intr %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
	   &_systat[1].counts.totintr,
	   &_systat[1].counts.intr[0],
	   &_systat[1].counts.intr[1],
	   &_systat[1].counts.intr[2],
	   &_systat[1].counts.intr[3],
	   &_systat[1].counts.intr[4],
	   &_systat[1].counts.intr[5],
	   &_systat[1].counts.intr[6],
	   &_systat[1].counts.intr[7],
	   &_systat[1].counts.intr[8],
	   &_systat[1].counts.intr[9],
	   &_systat[1].counts.intr[10],
	   &_systat[1].counts.intr[11],
	   &_systat[1].counts.intr[12],
	   &_systat[1].counts.intr[13],
	   &_systat[1].counts.intr[14],
	   &_systat[1].counts.intr[15],
	   &_systat[1].counts.intr[16],
	   &_systat[1].counts.intr[17],
	   &_systat[1].counts.intr[18],
	   &_systat[1].counts.intr[19],
	   &_systat[1].counts.intr[20],
	   &_systat[1].counts.intr[21],
	   &_systat[1].counts.intr[22],
	   &_systat[1].counts.intr[23]);
**/
    p = strstr(buf, "\nctxt ");
    sscanf(p, " ctxt %lu",
	   &_systat[1].counts.ctxt[0]);

    /* if /proc/diskstats exists (2.5.69 and up), use that --
     * better format, too */
    if (readfile("/proc/diskstats", buf, sizeof(buf)) > 0) {
	int ndevs = 0;
	static char *devnames[] = {
	    /* assume first scsi disk is the primary (system) drive */
            /* This ordering will determine the device display order */
            " nvme0n1 ", " nvme0n2 ", " nvme0n3 ", " nvme0n4 ",
            " nvme1n1 ", " nvme1n2 ", " nvme1n3 ", " nvme1n4 ",
            " nvme2n1 ", " nvme2n2 ", " nvme2n3 ", " nvme2n4 ",
            " nvme3n1 ", " nvme3n2 ", " nvme3n3 ", " nvme3n4 ",
	    " hda ", " hdb ", " hdc ", " hdd ", " hde ", " hdf ",
	    " vda ", " vdb ", " vdc ", " vdd ", " vde ", " vdf ",
	    " sda ", " sdb ", " sdc ", " sdd ", " sde ", " sdf ",
            " md0 ", " md1 ", " md2 ", " md3 ",
            " xvda ", " xvdh ",
            " dm-0 ", " dm-1 ", " dm-2 ", " dm-3 ",
	    " sr0 ", " sr1 ", " sr2 ", " sr3 ",
	    " sdg ", " sdh ", " sdi ", " sdj ", " sdk ", " sdl ",
	    " sr4 ", " sr5 ", " sr6 ", " sr7 ",
	    NULL };
        static int devnamelengths[lengthof(devnames)] = { 0 };
        for (i=0; devnames[i]; i++) devnamelengths[i] = strlen(devnames[i]);
	/* major, minor, name, ... */
	/*
	    3    0 hda 228 87 21562 1224 4 0 48 56 0 996 1280
	    3    1 hda1 290 21474 1 8
	    9    0 md0 9 0 66 0 1 0 8 0 0 0 0
	    // ...
	 */
	/* layout:
	    nreads, nreads_merged, nsect_read, ms_read,
	    nwrites, ??nwrites_merged??, nsect_written, ms_write,
	    num_ios_in_progress, num_ms_for_ios, total_ms_for_io
	 */
	double interval = now - _LAST_SAMPLE;
	for (i=0; devnames[i] && ndevs<lengthof(_systat[1].counts.diskinfo); i++) {
	    p = strstr(buf, devnames[i]);
	    if (p) {
                int devmajor, devminor;
		unsigned long long nmrd, rd_mrg, rd_sec, ms_rd;
		unsigned long long nmwr, wr_mrg, wr_sec, ms_wr;
                unsigned long long io_cnt, io_ms, io_totms;

                /* back up to the start of the line */
                p = startofline(buf, p);

                /* /usr/src/linux-source-4.3/Documentation/iostats.txt, kernels 2.4 and up */
                sscanf(p, " %d %d %s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                       &devmajor, &devminor, tmpbuf,
		       &nmrd, &rd_mrg, &rd_sec, &ms_rd,
                       &nmwr, &wr_mrg, &wr_sec, &ms_wr,
                       &io_cnt, &io_ms, &io_totms);

//		sscanf(p+devnamelengths[i], "%llu %llu %llu %llu"       // reads
//			    "%llu %llu %llu %llu"                       // writes
//			    "%llu %llu %llu",                           // ios
//		       &nmrd, &rd_mrg, &rd_sec, &ms_rd,
//		       &nmwr, &wr_mrg, &wr_sec, &ms_wr,
//		       &io_cnt, &io_ms, &io_totms);
// AR: this next one looks wrong... skips first? nmrd??
//		sscanf(p+devnamelengths[i], "%*u, %llu, %llu", &nmrd, &rd_sec);

		_systat[1].counts.diskinfo[ndevs][0] = nmrd + nmwr + rd_mrg + wr_mrg;   /* num reads+writes, proxy for tps */
		_systat[1].counts.diskinfo[ndevs][1] = nmrd + nmwr;                     /* xfers (after r/w merges) */
		_systat[1].counts.diskinfo[ndevs][2] = rd_sec/2 + wr_sec/2;             /* blks */
		// _systat[1].counts.diskinfo[ndevs][3] = (ullong)((ms_rd + ms_wr) / (interval * 1000) * 100);  /* msps, but track busy% */
                _systat[1].counts.diskinfo[ndevs][3] = (ullong)(io_ms);
                _systat[1].counts.diskinfo[ndevs][4] = rd_sec/2;
                _systat[1].counts.diskinfo[ndevs][5] = wr_sec/2;
		strncpy(_disknames[ndevs], devnames[i]+1, devnamelengths[i]-2);
// TODO: track and display kb in / out separately (not just total blks)
		ndevs += 1;
                if (ndevs >= lengthof(_systat[1].counts.diskinfo)) break;
	    }
	}
    }
    else if ((p = strstr(buf, "\ndisk_io:")) != NULL) {
	int maj, min, i, n;
	p += 9;
	for (i=0; sscanf(p, " (%d,%d):%n", &maj, &min, &n) == 2; i++) {
	    int a, b, c, d, e;
	    p += n;
	    if (i < 16) {
		finddiskname(_disknames[i], sizeof(_disknames[i]), maj, min);
		sscanf(p, "(%d,%d,%d,%d,%d)%n", &a, &b, &c, &d, &e, &n);
		_systat[1].counts.diskinfo[i][0] = a;           // seeks
		_systat[1].counts.diskinfo[i][1] = b + d;       // xfers
		_systat[1].counts.diskinfo[i][2] = c + e;       // blks
		_systat[1].counts.diskinfo[i][4] = c;           // rblk
		_systat[1].counts.diskinfo[i][5] = e;           // wblk
	    }
	    p += n;
	}
    }
    else {
	strncpy(_disknames[0], "hdd", sizeof(_disknames[0])-1);
	p = strstr(buf, "disk_rio");
	if (p) _systat[1].counts.diskinfo[0][1] = strtol(p+8, NULL, 10);
	p = strstr(buf, "disk_wio");
	if (p) _systat[1].counts.diskinfo[0][1] += strtol(p+8, NULL, 10);
	p = strstr(buf, "disk_rblk");
	if (p) _systat[1].counts.diskinfo[0][2] = strtol(p+9, NULL, 10);
	if (p) _systat[1].counts.diskinfo[0][4] = strtol(p+9, NULL, 10);
	p = strstr(buf, "disk_wblk");
	if (p) _systat[1].counts.diskinfo[0][2] += strtol(p+9, NULL, 10);
	if (p) _systat[1].counts.diskinfo[0][5]  = strtol(p+9, NULL, 10);
    }

    /* patch up disknames to make them more legible: abbreviate nvme0n1 to nvme0 */
    for (i=0; i<sizeof(_disknames); i++) {
        int n, m;
        if (sscanf(_disknames[i], "nvme%dn%d", &n, &m) == 2 && m == 1) {
            sprintf(_disknames[i], "nvme%d", n /*, m*/);
        }
    }

    /* network devices */
    readfile("/proc/net/dev", buf, sizeof(buf));
    for (_nnet = 0, p = buf; (p = strchr(p, ':')) && _nnet < lengthof(_netname); _nnet += 1) {
	/* back up to the beginning of the line */
	while (p[-1] != '\n')
	    --p;

	/* copy name from leftmost edge */
	for (i=0; _netname[_nnet][i] = p[i]; i++)
	    if (p[i] == ':') break;
	_netname[_nnet][i] = '\0';

	//if (strcmp(_netname[_nnet], "lo") == 0) continue;
	p = strchr(p, ':') + 1;
	sscanf(p, " %llu %llu %*d %*d %*d %*d %*d %*d %llu %llu",
	       &_systat[1].counts.net[_nnet*4 + 0],
	       &_systat[1].counts.net[_nnet*4 + 1],
	       &_systat[1].counts.net[_nnet*4 + 2],
	       &_systat[1].counts.net[_nnet*4 + 3]);
    }

    /* processes */
    {
	DIR *dp = opendir("/proc");
	struct dirent *de;
	while ((de = readdir(dp)) != NULL) {
	    char *s, *p;
	    int e, i;
            double now;
	    if (strchr("0123456789", de->d_name[0])) {
		char fname[200];
		ullong n, nn[10];
		unsigned long pgsz = _pageKB;
                double last_active;
                int is_active = 0;

                /* /proc/%d/sched contains "se.exec_start   :   12345678.012"
                 * which is updated with the last time the proc ran, in float milliseconds since boot.
                 * Use that info to distinguish "active" (ran in last 20 sec) from inactive */
                sprintf(fname, "/proc/%s/sched", de->d_name);
                e = readfile(fname, buf, sizeof(buf));
                if (e > 0) {
                    p = strstr(buf, "se.exec_start");
                    now = fptime();
                    n = scan_ullong(p+10, ":");
                    // FreeBSD defines "active" as having run in the last 20 seconds.
                    last_active = btime + _exec_start_delta + n/1000;
                    is_active = last_active >= now - 20;
                }
                /* else if (e == 0) then file is empty, process might have exited? */

		n = 0;
		memset(nn, 0, sizeof(nn));

		/* sum up txt,dta/stk?,lib using getpagesize() */
		sprintf(fname, "/proc/%s/stat", de->d_name);
		e = readfile(fname, buf, sizeof(buf));
		if (e <= 0) continue;

		/* pid=1, minflt=10, majflt=12, priority=18, nice=19,
		 * vsize=23, rss=24, nswap=36 (not maintained)
		 * NB: rss not correct!, statm fields not correct!
		 * ...however, status fields seem ok... */

		/* sum up vss, rss, minor faults, major faults, run-state */
		/* swapped out if rss == 0 and vmsz > 0 (kernel threads are all rss == 0 and vmsz == 0) */
		s = buf;
		/* skip to the third word in the status line, RSDZT status */
		for (i=0; i<3; i++)
		    p = strsep(&s, " \t");
                switch (*p) {
                // FIXME: /proc/*/status shows better IRST status?
		case 'R':
		    /* currently running */
		    _systat[1].counts.procs[0] += 1;    // r running
                    break;;;
		case 'S':
		    /* sleeping interruptible */
		    _systat[1].counts.procs[3] += 1;    // s sleeping
                    break;
		case 'D':
		    /* sleeping uninterruptabe */
		    _systat[1].counts.procs[2] += 1;    // d iowait (disk wait)
                    break;
		case 'T':
                case 't':
		    /* traced or stopped on signal */
		    _systat[1].counts.procs[3] += 1;    // d stopped (but not iowait)
                    break;
                // note: "// p page wait" is not tracked separately by linux
                }

		/* skip to the 10th word, minflt */
		if (!have_fault_counts) {
		    for (i=3; i<10; i++)
			p = strsep(&s, " \t");
		    _systat[1].counts.faults[0] += atol(p);	/* minflt */
		    p = strsep(&s, " \t");
		    p = strsep(&s, " \t");
		    _systat[1].counts.faults[1] += atol(p);	/* majflt */
		    /* skip to the 23rd and 24th words, vmsize and rss */
		    for (i=10; i<23; i++)
			p = strsep(&s, " \t");
		}

		sprintf(fname, "/proc/%s/statm", de->d_name);
		e = readfile(fname, buf, sizeof(buf));
                if (e > 0) {
                    // size, resident, shared(file), text, lib, data+stack, dirty
                    sscanf(buf, "%llu %llu %llu %llu %llu %llu %llu",
                           nn+0, nn+1, nn+2, nn+3, nn+4, nn+5, nn+6);
                    if (is_active) {
                        _systat[1].counts.meminfo[0][0][0] += nn[1] * pgsz; /* rss */
                        _systat[1].counts.meminfo[0][1][0] += nn[0] * pgsz; /* vmsz */
                        _systat[1].counts.meminfo[0][1][1] += nn[2] * pgsz; /* shrd, ie backed by a file */
                    }
                    _systat[1].counts.meminfo[1][0][0] += nn[1] * pgsz; /* rss */
                    _systat[1].counts.meminfo[1][1][0] += nn[0] * pgsz; /* vmsz */
                    _systat[1].counts.meminfo[1][1][1] += nn[2] * pgsz; /* shrd */
                    // kernel processes read "0 0 0 0 0 0 0"
                    if (nn[1] == 0 && nn[0] > 0) {
                        /* rss = 0 but vmsz > 0: process is swapped out */
                        _systat[1].counts.procs[3] -= 1;	/* -S sleeping */
                        _systat[1].counts.procs[4] += 1;	/* +W swapped out */
                    }
                }
                /* else process may have disappered? */
#if 0
		/* sum up memory use by all processes: */
		sprintf(fname, "/proc/%s/status", de->d_name);
		readfile(fname, buf, sizeof(buf));
		p = strstr(buf, "VmSize:");
		if (p) _systat[1].counts.meminfo[1][1][0] += strtoul(p+7, NULL, 10);
		p = strstr(buf, "VmRSS:");
		if (p) _systat[1].counts.meminfo[1][0][0] += strtoul(p+6, NULL, 10);
		p = strstr(buf, "VmExe:");
		if (p) _systat[1].counts.meminfo[1][1][1] += strtoul(p+6, NULL, 10);
		// not strictly BSD, but shared libs are also shared
		p = strstr(buf, "VmLib:");
		if (p) _systat[1].counts.meminfo[1][1][1] += strtoul(p+6, NULL, 10);
#endif
	    }
	}
	closedir(dp);
    }
    

    /* count the number of users logged in (w/ entries in utmp) */
    {
	struct utmp *entry;
	int nusers = 0;

	setutent();	/* rewind */
	while ((entry = getutent())  != NULL) {
	    if (entry->ut_type == USER_PROCESS)
		nusers += 1;
	}
	endutent();

	_systat[1].counts.nusers = nusers;
    }
#if 0
    sprintf(fname, "/tmp/%d.out", getpid());
    sprintf(buf, "who | wc -l > %s", fname);
    system(buf);
    readfile(fname, buf, sizeof(buf));
    remove(fname);
    sscanf(buf, "%d", &_systat[1].counts.nusers);
#endif

    return 0;
}

void delta_stats( )
{
    double t;
    int i, j;
    
    /* convert cpu time used (in jiffies) into percent (measured in ticks @HZ) */
    /* The times are for all cpu cores total, we do not read the per-core stats */
    t = 0;
    double interval = fptime() - _LAST_SAMPLE;
    for (i=0; i<7; i++) {
        /* v = (jiffies counted / jiffies per sampling interval) * 100% */
        double njiffies = (_systat[1].counts.cputime[i] - _systat[0].counts.cputime[i]);
        double maxjiffies = HZ * interval;  /* jiffies/second * seconds/interval */
        double v = njiffies / maxjiffies * 100.0;
	_systat[0].deltas.cpuuse[i] = v;
	t += v;
    }
    /* Normalize the times to 0..100% of the total available time in this interval.
     * (also account for polling period flutter, multi-core, or job suspended) */
    /* We keep 0-100% normalized times in cputime[], and summed % time in cpuuse[] */
    double s = 100.0 / t;
    for (i=0; i<7; i++) {
        _systat[0].deltas.cputime[i] = _systat[0].deltas.cpuuse[i] * s;
    }

    _systat[0].deltas.newprocs = _systat[1].counts.nlastpid -
	_systat[0].counts.nlastpid;

    _systat[0].deltas.totintr =
	_systat[1].counts.totintr - _systat[0].counts.totintr;
    for (i=0; i<lengthof(_systat[0].deltas.intr); i++)
	_systat[0].deltas.intr[i] =
	    _systat[1].counts.intr[i] - _systat[0].counts.intr[i];

    for (i=0; i<2; i++) {
	_systat[0].deltas.pager[i] = _systat[1].counts.pager[i] -
				     _systat[0].counts.pager[i];
	if (_systat[1].counts.swapper[i] > _systat[0].counts.swapper[i])
	    _systat[0].deltas.swapper[i] = _systat[1].counts.swapper[i] -
					   _systat[0].counts.swapper[i];
	else
	    /* negative (decreasing) count, display as "0" */
	    _systat[0].deltas.swapper[i] = 0;
	_systat[0].deltas.faults[i] = _systat[1].counts.faults[i] -
				      _systat[0].counts.faults[i];
	_systat[0].deltas.ctxt[i] = _systat[1].counts.ctxt[i] -
				    _systat[0].counts.ctxt[i];
    }

    for (i=0; i<lengthof(_systat[1].counts.diskinfo); i++) {
	for (j=0; j<6; j++)
	    _systat[0].deltas.diskinfo[i][j] =
		_systat[1].counts.diskinfo[i][j] -
		_systat[0].counts.diskinfo[i][j];
    }

    for (i=0; i<14; i++) {
	_systat[0].deltas.memstats[i] = _systat[1].counts.memstats[i] - _systat[0].counts.memstats[i];
    }

    for (i=0; i<lengthof(_systat[0].deltas.net); i++)
    	_systat[0].deltas.net[i] = _systat[1].counts.net[i] - _systat[0].counts.net[i];

    _systat[0].counts = _systat[1].counts;
    //memcpy(&_systat[0].counts, &_systat[1].counts, sizeof(_systat[0].counts));
}

/* display the accumulated info */
int show_stats( )
{
    char buf[200], tmpbuf[200];
    int i, j, r, n, rowi, coli;
    double scale, fr;
    char *p, *fmt;

    mvprintw(0, 0, VERSION);
    mvprintw(0, 12, "%d users", _systat[0].counts.nusers);
    mvprintw(0, 23, "Load %5.2f %5.2f %5.2f   ",
	   _systat[0].counts.loadavg[0],
	   _systat[0].counts.loadavg[1],
	   _systat[0].counts.loadavg[2]);
    mvprintw(1, 12, "Mark");

    // TEST:
    r = DISKS_ROW - 1;
    mvprintw(1, 12, "Cpus %lup/%luc/%lut  %5.3f GHz",
        _systat[0].counts.sysinfo.ncpus,
        _systat[0].counts.sysinfo.ncores,
        _systat[0].counts.sysinfo.nthreads,
        _systat[0].counts.sysinfo.mhz / 1000);

    mvprintw(0, PAGER_COL+5, "%.19s.%03d", ctime(&_systat[0].counts.date), _systat[0].counts.ms);

    move(3,0);
    printw("Mem %7s   Share   VM Tot    Share    Free", showmem(7, _systat[0].counts.sysinfo.memtotalkb));
    // printw("Tot %7s   Share   VM Tot    Share    Free", _systat[0].counts.sysinfo.memtotalkb);
    move(4,0);
    printw("Act %7s %7s  %7s  %7s %7s  Mem",
	   showmem(6, _systat[0].counts.meminfo[0][0][0]),
	   showmem(6, _systat[0].counts.meminfo[0][0][1]),
	   showmem(6, _systat[0].counts.meminfo[0][1][0]),
	   showmem(6, _systat[0].counts.meminfo[0][1][1]),
	   showmem(6, _systat[0].counts.memfree));
    move(5,0);
    printw("All %7s %7s  %7s  %7s          in kb",
	   showmem(7, _systat[0].counts.meminfo[1][0][0]),
	   showmem(7, _systat[0].counts.meminfo[1][0][1]),
	   showmem(7, _systat[0].counts.meminfo[1][1][0]),
	   showmem(7, _systat[0].counts.meminfo[1][1][1]));

    /* process stats */
    // was: mvprintw(7, 0, "Proc:r  p  d  s  w   Csw  Trp   Sys  Int  Sof  Flt");
    mvprintw(7, 0, "Proc:r   p   d   s   w  Csw  Trp  Int  Sof  Flt");
    mvprintw(8, 2, "%s", shownum(4, _systat[0].counts.procs[0] - 1));   // do not count ourselves as a running process
    mvprintw(8, 6, "%s", shownum(4, _systat[0].counts.procs[1]));
    mvprintw(8, 10, "%s", shownum(4, _systat[0].counts.procs[2]));
    mvprintw(8, 14, "%s", shownum(4, _systat[0].counts.procs[3]));
    mvprintw(8, 18, "%s", shownum(4, _systat[0].counts.procs[4]));
    mvprintw(8, 22, "%5s", shownum(4, _systat[0].deltas.ctxt[0]));
    mvprintw(8, 28, "%s", shownum(4, 0));	// traps, overwritten with New
    //mvprintw(8, 30, "%s", shownum(5, 0));	// syscalls, omit (not tracked)
    mvprintw(8, 33, "%s", shownum(4, _systat[0].deltas.totintr));
    mvprintw(8, 38, "%s", shownum(4, _systat[0].deltas.faults[0]));
    mvprintw(8, 43, "%s", shownum(4, _systat[0].deltas.faults[1]));
    // overwrite Trp with New
    mvprintw(7, 29, "New");
    mvprintw(8, 27, "%s", shownum(5, _systat[0].deltas.newprocs));

    r = 6;  // mem stats row
    mvprintw(r++, PAGER_COL, "%s cow", shownum(7, _systat[0].counts.memstats[0]));
    mvprintw(r++, PAGER_COL, "%s zfod", shownum(7, _systat[0].deltas.memstats[1]));
    mvprintw(r++, PAGER_COL, "%s daefr", shownum(7, _systat[0].deltas.memstats[7]));
    mvprintw(r++, PAGER_COL, "%s prcfr", shownum(7, _systat[0].deltas.memstats[8]));
    mvprintw(r++, PAGER_COL, "%s react", shownum(7, _systat[0].deltas.memstats[9]));
    mvprintw(r++, PAGER_COL, "%7s pdwak", shownum(7, _systat[0].deltas.memstats[10]));
    mvprintw(r++, PAGER_COL, "%s pdpgs", shownum(7, _systat[0].deltas.memstats[11]));
    mvprintw(r++, PAGER_COL, "%s intrn", shownum(7, _systat[0].deltas.memstats[12]));
    mvprintw(r++, PAGER_COL-1, "%s wire", shownum(8, _systat[0].counts.memstats[2]));
    mvprintw(r++, PAGER_COL-1, "%s act", shownum(8, _systat[0].counts.memstats[3]));
    mvprintw(r++, PAGER_COL-1, "%s inact", shownum(8, _systat[0].counts.memstats[4]));
    mvprintw(r++, PAGER_COL-1, "%s cache", shownum(8, _systat[0].counts.memstats[5]));
    mvprintw(r++, PAGER_COL-1, "%s free", shownum(8, _systat[0].counts.memstats[6]));
    mvprintw(r++, PAGER_COL-1, "%s buf", shownum(8, _systat[0].counts.memstats[13]));

    fmt = "%6s%%s%6s%%q%6s%%u%6s%%n%6s%%i%6s%%w";
    mvprintw(10, 0, fmt,
	    showfloat(6, 1, _systat[0].deltas.cpuuse[0]),	/* system */
	    showfloat(6, 1, _systat[0].deltas.cpuuse[1]),	/* interrupt */
	    showfloat(6, 1, _systat[0].deltas.cpuuse[2]),	/* user not including nice */
	    showfloat(6, 1, _systat[0].deltas.cpuuse[3]),	/* nice */
	    showfloat(6, 1, _systat[0].deltas.cputime[4]),	/* idle -- as percent of total system time */
	    showfloat(6, 1, _systat[0].deltas.cpuuse[5]));	/* iowait */
                                                /* TODO: guest, guest-nice */

    if (PAGER_COL >= 56) {
        mvprintw(10, 6*8, "%6s%%t", showfloat(6, 1, _systat[0].deltas.cputime[6]));	/* steal */
        /* TODO: guest, guest-nice */
    }

    scale = (PAGER_COL < 60) ? 0.5 : 0.6;

    /* TODO: compose the graph into a char buffer, and show the string */

    rowi = 11;
    mvprintw(rowi, 0, "|      ");
    for (i=1; i<=10; i++) mvprintw(rowi, 10*scale*i, "|      ");
    mvprintw(rowi, 10*scale*10, "|");

    /* pre-clear the graph line; doing it the other way leaves junk? */
    rowi = 12;
    for (i=0; i<10; i++) mvprintw(rowi, 10*scale*i, "      ");

    /* draw the graph to cover both endpoints 0% and 100%, ie 51 chars */
    move(12,0);
    fr = i = 0;
    fr += scale * _systat[0].deltas.cputime[0];
    for (; (i+.5)<fr; i++) addch('=');		/* kernel (system) */
    fr += scale * _systat[0].deltas.cputime[1];
    for (; (i+.5)<fr; i++) addch('+');		/* interrupt handling */
    fr += scale * _systat[0].deltas.cputime[2];
    for (; (i+.5)<fr; i++) addch('>');		/* user */
    fr += scale * _systat[0].deltas.cputime[3];
    for (; (i+.5)<fr; i++) addch('-');		/* nice (lowered prio, >0) */
    fr += scale * _systat[0].deltas.cputime[5];
    for (; (i+.5)<fr; i++) addch('.');		/* iowait */
    fr += scale * _systat[0].deltas.cputime[6];
    for (; (i+.5)<fr; i++) addch('$');		/* cpu steal */
    fr += scale * _systat[0].deltas.cputime[4];
    for (; (i+.5)<fr; i++) addch(' ');		/* idle */

    // Interrupts
    mvprintw(6, INTR_COL, " Interrupts");
    mvprintw(7, INTR_COL, "%5s total", shownum(4, _systat[0].deltas.totintr));
    // r = interrupts row
    for (r=8, i=0; i<lengthof(_systat[1].counts.intr); i++) {
	/* map interrupt to device in /proc/interrupts (2.4 series) */
	if (_systat[1].counts.intr[i] && r<LINES)
        {
	    strncpy(buf, _intrnames[i], sizeof(buf) - 1);
	    if (COLUMNS - INTR_COL - 11 < sizeof(buf)) buf[COLUMNS - INTR_COL - 11] = '\0';
            else buf[sizeof(buf) - 1] = '\0';
            mvprintw(r++, INTR_COL, " %4s %s %.32s",
                shownum(4, _systat[0].deltas.intr[i]), _intrnums[i], buf);
	}
    }

    mvprintw(2, PAGER_COL, "     VN PAGER  SWAP PAGER");
    mvprintw(3, PAGER_COL, "     in out      in out");
    /*                        12345 12345 12345 12345   */
    /*
    mvprintw(4, PAGER_COL,   " %5lu %-5lu %5lu %-5lu",
        _systat[0].deltas.pager[0],
        _systat[0].deltas.pager[1],
        _systat[0].deltas.swapper[0],
        _systat[0].deltas.swapper[1]);
    */
    p = showmem(5, _systat[0].deltas.pager[0]);
    mvprintw(4, PAGER_COL+2, "%5s", p);
    p = showmem(5, _systat[0].deltas.pager[1]); while (*p == ' ') p++;
    mvprintw(4, PAGER_COL+8, "%-5s", p);
    p = showmem(5, _systat[0].deltas.swapper[0]);
    mvprintw(4, PAGER_COL+14, "%5s", p);
    p = showmem(5, _systat[0].deltas.swapper[1]); while (*p == ' ') p++;
    mvprintw(4, PAGER_COL+20, "%-5s", p);

    r = DISKS_ROW;
    mvprintw(r+0, 0, "Disks");
    mvprintw(r+1, 0, "  tps");
    mvprintw(r+2, 0, "xfers");
    // mvprintw(r+3, 0, " blks");
    // mvprintw(r+4, 0, "%busy");
    // separate blks read/written
    mvprintw(r+3, 0, " rblk");
    mvprintw(r+4, 0, " wblk");
    mvprintw(r+5, 0, "%busy");
    // for each block device, print 4 stats (show as many devices as can fit, 16 max)
    n = (PAGER_COL - 6) / 6 ;
    if (DISKS_ROW >= 18) n += 2;
    if (n > 16) n = 16;
    double interval = fptime() - _LAST_SAMPLE;
    for (coli=0, i=0; i<n; i++) {
        double busy = _systat[0].deltas.diskinfo[i][3] / (interval * 1000) * 100;
        if (!strlen(_disknames[i])) continue;
        /* only show devices with activity, to skip controllers without attached devices */
        if (_systat[0].counts.diskinfo[i][1]) {
            mvprintw(r+0, 6+6*coli, "%6s", _disknames[i]);
            mvprintw(r+1, 6+6*coli, "%6.6s", shownum(5, _systat[0].deltas.diskinfo[i][0]));     // tps
            mvprintw(r+2, 6+6*coli, "%6.6s", shownum(5, _systat[0].deltas.diskinfo[i][1]));     // xfers
            mvprintw(r+3, 6+6*coli, "%6.6s", showmem(5, _systat[0].deltas.diskinfo[i][4]));     // rblk
            mvprintw(r+4, 6+6*coli, "%6.6s", showmem(5, _systat[0].deltas.diskinfo[i][5]));     // wblk
            mvprintw(r+5, 6+6*coli, "%6.6s", busy ? showfloat(5, 1, busy) : " ");               // %busy
            ++coli;
        }
    }

    /*
     * NB: net-device auto-detect is not reliable; appears only when used
     * and does not disappear when un-registered ([0].counts > 0 !)
     * NB: ppp0 does disappear when connection is terminated...
     */
    for (rowi=14, i=0; i<_nnet && rowi<14+8; i++) {
        if (i >= DISKS_ROW) break;
        mvprintw(rowi, 0, "                                                    ");
	move(rowi,0);
        // only display those interfaces that have ever been active
	if (_systat[0].counts.net[i*4 + 0] || _systat[0].counts.net[i*4 + 2]) {
	    printw("%8s:  Rx: %s/%s Tx: %s/%s             ",
		   _netname[i],
		   showscaledcount(5, _systat[0].deltas.net[i*4 + 0]),
		   showscaledcount(5, _systat[0].deltas.net[i*4 + 1]),
		   showscaledcount(5, _systat[0].deltas.net[i*4 + 2]),
		   showscaledcount(5, _systat[0].deltas.net[i*4 + 3]));
            rowi++;
        }
    }

    move(LINES-1, COLUMNS-1);

    return 0;
}

int usage( int ecode )
{
    static char msg[] =
	"systat " VERSION " -- BSD-style display of system activity\n"
	"usage: systat [interval]\n"
	"\n"
	"Options:\n"
	"  -h    show this message\n"
	"  -V    show version and exit\n"
	"\n"
	"Written by Andras Radics to mimic the BSD 2.2 \"% systat -vm\" command.\n"
	"Motto: Linux is Unix. GNU's Not Unix.\n"
	"";
    fprintf((ecode ? stderr : stdout), "%s", msg);
    exit(ecode);
}

void die( int ecode )
{
    fflush(stdout);
    echo();
    nocbreak();
    // IMPORTANT: endwin must be called after echo and cbreak have been restored, else
    // the terminal is left in noecho mode with printed newlines not adding carriage returns
    endwin();
    exit(ecode);
}

void resize_screen( )
{
    if (_win) endwin();
    _win = initscr();
    if (!_win) die(3);
    clear();

    show_stats();
    refresh();
}

int main( int ac, char *av[] )
{
    char **argv = av;
    double pause = 1.00, mark;
    extern int optind;
    int old_rows, old_cols;
    int c, done = 0;

    // get terminal size
    on_winch();
    old_rows = LINES;
    int old_columns = COLUMNS;

    signal(SIGWINCH, on_winch);
    signal(SIGHUP, on_sighup);
    signal(SIGKILL, on_sigterm);

    while ((c = getopt(ac, av, "hV")) >= 0) {
	switch (c) {
	case 'h':
	    usage(0);
	    break;
	case 'V':
	    printf("systat " VERSION "\n");
	    exit(0);
	    break;
	case '?':
	    fprintf(stderr, "try -h for help\n");
	    exit(1);
	    break;
	default:
	    ;
	}
    }
    ac -= optind;
    av += optind;

    if (ac > 0)
        pause = atof(*av);

// FIXME: long intervals do not work as expected (5 sec, 50 sec)
    _INTERVAL = pause;
    _linuxver = gather_version();
    _pageKB = getpagesize() / 1024;
    _btime = getbtime();
    _LAST_SAMPLE = _btime;
    _exec_start_delta = get_exec_drift(_btime);

    _win = initscr();
    if (!_win) die(2);
    clear();

    cbreak();
    noecho();
    nodelay(_win, 1);

    mark = fptime();
    long loop_count = 0;
    for ( ; !done && !_had_hup && !_had_sigterm; loop_count++) {
	int paused = 0;
	gather_stats(loop_count);
	int now = fptime();
	delta_stats();
	show_stats();
	_LAST_SAMPLE = now;
	refresh();

	if (pause == 0.0) break;

        mark += pause;
        if (mark < fptime()) mark = fptime();

        do {
            fpsleep(.01);

	    switch(getch()) {
	    case 'q':
		done = 1;
		break;
            // note: if ^Z has no effect, `reset` the terminal
            case ' ':
                mark = fptime();
                paused = !paused;
                break;
	    }

            // if window size changed, redraw the whole page
            if (LINES != old_rows || COLUMNS != old_columns) {
                old_rows = LINES;
                old_columns = COLUMNS;
                resize_screen();
            }

        } while (paused || (fptime() < mark && !done));
    }

    /* on receipt of SIGHUP restart (reload new binary) */
    if (_had_hup) {
        execvp(argv[0], argv);
        /* NOTREACHED */
    }

    die(0);
}
