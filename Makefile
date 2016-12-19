OBJ=vox.o adpcm.o
DOBJ=devox.o adpcm.o
CC=gcc
CFLAGS=
INSTALLDIR=/usr/local

all: devox vox

install: all
	cp vox $(INSTALLDIR)/bin	
	cp devox $(INSTALLDIR)/bin	
	cp vox.1 $(INSTALLDIR)/man/man1
	ln -s $(INSTALLDIR)/man/man1/vox.1 $(INSTALLDIR)/man/man1/devox.1

devox: ${DOBJ}
	$(CC) -o $@ ${DOBJ}

vox: ${OBJ}
	$(CC) -o $@ ${OBJ}

vox.o:vox.c adpcm.h
	$(CC) $(CFLAGS) -c vox.c 

devox.o:devox.c adpcm.h
	$(CC) $(CFLAGS) -c devox.c 

adpcm.o:adpcm.c adpcm.h
	$(CC) $(CFLAGS) -c adpcm.c
