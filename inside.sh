#!/bin/bash

echo Inside: start of script

export CVMFS_DISTRO_BASE=/cvmfs/nova.opensciencegrid.org/
source $CVMFS_DISTRO_BASE/novasoft/slf6/novasoft/setup/setup_nova.sh

cafe -bq test_inside.C

#./test_inside

echo Inside: end of script
