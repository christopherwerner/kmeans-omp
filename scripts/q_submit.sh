#!/usr/bin/env bash

# non-interactive, use mei queue, 1 processor, 1 hour
qsub -qmei -lnodes=1 -lwalltime=1:30:00 ./q_setup.sh
