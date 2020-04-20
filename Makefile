#
# Copyright 2018, Derrow Decision Developments
#
# PiBrematic Brennenstuhl Breamtic Clone
#


PiBrematic:
	gcc -DRPI PiBrematic.c -o PiBrematic -lwiringPi

clean:
	rm -f *.o PiBrematic

