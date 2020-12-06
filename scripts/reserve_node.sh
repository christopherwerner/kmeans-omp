#!/usr/bin/env bash

# interactive, 1 processor, 1 hour
qsub -I -qcpar -lnodes=1 -lwalltime=1:00:00
