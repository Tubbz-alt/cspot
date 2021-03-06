#!/bin/bash

yum -y update
yum -y install epel-release
yum -y install zeromq-devel
yum -y install gcc autoconf gdb git make cmake libtool uuid-devel
mkdir -p /src
cd /src
git clone git://github.com/zeromq/czmq.git
cd czmq
./autogen.sh && ./configure && make check
make install
ldconfig
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
