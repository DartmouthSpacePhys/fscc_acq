#!/bin/bash

PORT=${1:-0}
ACQ_N=${2:-50}
RTD_DT=${3:-0.5}
RTD_AVG=${4:-6}

sudo ./fscc_acq -p ${PORT} -x ${ACQ_N} -d ${RTD_DT} -a ${RTD_AVG}
