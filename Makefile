# Generated automatically from Makefile.in by configure.
EXES=bonnie++ zcav getc_putc getc_putc_helper
EXE=bon_csv2html generate_randfile

all: $(EXE) $(EXES)

SCRIPTS=bon_csv2txt

prefix=/home/rjc/debian/bonnie++-1.93/debian/bonnie++/usr
eprefix=${prefix}
#MORE_WARNINGS=-Weffc++ -Wcast-align
WFLAGS=-Wall -W -Wshadow -Wpointer-arith -Wwrite-strings -pedantic -ffor-scope $(MORE_WARNINGS)
CFLAGS=-O2  -DNDEBUG $(WFLAGS) $(MORECFLAGS)
CXX=c++ $(CFLAGS)
LINK=c++
THREAD_LFLAGS=-lpthread

INSTALL=/usr/bin/install -c

BONSRC=bonnie++.cpp bon_io.cpp bon_file.cpp bon_time.cpp semaphore.cpp \
 sync.cpp thread.cpp bon_suid.cpp duration.cpp rand.o util.o
BONOBJS=$(BONSRC:.cpp=.o)

MAN1=bon_csv2html.1 bon_csv2txt.1 generate_randfile.1
MAN8=bonnie++.8 zcav.8 getc_putc.8

ZCAVSRC=zcav.cpp thread.cpp zcav_io.cpp bon_suid.cpp duration.cpp
ZCAVOBJS=$(ZCAVSRC:.cpp=.o)

GETCSRC=getc_putc.cpp bon_suid.cpp duration.cpp util.o
GETCOBJS=$(GETCSRC:.cpp=.o)

GETCHSRC=getc_putc_helper.cpp duration.cpp
GETCHOBJS=$(GETCHSRC:.cpp=.o)

ALLOBJS=$(BONOBJS) $(ZCAVOBJS) generate_randfile.o bon_csv2html.o

bonnie++: $(BONOBJS)
	$(LINK) -o bonnie++ $(BONOBJS) $(THREAD_LFLAGS)

zcav: $(ZCAVOBJS)
	$(LINK) -o zcav $(ZCAVOBJS) $(THREAD_LFLAGS)

getc_putc: $(GETCOBJS)
	$(LINK) -o getc_putc $(GETCOBJS) $(THREAD_LFLAGS)

getc_putc_helper: $(GETCHOBJS)
	$(CXX) -o getc_putc_helper $(GETCHOBJS)

bon_csv2html: bon_csv2html.o
	$(LINK) bon_csv2html.o -o bon_csv2html

generate_randfile: generate_randfile.o
	$(LINK) generate_randfile.o -o generate_randfile

install-bin: $(EXE) $(EXES)
	mkdir -p $(eprefix)/bin $(eprefix)/sbin
	${INSTALL} -s $(EXES) $(eprefix)/sbin
	${INSTALL} -s $(EXE) $(eprefix)/bin
	${INSTALL} $(SCRIPTS) $(eprefix)/bin

install: install-bin
	mkdir -p /home/rjc/debian/bonnie++-1.93/debian/bonnie++/usr/share/man/man1 /home/rjc/debian/bonnie++-1.93/debian/bonnie++/usr/share/man/man8
	${INSTALL} -m 644 $(MAN1) /home/rjc/debian/bonnie++-1.93/debian/bonnie++/usr/share/man/man1
	${INSTALL} -m 644 $(MAN8) /home/rjc/debian/bonnie++-1.93/debian/bonnie++/usr/share/man/man8

%.o: %.cpp
	$(CXX) -c $<

clean:
	rm -f $(EXE) $(EXES) $(ALLOBJS) build-stamp install-stamp
	rm -rf debian/tmp core debian/*.debhelper
	rm -f debian/{substvars,files} config.log depends.bak

realclean: clean
	rm -f config.* Makefile bonnie++.spec
	rm -f bon_csv2txt bon_csv2html.1 sun/pkginfo

dep:
	makedepend -Y -f depends *.cpp 2> /dev/null

include depends

