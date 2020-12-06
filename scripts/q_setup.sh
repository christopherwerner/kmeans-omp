module load gcc/5.3.0
module load papi/5.5.0
export LC_ALL=C

# echo environment
#source /q_info.sh

echo "qsub environment:"
echo "host          : $PBS_O_HOST"
echo "workdir       : $PBS_O_WORKDIR"
echo "original queue: $PBS_O_QUEUE"
echo "actual queue  : $PBS_QUEUE"
echo "environment   : $PBS_ENVIRONMENT"
echo "job ID        : $PBS_JOBID"
echo "job name      : $PBS_JOBNAME"

scripts_dir=$PBS_O_WORKDIR
src_dir=${scripts}/..

source ${scripts_dir}/q_setup.sh

cd $src_dir
make all

cd $scripts
export KMEANS_HOME=$src_dir
source jutland_run.sh


