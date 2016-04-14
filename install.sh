#!/bin/bash          
./bootstrap
./configure --enable-sii-assign --disable-8139too --enable-hrtimer --enable-cycles --enable-e1000e
sudo /opt/etherlab/etc/init.d/ethercat stop
make clean
make all modules
sudo make modules_install install
sudo ldconfig
sudo depmod
sudo /opt/etherlab/etc/init.d/ethercat start
