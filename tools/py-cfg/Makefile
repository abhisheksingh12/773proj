# research/py-cfg/Makefile
#
# Mark Johnson, version of 27th August 2009
#
# Usage:
#
#   production:
#
#      make clean py-cfg
#
#   debugging:
#
#      make clean run-toy NDEBUG=
#
# 

top: py-cfg

.PHONY: run-toy
run-toy: py-cfg testengger.lt testeng.yld
	$(EXEC) py-cfg -D -R -1 -d 500 -a 0 -b 1 -n 1000 -E -A testeng.prs -N 10 -G testeng.wlt testengger.lt  < testeng.yld

############################################################
#                                                          #
#                    Program build                         #
#                                                          #
############################################################

NDEBUG=-DNDEBUG

ifeq ($(NDEBUG),-DNDEBUG)
  #
  # production
  #
  CC = $(CXX)
  CFLAGS = -MMD -O6 -Wall -ffast-math -finline-functions -fomit-frame-pointer -fstrict-aliasing $(GCCFLAGS)
  LDFLAGS = -Wall -O6 $(GCCLDFLAGS)
  EXEC = time
else
  #
  # debugging
  #
  CFLAGS = -g -O1 -MMD -Wall -ffast-math -fstrict-aliasing $(GCCFLAGS)
  LDFLAGS = -g -Wall $(GCCLDFLAGS)
  EXEC = valgrind --tool=memcheck --leak-check=full
endif

#
# profiling
#
# CFLAGS = -g -pg -O6 -MMD -Wall -ffast-math -fno-default-inline -fno-inline $(GCCFLAGS)
# CFLAGS = -g -pg -O -MMD -Wall -ffast-math $(GCCFLAGS)
# LDFLAGS = -g -pg

CXXFLAGS = $(CFLAGS) 

TARGETS = py-cfg
SOURCES = gammadist.c mt19937ar.c py-cfg.cc xtree.cc sym.cc
OBJECTS = $(patsubst %.l,%.o,$(patsubst %.c,%.o,$(SOURCES:%.cc=%.o)))

py-cfg: gammadist.o py-cfg.o mt19937ar.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

gammadist.o: gammadist.c
	gcc -c $(CFLAGS) -std=c99 $< -o $@

mt19937ar.o: mt19937ar.c
	gcc -c $(CFLAGS) $< -o $@

.PHONY: 
clean: 
	rm -fr *.o *.d *~ core

.PHONY: real-clean
real-clean: clean 
	rm -fr $(TARGETS)

# this command tells GNU make to look for dependencies in *.d files
-include $(patsubst %.l,%.d,$(patsubst %.c,%.d,$(SOURCES:%.cc=%.d)))
