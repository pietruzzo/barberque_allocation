#!/bin/bash
#
# Copyright (C) 2014  Politecnico di Milano
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

# ===================== Setup =================================

AMD_SAMPLE_PREFIX="/opt/AMDAPP/samples/opencl/bin/x86_64/"

### GPU coscheduling scenarios
sample_cmdline=(
	$AMD_SAMPLE_PREFIX"NBody -i 250 -x 32768 -q -t"
	$AMD_SAMPLE_PREFIX"FluidSimulation2D -i 10000 -q"
	$AMD_SAMPLE_PREFIX"MonteCarloAsian -i 10 -c 256"
)

# =============================================================

#1: Sample
#2: Number of instances
#3: Parameter flag
#4: Parameter value
function run_sample {
	for i in `seq 1 $2`; do
		printf "Running up to %d instances " $2
		($1 -q -i ${NUMITER[$SEL]} $3 $4 -t &)
	done
	wait
}

function run_gpu_cosched_scenario {
	declare -A cosched_times
	echo "...Run scenarios..."
	for i in `seq 0 2`; do
		for j in `seq 0 2`; do
			printf "[%s, %s]...\n" ${ocl_names[$i]} ${ocl_names[$j]}
			startSamplePair $i $j 0
			#sleep 3
		done
	done

	# Printing
	echo "Results matrix:"
	for i in `seq 0 2`; do
		for j in `seq 0 2`; do
			printf "\t%d" ${cosched_times[$i,$j]}
		done
		printf "\n"
	done
	echo ${cosched_times[@]}
}

function startSamplePair {
	#launch $1 $2 > /dev/null 2>1 &
	if [ $3 -eq "0" ]; then
		launchNoBBQSample $1 $2  &
	elif [ $3 -eq "0" ]; then
		launchBBQSample $1 $2 > /dev/null 2>&1 &
	fi

	sleep 1
	if [ $3 -eq "0" ]; then
		launchNoBBQSample $2 $1  &
	elif [ $3 -eq "0" ]; then
		launchBBQSample $2 $1 > /dev/null 2>&1 &
	fi
	wait
}

function launchNoBBQSample {
	echo " NOBBQ: " ${sample_cmdline[$1]}
	local start_t=0
	local end_t=0
	local diff_t=0
	start_t=$(date +%s)
	eval ${sample_cmdline[$1]} 2>&1 |./getAdapterInfoNOBBQ.awk
	end_t=$(date +%s)
	diff_t=$((end_t-start_t))
	cosched_times[$1,$2]=$diff_t
	printf "\t"
	echo ${ocl_names[$1]} "has finished in" ${cosched_times[$1,$2]} "s"
#	echo ${cosched_times[$1,$2]}" " >> \
#		$TESTDIR"/GPUcs-"${ocl_names[$1]}"_"${ocl_names[$2]}".dat"

	ADAPTER_INFO=$(cat /tmp/OCLSampleRuntime.dat)
	IFS=" " read -ra INFO <<< "$ADAPTER_INFO"
	printf "%d %d %d %d %d\n" $diff_t ${INFO[0]} ${INFO[1]} ${INFO[2]} ${INFO[3]} >>\
		$TESTDIR"/GPUcs-"${ocl_names[$1]}"_"${ocl_names[$2]}".dat"


}


#################### Start the test ######################################
# Import setup data
source setupSamples.sh
setup

case $1 in
	1)	# Evaluate the GPU co-scheduling combinations
		for ((r=1; r <=$NUMRUN; ++r)); do
			printf " ################## RUN[%d] #################\n" $r
			run_gpu_cosched_scenario
		done
		exit 1
		;;
	"-h")
		print_help
		;;

esac


for s in $AMD_SAMPLES; do
	# Launch sample
	SAMPLE=$s
	SEL=""
	CS=${s:0:2}
	case $CS in
		"NB")
			SEL=1
			PVALUES=${NB_PARAMS[@]}
			;;
		"Fl")
			SEL=2
			;;
		"Mo")
			SEL=3
			PVALUES=${MC_PARAMS[@]}
			;;
		# -- 4: Stereomatching missing here --
		"Ma")
			SEL=5
			;;
		*)
			SEL=0
			printf "No parameters for %s\n" $s
			;;
	esac

	printf "============== Sample=%s SEL=%s ================\n" $SAMPLE $SEL
	printf "Parameter set = %s\n" $PVALUES

	for i in $NUMINST; do
		printf "# Instances = %s\n" $i

		# --- FluidSimulation2D --- #
		if [ $SEL == 2 ]; then
			printf "[%s] No application parameters\n" $SAMPLE
			OUTFILENAME=$TESTDIR/"NOBBQ-"$SAMPLE"-N"$i"-I"${NUMITER[$SEL]}"-P0-Runtime.dat"
			printf "[%s] Output file: %s\n" $SAMPLE $OUTFILENAME
			for ((r=1; r <=$NUMRUN; ++r)); do
				print_test_header $SAMPLE $r $i $p
				echo $AMD_SAMPLE_PREFIX$SAMPLE -q -i ${NUMITER[$SEL]} -t
				START=$(date +%s)
				(run_sample $AMD_SAMPLE_PREFIX$SAMPLE $ix) 2>&1 |./getAdapterInfoNOBBQ.awk
				END=$(date +%s)
				DIFF=$((END-START))
				ADAPTER_INFO=$(cat /tmp/OCLSampleRuntime.dat)
				IFS=" " read -ra INFO <<< "$ADAPTER_INFO"
				printf "%d %d %d %d %d\n" $DIFF ${INFO[0]} ${INFO[1]} ${INFO[2]} ${INFO[3]} >> $OUTFILENAME
				printf "%d %d %d %d %d\n" $DIFF ${INFO[0]} ${INFO[1]} ${INFO[2]} ${INFO[3]}
				sleep 2
			done
		else
		# --- Nbody, Montecarlo --- #
			for p in $PVALUES; do
				printf "[%s] Parameter value = %d\n" $SAMPLE $p
				OUTFILENAME=$TESTDIR/"NOBBQ-"$SAMPLE"-N"$i"-I"${NUMITER[$SEL]}"-P"$p"-Runtime.dat"
				printf "[%s] Output file: %s\n" $SAMPLE $OUTFILENAME
				for ((r=1; r <=$NUMRUN; ++r)); do
					print_test_header $SAMPLE $r $i $p
					echo $AMD_SAMPLE_PREFIX$SAMPLE -i ${NUMITER[$SEL]} ${ARGS[$SEL]} $p -t
					START=$(date +%s)
					(run_sample $AMD_SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]} $p) 2>&1 |./getAdapterInfoNOBBQ.awk
					END=$(date +%s)
					DIFF=$((END-START))
					ADAPTER_INFO=$(cat /tmp/OCLSampleRuntime.dat)
					IFS=" " read -ra INFO <<< "$ADAPTER_INFO"
					printf "%d %d %d %d %d\n" $DIFF ${INFO[0]} ${INFO[1]} ${INFO[2]} ${INFO[3]} >> $OUTFILENAME
					printf "%d %d %d %d %d\n" $DIFF ${INFO[0]} ${INFO[1]} ${INFO[2]} ${INFO[3]}
					sleep 2
				done
			done
		else
		# --- Mandelbrot --- #
			for p in $PVALUES; do
				OUTFILENAME=$TESTDIR/"NOBBQ-"$SAMPLE"-N"$i"-I"${NUMITER[$SEL]}"-P"$p"-Runtime.dat"
				printf "[%s] Output file: %s\n" $SAMPLE $OUTFILENAME
				for ((r=1; r <=$NUMRUN; ++r)); do
					print_test_header $SAMPLE $r $i $p
					echo $AMD_SAMPLE_PREFIX$SAMPLE -i ${NUMITER[$SEL]} ${ARGS[$SEL]} --double
					START=$(date +%s)
					(run_sample $AMD_SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]} --double) 2>&1 |./getAdapterInfoNOBBQ.awk
					END=$(date +%s)
					DIFF=$((END-START))
					ADAPTER_INFO=$(cat /tmp/OCLSampleRuntime.dat)
					IFS=" " read -ra INFO <<< "$ADAPTER_INFO"
					printf "%d %d %d %d %d\n" $DIFF ${INFO[0]} ${INFO[1]} ${INFO[2]} ${INFO[3]} >> $OUTFILENAME
					printf "%d %d %d %d %d\n" $DIFF ${INFO[0]} ${INFO[1]} ${INFO[2]} ${INFO[3]}
					sleep 2
				done
			done
		fi
	done
done




