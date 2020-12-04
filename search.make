SOURCEDIR=./
OUTDIR=bin/

# mac:
#CXX=/usr/local/bin/gcc-10
#INCLUDES=
#LIBS=

CXX=gcc
INCLUDES=-I/share/apps/papi/5.5.0/include
CXXFLAGS= -O3 -std=c99 -g -fopenmp $(INCLUDES)
#CXXFLAGS= -O3 -std=c++11 -mavx -pg -qopenmp -qopt-report5 $(INCLUDES)
LIBS=-L/share/apps/papi/5.5.0/lib -lpapi

PROGS=$(OUTDIR)kmeans

.PHONY: default
default: $(OUTDIR) #$(SOURCEDIR)kmeans.c
	$(CXX) $(CXXFLAGS) -o $(PROGS) $(SOURCEDIR)kmeans.c $(SOURCEDIR)csvhelper.c $(LIBS)

$(OUTDIR):
	mkdir $(OUTDIR)

.PHONY: clean
clean:
	rm -r $(OUTDIR)
