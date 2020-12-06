#!/usr/bin/env bash
# Run the kmeans program multiple times and collect results
current_dir=$( cd "$( dirname ${BASH_SOURCE[0]} )" && pwd )
data_dir=${current_dir}/data
out_dir=${current_dir}/outdata
test_dir=${current_dir}/testdata
metrics_dir=${current_dir}/reports
bin_dir=${current_dir}/bin
kmeans_simple=${bin_dir}/kmeans_simple
kmeans_omp=${bin_dir}/kmeans_omp1

multirun() {
  local indata=${data_dir}/${in}
  local outdata=${out_dir}/${out}
  local metrics_file=${metrics_dir}/${metrics}
  local test_file=${test_dir}/${test}
  local test_args="-t ${test_file}"

  if [ ! -f "${indata}" ]; then
    echo "Cannot find input at ${indata}"; return 1;
  else
    echo "raw input data  :  ${indata}"
  fi

  if [ -f "${metrics_file}" ]; then
    echo "Found an existing metrics file at ${metrics_file}"
	  askcontinue "Do you want to delete the report at ${metrics_file} and start clean?" && rm -f $metrics_file
  fi

  echo "clustered data  :  ${outdata}"
  echo "metrics report  :  ${metrics_file}"
  echo "num clusters   : ${num_clusters}"
  echo "max points     : ${max_points}"
  echo "max iterations : ${max_iterations}"

  # simple run first
  local program=${kmeans_simple}
  local full_label="${label} simple"
  singlerun

  for ((i=0; i<=max_threads; i+=thread_step)) do
     export OMP_NUM_THREADS=$i
     local omp_label="${label} omp threads=$i"

     for ((j=min_chunks; j<=max_chunks; j+=chunk_step)) do
       export OMP_SCHEDULE=dynamic,$j
       full_label="${omp_label}; schedule=$OMP_SCHEDULE"
       echo "====== RUNNING $full_label ======"
       singlerun
     done
  done
}

singlerun() {
#  echo "threads: $OMP_NUM_THREADS"
  "${program}" -f "${indata}" -o "${outdata}" -m "${metrics_file}" ${test_args} \
      -k ${num_clusters} -n ${max_points} -i ${max_iterations} -l "${full_label}"
#  local args="-f ${indata} -o ${outdata} -m ${metrics_file} -m
    echo "singlerun"
}

askcontinue() {
  local question=${1:-"Do you want to continue?"}
	read -p "$question (waiting 5s then defaulting to YES)? " -n 1 -r -t 5
	echo    # (optional) move to a new line
	# if no reply (timeout) or yY then yes, else leave it false
	if [[ -z "$REPLY" || $REPLY =~ ^[Yy]$ ]];then
		return 0
	fi
	return 1
}

in="iris_petals_2.csv"
out="iris_petals_2_clustered.csv"
test="iris_petals_knime.csv"
metrics="iris_petals_metrics.csv"
num_clusters=3
max_points=200
max_iterations=20
max_threads=4
thread_step=2
min_chunks=2
max_chunks=4
chunk_step=2
label=iris
multirun

in="s1.csv"
out="s1_clustered.csv"
test="s1_clustered_knime.csv"
metrics="s1_metrics.csv"
num_clusters=15
max_points=10000
max_iterations=200
max_threads=32
thread_step=2
min_chunks=2
max_chunks=4
chunk_step=2
label=s1
#multirun

echo "done"
