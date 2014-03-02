#
#  flacplay.dll makefile for gnu MAKE and g++
#

# build environment
#-----------------------
#  gcc -v
# 
# Reading specs from J:/usr/gcc/3.3.5-csd3/lib/gcc-lib/i386-pc-os2-emx/3.3.5/specs
# Configured with: D:/CODING/LIBC/SVN/BRANCHES/LIBC-0.6/src/gcc/configure 
#  --enable-clh --enable-threads=os2 --enable-shared=libgcc,bfd,opcodes --enable-nls 
#  --without-included-gettext --with-local-prefix=D:/CODING/LIBC/SVN/BRANCHES/LIBC-0.6/TOOLS/x86.os2/gcc/staged 
#  --prefix=/gcc --with-gnu-as --disable-libgcj --enable-languages=c,c++
# Thread model: os2
# gcc version 3.3.5 (Bird Build 2007-06-10 14:30)
# 
#  make -v
# GNU Make version 3.77, by Richard Stallman and Roland McGrath.
# Copyright (C) 1988, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98
# 	Free Software Foundation, Inc.
# This is free software; see the source for copying conditions.
# There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.
# 
# Report bugs to <bug-make@gnu.org>.
# 
#

NAME = fmg
LIBS = -L/usr/test/lib -lm
INCLUDES  = -I/usr/test/include -I/usr/test/ffmpeg/include
DEPDIR = .deps
PRODVERSION :=$(shell cat Version)
PRODBUILD   :=$(shell cat Build)
PRODNAME     =ffmpeg
AUTHOR	     =ntim
CONTACTS     =<ntim@softhome.net>
BUILDHOST    =$(shell echo $(HOSTNAME) | awk '{printf "%13s",$$1}' )

#		(empty)- no debug, 1 - base, 2 - more, 3 - most verbose
#DEBUG=-DDEBUG=3	
DEBUG=-UDEBUG
DEFS=$(DEBUG) -DHAVE_CONFIG_H -DPRODNAME=\"$(PRODNAME)\" -DPRODVERSION=\"$(PRODVERSION)\" -DPRODBUILD=\"$(PRODBUILD)\" 
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
ADDOBJ = 
ADDLIB = -L/usr/test/ffmpeg/lib -llibavformat_s -llibavcodec_s -llibavutil_s -Lu:/extras/lib -la52 -lfaac -lfaad -lgsm -lmp3lame -lz -lm

CC = g++ -c
LD = g++ -s
RM = rm

CFLAGS   = -Zmts -Wall -O3
TARGET = $(NAME)play

all: Version Build $(DEPDIR) $(TARGET).dll

DEF      = $(NAME)play.def
OBJECTS := $(addprefix $(DEPDIR)/, $(NAME)play.o decoder.o decoder++.o debug.o allneeded.o)

$(TARGET).dll : $(OBJECTS) $(ADDOBJ) $(DEF) Makefile
	$(LD) -s -Zdll -Zmts $(CFLAGS) -o $(TARGET).dll $(OBJECTS) $(ADDOBJ) $(LIBS) $(ADDLIB) $(DEF)
#	lxlite $(TARGET).dll

clean:
	-@$(RM) *.dll $(OBJECTS) *.bak $(ADDOBJ) $(DEF) $(DEPDIR)/* 2> nul

# bldlevel support:
$(NAME)play.def: form.def Version Build Makefile
	DATE=`date +"%d %b %Y %X"` ; \
	sed -e 's/LIBRARY PRODNAME INITINSTANCE/LIBRARY '$(TARGET)' INITINSTANCE/g' \
	    -e 's/^DESCRIPTION .*/DESCRIPTION "@#'$(AUTHOR)':'$(PRODVERSION)'.'$(PRODBUILD)'#@##1## '"$$DATE"'$(BUILDHOST)::en:ru:$(PRODBUILD)::@@$(PRODNAME) plug-in for PM123 (C)2008 $(AUTHOR) $(CONTACTS)"/g' \
	    $< > $@

-include ./$(DEPDIR)/*.Po

$(DEPDIR)/%.o: %.cpp Version Build Makefile
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

$(DEPDIR)/%.o: %.c Version Build Makefile
	gcc -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

$(DEPDIR):
	mkdir $(DEPDIR)
