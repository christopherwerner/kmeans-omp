#!/usr/bin/env bash

echo "qsub environment:"
echo "host          : $PBS_O_HOST"
echo "workdir       : $PBS_O_WORKDIR"
echo "original queue: $PBS_O_QUEUE"
echo "actual queue  : $PBS_QUEUE"
echo "environment   : $PBS_ENVIRONMENT"
echo "job ID        : $PBS_JOBID"
echo "job name      : $PBS_JOBNAME"
