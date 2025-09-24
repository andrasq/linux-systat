systat: systat.c
	cc -Os ${CFLAGS} -o systat systat.c -lncurses -ltinfo -lm

static:
	CFLAGS="-static" make systat

install: systat
	sudo install systat /usr/local/bin/

clean:
	rm -f systat
