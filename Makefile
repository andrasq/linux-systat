# ncurses needs tinfo, but -static does not load it automatically
systat: systat.c Makefile
	cc -O2 ${CFLAGS} -o systat systat.c -lncurses -ltinfo -lm

static:
	CFLAGS="-static" make systat

install: systat
	sudo install systat /usr/local/bin/

clean:
	rm -f systat
