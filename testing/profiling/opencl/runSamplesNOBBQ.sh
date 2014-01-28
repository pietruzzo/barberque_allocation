#!/bin/bash
#
# Copyright (C) 2012  Politecnico di Milano
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

# ========================== Setup data ==================================

BOSP_BASE=${BOSP_BASE:-"/home/ccaffarri/BOSP"}
DATETIME=`date +%Y%m%d_%H%M%S`
DATETIME_STR=`date`

NUMRUN=${NUMRUN:-5}
NUMINST=${NUMINST:-"1 2 3 4"}
#NUMINST=${NUMINST:-"4"}
AWM=${AWM:-"0"}
DEVICES=${DEVICES:-"cpu gpu"}
AMD_SAMPLE_PREFIX="/opt/AMDAPP/samples/opencl/bin/x86_64/"
OUTDIR=${OUTDIR:-$BOSP_BASE"/out/var/ocl"}
OUTFILENAME="BbqueOCLStats"
DATADIR_PREFIX=${DATADIR_PREFIX:-"/tmp"}
XORG_CONF="/etc/X11/xorg.conf_gpu"

# ========================== Samples data ==================================
ocl_names=(nbody fluidsimulation2D montecarlo)
#AMD_SAMPLES="NBody FluidSimulation2D MonteCarloAsian"
#AMD_SAMPLES="NBody MonteCarloAsian"
AMD_SAMPLES="NBody"
#AMD_SAMPLES="MonteCarloAsian"
#AMD_SAMPLES="FluidSimulation2D MonteCarloAsian"
#SEL=$1
ARGS=("" "-x" "" "-c")
# Number of particles
#NB_PARAMS=(32768 16384 8192 4096 2048 1024)
NB_PARAMS=(32768)
# Number of steps
#MC_PARAMS=(8 16 32 64 128 256)
MC_PARAMS=(256)
# Number of iterations per sample execution
NUMITER=(0 1000 2000 5)


function setup {
	# Block GPUs frequency
	(amdconfig --adapter=all --od-setclocks=300,400 --input=$XORG_CONF)

	# Set cpufreq governor to 'performance'
	echo "cpufreq governors:"
	for i in `seq 0 7`;do
		cpu_gov=$(cat /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor)
		echo "cpu"$i": "$cpu_gov
		if [ ! $cpuf_gov = "performance" ]; then
			echo "Please set the cpufreq governor to performance!"
			exit
		fi
	done

	# Create output directory
	if [ ! -d "$OUTDIR" ]; then
		mkdir $OUTDIR
	fi
	TESTDIR=$OUTDIR/$DATETIME
	mkdir $TESTDIR
	echo "Created directory: " $TESTDIR
}

function run_sample {
	for i in `seq 1 $2`; do
		printf "Running up to %d instances " $2
		($1 -q -i ${NUMITER[$SEL]} $3 $4 -t &)
	done
	wait
}

### GPU coscheduling scenarios
sample_cmdline=(
$AMD_SAMPLE_PREFIX"NBody -i 300 -x 32768 -q -t"
$AMD_SAMPLE_PREFIX"FluidSimulation2D -i 1000 -q -t"
$AMD_SAMPLE_PREFIX"MonteCarloAsian -i 10 -c 256 -t")

# $1 = sample
# $2 = current number of run
# $3 = number of instances
# $4 = application parameter value

function print_test_header {
	printf "============================================\n"
	printf "| BBQ Test run    : %-3d / %-16d | \n" $2 $NUMRUN
	echo   "--------------------------------------------"
	printf "| SAMPLE          : %-17s      | \n" $1
	printf "| Instances       : %-17d      | \n" $3
	printf "| Parameter value : %-17d      | \n" $4
	printf "============================================  \n"
}


#1: Sample
#2: Number of instances
#3: Parameter flag
#4: Parameter value
function run_sample {
	for i in `seq 1 $2`; do
		printf "Running up to %d instances " $2
		(BBQUE_RTLIB_OPTS="o1" $1 -q -i ${NUMITER[$SEL]} $3 $4 &)
	done
	wait
}

function run_stereomatch {
	for i in `seq 1 $2`; do
		printf "Running up to %d instances " $2
	#	echo "BBQUE_RTLIB_OPTS=o1 $1 -i" $NUMITER \
	#		"--path_img_ref "$BOSP_BASE"/contrib/ocl-samples/StereoMatching/tsukuba" $3 $4 > nist
		(BBQUE_RTLIB_OPTS="o1" $1 -i ${NUMITER[$SEL]}\
			--path_img_ref $BOSP_BASE/contrib/ocl-samples/StereoMatching/tsukuba
			$3 $4 &)
	done
	wait
}

function launchScenarios {
	declare -A cosched_times
	echo "...Run scenarios..."
	for i in `seq 0 2`; do
		for j in `seq 0 2`; do
			#if [ "$i" -eq "$j" ]; then
				#echo " --- Skipping --- " $i $j
				#continue
			#fi
			echo "Running... " ${ocl_names[$i]} ${ocl_names[$j]}
			startSamples $i $j
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

function startSamples {
	#launch $1 $2 > /dev/null 2>1 &
	launch $1 $2  &
	sleep 2
	#launch $2 $1 > /dev/null 2>1 &
	launch $2 $1  &
	wait
}

function launch {
	echo ":::: Launching: " ${sample_cmdline[$1]}
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
setup


case $1 in
	1)	# Evaluate the GPU co-scheduling combinations
		for ((r=1; r <=$NUMRUN; ++r)); do
			printf " ################## RUN[%d] #################" $r
			launchScenarios
		done
		exit 1
		;;
	"-h")
		print_help
		;;

esac



for s in $AMD_SAMPLES; do
	# Launch sample
	#SAMPLE=${SAMPLES[$SEL]}
	SAMPLE=$s
	SEL=""
	CS=${s:0:1}
	case $CS in
		"N")
			SEL=1
			PVALUES=${NB_PARAMS[@]}
			;;
		"F")
			SEL=2
			;;
		"M")
			SEL=3
			PVALUES=${MC_PARAMS[@]}
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
			OUTFILENAME=$OUTDIR/$DATETIME/"NOBBQ-"$SAMPLE"-N"$i"-I"${NUMITER[$SEL]}"-P0-Runtime.dat"
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
				OUTFILENAME=$OUTDIR/$DATETIME/"NOBBQ-"$SAMPLE"-N"$i"-I"${NUMITER[$SEL]}"-P"$p"-Runtime.dat" 
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
		fi
	done
done




