#!/bin/bash          
patch -p1 < ./patch/ethernet.c.patch
patch -p1 < ./patch/generic.c.patch
patch -p1 < ./patch/fsm_soe.c.patch

