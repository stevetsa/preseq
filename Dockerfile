#### 
# http://smithlabresearch.org/software/preseq/
# The preseq package is aimed at predicting and estimating the complexity of a genomic sequencing library, equivalent to predicting and estimating the number of redundant reads from a given sequencing depth and how many will be expected from additional sequencing 
# using an initial sequencing experiment. The estimates can then be used to examine the utility of further sequencing, optimize the sequencing depth, 
# or to screen multiple libraries to avoid low complexity samples.
# 64-bit machine, GCC version >= 4.1, and GSL version 1.15.
####

FROM ubuntu:14.04
MAINTAINER Steve Tsang <mylagimail2004@yahoo.com>
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install --yes \
 build-essential \
 gcc-multilib \
# libgsl-dev \
 apt-utils \
 git-all \
 wget 

#ENV DEBIAN_FRONTEND=""
#RUN DEBIAN_FRONTEND=dialog apt-get install --yes libgsl-dev

RUN DEBIAN_FRONTEND=noninteractive apt-get install --yes automake autoconf zlib1g-dev libbz2-dev liblzma-dev libncurses5-dev libcurl3 libcurl4-openssl-dev libgsl0ldbl curl  gsl-bin
WORKDIR /opt
RUN git clone https://github.com/samtools/htslib.git
WORKDIR /opt/htslib
RUN autoheader
RUN autoconf
RUN ./configure
RUN make
RUN make install
ENV PATH "$PATH:/opt/htslib/"

WORKDIR /opt
RUN git clone https://github.com/samtools/samtools.git
WORKDIR /opt/samtools
RUN autoheader
RUN autoconf -Wno-syntax
RUN ./configure    # Optional, needed for choosing optional functionality
RUN make
RUN make install
ENV PATH "$PATH:/opt/samtools/"

# Download binary
WORKDIR /opt/
RUN wget http://smithlabresearch.org/downloads/preseq_linux_v2.0.tar.bz2
RUN tar -jxvf preseq_linux_v2.0.tar.bz2
ENV PATH "$PATH:/opt/preseq_v2.0/"

#RUN wget http://gnu.mirrors.pair.com/gsl/gsl-2.5.tar.gz
#RUN tar xvzf gsl-2.5.tar.gz
#WORKDIR /opt/gsl-2.5
#RUN ./configure --prefix=/usr/local/include/gsl
#RUN make
#RUN make check
#RUN make install

#Libraries have been installed in:
#   /opt/gsl-2.5/lib
#If you ever happen to want to link against installed libraries
#in a given directory, LIBDIR, you must either use libtool, and
#specify the full pathname of the library, or use the `-LLIBDIR'
#flag during linking and do at least one of the following:
#   - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
#     during execution
#   - add LIBDIR to the `LD_RUN_PATH' environment variable
#     during linking
#   - use the `-Wl,-rpath -Wl,LIBDIR' linker flag
#   - have your system administrator add LIBDIR to `/etc/ld.so.conf' 

#ENV LIBDIR /usr/local/include/gsl/lib

## storing a copy of source
WORKDIR /opt
RUN git clone --recursive https://github.com/smithlabcode/preseq
#RUN cp /opt/bowtie2-2.3.4.1-linux-x86_64/bowtie* /usr/local/bin

