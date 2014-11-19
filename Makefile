DESTDIR = /N/B/gustav/bin
MANDIR  = /N/B/gustav/man/man1
CC = cc
CFLAGS = -I/N/hpc/mpich2/include
LIBS = -L/N/hpc/mpich2/lib
LDFLAGS = -lmpich
TARGET = hellow2

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) -o $@ $(TARGET).o $(LIBS) $(LDFLAGS)

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c $(TARGET).c

install: all $(TARGET).1
	[ -d $(DESTDIR) ] || mkdirhier $(DESTDIR)
	install $(TARGET) $(DESTDIR)
	[ -d $(MANDIR) ] || mkdirhier $(MANDIR)
	install $(TARGET).1 $(MANDIR)

clean:
	rm -f *.o $(TARGET)

clobber: clean
	rcsclean