#!/bin/bash          
echo -n "char* gitlog = \"" > master/gitlog.h && git log | cut -c 8-15 -z >> master/gitlog.h && echo "\";" >> master/gitlog.h
./bootstrap
./configure --enable-sii-assign --disable-8139too --enable-hrtimer --enable-cycles
sudo /opt/etherlab/etc/init.d/ethercat stop
make clean
make all modules
sudo make modules_install install
sudo ldconfig
sudo depmod
sudo /opt/etherlab/etc/init.d/ethercat start
