## Specify the executable to create
bin_PROGRAMS = \
    weatherstation

## Specify the sources (no wildcards) for compilation
weatherstation_SOURCES = \
    weatherstation.c

## Specify the linker options
weatherstation_LDFLAGS = $(AM_LDFLAGS)

## Specify the libraries to link against
weatherstation_LDADD = \
    -lmosquitto \
    -lconfig \
    -lrt
    
## Specify the compiler options
weatherstation_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/include/private \
    -fPIC -pthread -Wall -O -fno-omit-frame-pointer -g3 \
    $(AM_CFLAGS)
