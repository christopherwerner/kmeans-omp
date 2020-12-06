
UNAME_S := $(shell uname -s)
SOURCEDIR=./
OUTDIR=bin/

DEBUG_FLAGS=
#DEBUG_FLAGS=-DTRACE
#DEBUG_FLAGS=-DDEBUG -DTRACE
# Uncomment this to get some OMP debug info like threads assigned etc
#DEBUG_FLAGS=-DDEBUG_OMP

ifeq ($(UNAME_S),Linux)
CXX=gcc
INCLUDES=-I/share/apps/papi/5.5.0/include
CXXFLAGS= -O3 -std=c99 -g -fopenmp $(INCLUDES) $(DEBUG_FLAGS)
LIBS=-L/share/apps/papi/5.5.0/lib -lpapi
endif
ifeq ($(UNAME_S),Darwin)
CXX=/usr/local/bin/gcc-10
INCLUDES=
CXXFLAGS= -O3 -std=c99 -g -fopenmp $(INCLUDES) $(DEBUG_FLAGS)
LIBS=
endif
#CXXFLAGS= -O3 -std=c++11 -mavx -pg -qopenmp -qopt-report5 $(INCLUDES)

PROGS=$(OUTDIR)kmeans

.PHONY: all
all: $(OUTDIR) kmeans_simple kmeans_omp1 kmeans_omp2

kmeans_simple:
	$(CXX) $(CXXFLAGS) -o $(OUTDIR)kmeans_simple $(SOURCEDIR)kmeans.c $(SOURCEDIR)kmeans_support.c \
 						  $(SOURCEDIR)kmeans_simple_impl.c $(SOURCEDIR)csvhelper.c $(HEADERS) $(LIBS)

kmeans_omp1:
	$(CXX) $(CXXFLAGS) -o $(OUTDIR)kmeans_omp1 $(SOURCEDIR)kmeans.c $(SOURCEDIR)kmeans_support.c \
 						  $(SOURCEDIR)kmeans_omp1_impl.c $(SOURCEDIR)csvhelper.c $(HEADERS) $(LIBS)

kmeans_omp2:
	$(CXX) $(CXXFLAGS) -o $(OUTDIR)kmeans_omp2 $(SOURCEDIR)kmeans.c $(SOURCEDIR)kmeans_support.c \
 						  $(SOURCEDIR)kmeans_omp1_impl.c $(SOURCEDIR)csvhelper.c $(HEADERS) $(LIBS)

$(OUTDIR):
	mkdir $(OUTDIR)

.PHONY: clean
clean:
	rm -r $(OUTDIR)

