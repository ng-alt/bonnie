
all: bonnie++

SCRIPTS=bon_csv2html bon_csv2txt
EXES=bonnie++ zcav
#PPC=-mno-powerpc -mno-power

#OS2=yes

ifdef OS2
CC=icc /G4 /Gm+ /Gl+ /O+ /Oi+ /Ol+ -DOS2 /Q+
else
CC=gcc -O2 -g -Wall -pipe
endif

SOURCE=bon_io.cpp bon_file.cpp bon_time.cpp forkit.cpp semaphore.cpp
ifdef OS2
OBJS=$(SOURCE:.cpp=.obj)
else
OBJS=$(SOURCE:.cpp=.o)
endif

all: $(EXES)

bonnie++: bonnie++.cpp $(OBJS)
	$(CC) bonnie++.cpp -o bonnie++ $(OBJS) $(LFLAGS)

zcav: zcav.cpp
	$(CC) zcav.cpp -o zcav $(LFLAGS)

install: $(EXES)
	mkdir -p $(DESTDIR)/usr/bin $(DESTDIR)/usr/sbin
	strip bonnie++ zcav
	cp $(SCRIPTS) $(DESTDIR)/usr/bin
	cp zcav bonnie++ $(DESTDIR)/usr/sbin

ifdef OS2
%.obj: %.cpp %.h bonnie.h
else
%.o: %.cpp %.h bonnie.h
endif
	$(CC) -c $< -o $@

clean:
	rm -f $(EXES) $(OBJS)
	rm -rf build-stamp install-stamp debian/tmp err out


