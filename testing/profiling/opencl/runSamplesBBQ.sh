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
BBQ=1
BOSP_BASE=${BOSP_BASE:-"/home/ccaffarri/BOSP"}
#BOSP_BASE=${BOSP_BASE:-"/opt/BOSP"}
DATETIME=`date +%Y%m%d_%H%M%S`
DATETIME_STR=`date`

OCLPROF="o1"
#OCLPROF=""
NUMRUN=${NUMRUN:-1}
NUMINST=${NUMINST:-"1 2 3 4"}
#NUMINST=${NUMINST:-"4"}
AWM=${AWM:-"0"}
DEVICES=${DEVICES:-"cpu gpu"}
SAMPLE_DIR=${SAMPLE_DIR:-$BOSP_BASE"/out/usr/bin"}
SAMPLE_PREFIX=${SAMPLE_PREFIX:-$SAMPLE_DIR"/bbque-ocl-"}
AMD_SAMPLE_PREFIX="/opt/AMDAPP/samples/opencl/bin/x86_64/"
OUTDIR=${OUTDIR:-$BOSP_BASE"/out/var/ocl"}
OUTFILENAME="BbqueOCLStats"
DATADIR_PREFIX=${DATADIR_PREFIX:-"/tmp"}
SEL=$1

# ========================== Samples data ==================================
ocl_names=(nbody stereomatch fluidsimulation2D montecarlo)
#AMD_SAMPLES="NBody FluidSimulation2D MonteCarloAsian"
#AMD_SAMPLES="MonteCarloAsian"
SAMPLES="nbody stereomatch fluidsimulation2D montecarlo"
#SAMPLES="fluidsimulation2D montecarlo"
#SAMPLES="nbody fluidsimulation2D montecarlo"
#SAMPLES="stereomatching"
ARGS=("" "-x" "--fps" "" "-c")
# Number of particles
#NB_PARAMS=(32768 16384 8192 4096 2048 1024)
NB_PARAMS=(32768)
# Number of steps
#MC_PARAMS=(8 16 32 64 128 256)
MC_PARAMS=(256)
#MC_PARAMS=(64)
# FPS
#SM_PARAMS=(1 3 5 10 15 20)
SM_PARAMS=(20)
# Number of iterations per sample execution
NUMITER=(0 300 100 20 1)
#NUMITER=(0 10 20 90 1)


declare -A cosched_times


function print_help {
	echo "#################################"
	echo "#        Select sample!         #"
	echo "#===============================#"
	echo "# 0) fluidsimulation2D          #"
	echo "# 1) nbody                      #"
	echo "# 2) montecarlo                 #"
	echo "# 3) mandelbrot                 #"
	echo "# 4) stereomatch                #"
	echo "#################################"
}

function clean_out {
	rm /tmp/$OUTFILENAME.log
}

function setup {
	# Block GPUs frequency
	(amdconfig --adapter=all --od-setclocks=300,400)

	# Set cpufreq governor to 'performance'
	echo "cpufreq governors:"
	for i in `seq 0 7`;do
		cpu_gov=$(cat /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor)
		echo "cpu"$i": "$cpu_gov
		if [ $cpuf_gov ! "performance" ]; then
			echo "Please set the cpufreq governor to performance!"
			exit
		fi
	done

	case $1 in
		1)	# No OpenCL command profiling!
			unset BBQUE_RTLIB_OPTS
			;;
		"-h")
			print_help
			;;
		*)
			BBQUE_RTLIB_OPTS="o1"
	esac
}

sample_cmdline=(
"$SAMPLE_PREFIX""nbody -i 500000 -x 32768 -q"
"$SAMPLE_PREFIX""stereomatch -i 1500 --path_img_ref /home/ccaffarri/BOSP/contrib/ocl-samples/StereoMatching/tsukuba --fps 20"
"$SAMPLE_PREFIX""fluidsimulation2D -q -i 80"
"$SAMPLE_PREFIX""montecarlo -i 1 -c 256")

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
		eval `BBQUE_RTLIB_OPTS="o1" $1 -i ${NUMITER[$SEL]} \
		--path_img_ref $BOSP_BASE/contrib/ocl-samples/StereoMatching/tsukuba $3 $4 &`
	done
	wait
}

function launchScenarios {
	declare -A cosched_times
	echo "...Run scenarios..."
	for i in `seq 0 3`; do
		for j in `seq 0 3`; do
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
	for i in `seq 0 3`; do
		for j in `seq 0 3`; do
			printf "\t%d" ${cosched_times[$i,$j]}
		done
		printf "\n"
	done
	echo ${cosched_times[@]}
}

function startSamples {
	launch $1 $2 > /dev/null 2>1 &
	sleep 2
	launch $2 $1 > /dev/null 2>1 &
	wait
}

function launch {
	echo ":::: Launching: " ${sample_cmdline[$1]}
	local start_t=0
	local end_t=0
	local diff_t=0
	start_t=$(date +%s)
	eval ${sample_cmdline[$1]}
	end_t=$(date +%s)
	diff_t=$((end_t-start_t))
	cosched_times[$1,$2]=$diff_t
	echo "In matrix ->" ${cosched_times[$1,$2]}
	echo ${cosched_times[$1,$2]}" " >> \
		$TESTDIR"/GPUcs-"${ocl_names[$1]}"_"${ocl_names[$2]}".dat"

}

## Start ##
setup $1

# Create output directory
if [ ! -d "$OUTDIR" ]; then
	mkdir $OUTDIR
fi
TESTDIR=$OUTDIR/$DATETIME
mkdir $TESTDIR


###################### Scenario based experiments #############################

case $1 in
	1)	# Evaluate the GPU co-scheduling combinations
		launchScenarios
		exit 1
		;;
	"-h")
		print_help
		;;

esac

###############################################################################


####################### Samples explorations ##################################


if [ $BBQ = 0 ]; then
	echo "*************** Test WITHOUT The BarbequeRTRM *******************"
	SAMPLE_PREFIX=$AMD_SAMPLE_DIR
	SAMPLES=$AMD_SAMPLES
fi


for s in $SAMPLES; do
	# Launch sample
	#SAMPLE=${SAMPLES[$SEL]}
	SAMPLE=$s
	SEL=""
	CS=${s:0:1}
	case $CS in
		"n")	#NBody
			SEL=1
			PVALUES=${NB_PARAMS[@]}
			;;
		"s")	#Stereomatching
			SEL=2
			PVALUES=${SM_PARAMS[@]}
			;;
		"f")	#Fluidsimulation2D
			SEL=3
			;;
		"m")	#Montecarlo
			SEL=4
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
		# Remove BBQ generated log file
		clean_out

		# --- FluidSimulation2D --- #
		if [ $SEL == 3 ]; then
			printf "[%s] No application parameters\n" $SAMPLE
			for ((r=1; r <=$NUMRUN; ++r)); do
				p=0
				printf "[%s] [nI=%d]************ RUN %d *********************\n" $SAMPLE $i $r
				echo $SAMPLE_PREFIX$SAMPLE -q -i ${NUMITER[$SEL]}
				START=$(date +%s)
				(run_sample $SAMPLE_PREFIX$SAMPLE $i) 2>&1 |./getAdapterInfo.awk
				END=$(date +%s)
				DIFF=$((END-START))
				printf "Time: %d s\n" $DIFF  >> /tmp/$OUTFILENAME.log
				printf "Param: %d\n" $p      >> /tmp/$OUTFILENAME.log
				sleep 3
			done
			printf "Finish\n\n" >> /tmp/$OUTFILENAME.log
		# --- Stereomatching --- #
		elif [ $SEL == 2 ]; then
			for p in $PVALUES; do
				printf "[%s] Parameter value = %d\n" $SAMPLE $p
				echo $BOSP_BASE"/contrib/ocl-samples/StereoMatching/tsukuba"
				for ((r=1; r <=$NUMRUN; ++r)); do
					printf "[%s] [nI=%d]************ RUN %d *********************\n" $SAMPLE $i $r
					echo $SAMPLE_PREFIX$SAMPLE -i ${NUMITER[$SEL]} ${ARGS[$SEL]} $p
					START=$(date +%s)
					(run_stereomatch $SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]} $p) 2>&1 |./getAdapterInfo.awk
					END=$(date +%s)
					DIFF=$((END-START))
					printf "Time: %d s\n" $DIFF >> /tmp/$OUTFILENAME.log
					printf "Param: %d\n" $p     >> /tmp/$OUTFILENAME.log
					sleep 3
				done
				printf "Finish\n\n" >> /tmp/$OUTFILENAME.log
			done
		else
		# --- Nbody, Montecarlo --- #
			for p in $PVALUES; do
				printf "[%s] Parameter value = %d\n" $SAMPLE $p
				for ((r=1; r <=$NUMRUN; ++r)); do
					printf "[%s] [nI=%d]********* RUN %d *********************\n" $SAMPLE $i $r
					echo $SAMPLE_PREFIX$SAMPLE -q -i ${NUMITER[$SEL]} ${ARGS[$SEL]} $p
					START=$(date +%s)
					(run_sample $SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]} $p) 2>&1 |./getAdapterInfo.awk
					END=$(date +%s)
					DIFF=$((END-START))
					printf "Time: %d s\n" $DIFF >> /tmp/$OUTFILENAME.log
					printf "Param: %d\n" $p     >> /tmp/$OUTFILENAME.log
					sleep 3
				done
				printf "Finish\n\n" >> /tmp/$OUTFILENAME.log
			done
		fi
		# Extract data
		awk -v outfile=$OUTDIR/$DATETIME/"BBQ-"$SAMPLE"-N"$i"-P" -f extractData.awk /tmp/$OUTFILENAME.log
		awk -v outfile=$OUTDIR/$DATETIME/"BBQ-"$SAMPLE"-N"$i"-I"${NUMITER[$SEL]}"-P" -f extractData.awk /tmp/$OUTFILENAME.log
	done
done



