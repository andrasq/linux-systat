systat: systat.c
	cc -Os ${CFLAGS} -o systat systat.c -lncurses -lm

install: systat
	sudo install systat /usr/local/bin/

clean:
	rm -f systat
