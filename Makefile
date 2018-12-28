systat: systat.c
	cc -Os -o systat systat.c -lncurses

install: systat
	sudo install systat /usr/local/bin/

clean:
	rm -f systat
