#all:map.so

#CFLAGS = -g3 -O0 -Wall -fPIC --shared
#FILE = /home/ejoy/test/c/map
#map.so:main.c
#	gcc $(CFLAGS) -I$(FILE) &^ -o $@
#

all: map

map:main.c
	#gcc main.c -lm -o map
	gcc -g3 -o0 -Wall -fPIC $^ -lm  -o $@

clear:
	rm map
