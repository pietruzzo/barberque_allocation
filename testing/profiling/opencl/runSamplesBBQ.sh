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

BBQ=1
SAMPLE_DIR=${SAMPLE_DIR:-$BOSP_BASE"/out/usr/bin"}
SAMPLE_PREFIX=${SAMPLE_PREFIX:-$SAMPLE_DIR"/bbque-ocl-"}

declare -A cosched_times

sample_cmdline=(
	"$SAMPLE_PREFIX""nbody -i 500000 -x 32768 -q"
	"$SAMPLE_PREFIX""stereomatch -i 1500 --path_img_ref "$BOSP_BASE"/contrib/ocl-samples/StereoMatching/tsukuba --fps 20"
	"$SAMPLE_PREFIX""fluidsimulation2D -q -i 200"
	"$SAMPLE_PREFIX""montecarlo -i 1 -c 256"
)

# =============================================================

#1: Sample
#2: Number of instances
#3: Parameter flag
#4: Parameter value
function run_sample {
	for i in `seq 1 $2`; do
		printf "Running up to %d instance(s)\n" $2
		(BBQUE_RTLIB_OPTS=$OCLPROF $1 -q -i ${NUMITER[$SEL]} $3 $4 &)
	done
	wait
}

function run_stereomatch {
	for i in `seq 1 $2`; do
		printf "Running up to %d instance(s)\n" $2
	#	echo "BBQUE_RTLIB_OPTS=o1 $1 -i" $NUMITER \
	#		"--path_img_ref "$BOSP_BASE"/contrib/ocl-samples/StereoMatching/tsukuba" $3 $4 > nist
		(BBQUE_RTLIB_OPTS="o1" $1 -i ${NUMITER[$SEL]} \
		--path_img_ref $BOSP_BASE/contrib/ocl-samples/StereoMatching/tsukuba $3 $4 &)
	done
	wait
}

function run_gpu_cosched_scenario {
	declare -A cosched_times
	echo "...Run scenarios..."
	for i in `seq 0 3`; do
		for j in `seq 0 3`; do
			printf "[%s, %s]...\n " ${ocl_names[$i]} ${ocl_names[$j]}
			startSamplePair $i $j
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

function startSamplePair {
	launchBBQSample $1 $2 > /dev/null 2>&1 &
#	launchBBQSample $1 $2  &
	sleep 1
	launchBBQSample $2 $1 > /dev/null 2>&1 &
#	launchBBQSample $2 $1  &
	wait
}

function launchBBQSample {
	echo " BBQ: " ${sample_cmdline[$1]}
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

function print_help {
	echo " : Default execution"
	echo "1: GPU co-scheduling execution"
	echo "2: GPU exclusive execution"
	echo "3: Heterogeneous scenario"
}

function clean_out {
	rm /tmp/$OUTFILENAME.log
}


function run_test {
	SAMPLE=$1
	SEL=$2
	printf "==================== (SEL=%d: %s) ================\n" $SEL $SAMPLE
	printf "Parameter set = [%s]\n" $PVALUES

	for i in $NUMINST; do
#		printf "# Instances = %s\n" $i
		# Remove BBQ generated log file
		clean_out

		case $SEL in
		# --- FluidSimulation2D --- #
			"3")
			printf "[%s] No application parameters\n" $SAMPLE
			for ((r=1; r <=$NUMRUN; ++r)); do
				p=0
				print_test_header $SAMPLE $r $i $p
				echo $SAMPLE_PREFIX$SAMPLE -q -i ${NUMITER[$SEL]}
				START=$(date +%s)
				(run_sample $SAMPLE_PREFIX$SAMPLE $i) 2>&1 |./getAdapterInfo.awk
				END=$(date +%s)
				DIFF=$((END-START))
				printf "Time: %d s\n" $DIFF | tee -a /tmp/$OUTFILENAME.log
				printf "Param: %d\n" $p      >> /tmp/$OUTFILENAME.log
				sleep 3
			done
			printf "Finish\n\n" >> /tmp/$OUTFILENAME.log
			;;
		# --- Stereomatching --- #
			"2")
			echo $BOSP_BASE"/contrib/ocl-samples/StereoMatching/tsukuba"
			for ((r=1; r <=$NUMRUN; ++r)); do
				print_test_header $SAMPLE $r $i
				echo $SAMPLE_PREFIX$SAMPLE -i ${NUMITER[$SEL]} ${ARGS[$SEL]}
				START=$(date +%s)
				(run_stereomatch $SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]}) 2>&1 |./getAdapterInfo.awk
				#(run_stereomatch $SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]}) |./getAdapterInfo.awk
				END=$(date +%s)
				DIFF=$((END-START))
				printf "Time: %d s\n" $DIFF | tee -a /tmp/$OUTFILENAME.log
				printf "Param: %d\n"      >> /tmp/$OUTFILENAME.log
				sleep 2
			done
			printf "Finish\n\n" >> /tmp/$OUTFILENAME.log
			;;
			"1" | "4")
		# --- Nbody, Montecarlo --- #
			for p in $PVALUES; do
				for ((r=1; r <=$NUMRUN; ++r)); do
					print_test_header $SAMPLE $r $i $p
					echo $SAMPLE_PREFIX$SAMPLE -q -i ${NUMITER[$SEL]} ${ARGS[$SEL]} $p
					START=$(date +%s)
					(run_sample $SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]} $p) 2>&1 |./getAdapterInfo.awk
					#(run_sample $SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]} $p) |./getAdapterInfo.awk
					END=$(date +%s)
					DIFF=$((END-START))
					#printf "Time: %d s\n" $DIFF >> /tmp/$OUTFILENAME.log
					printf "Time: %d s\n" $DIFF | tee -a /tmp/$OUTFILENAME.log
					printf "Param: %d\n" $p     >> /tmp/$OUTFILENAME.log
					sleep 3
				done
				printf "Finish\n\n" >> /tmp/$OUTFILENAME.log
			done
			;;
		# --- Mandelbrot --- #
			"5")
			for ((r=1; r <=$NUMRUN; ++r)); do
				print_test_header $SAMPLE $r $i
				echo $SAMPLE_PREFIX$SAMPLE -q -i ${NUMITER[$SEL]} ${ARGS[$SEL]} --double
				START=$(date +%s)
				(run_sample $SAMPLE_PREFIX$SAMPLE $i ${ARGS[$SEL]} --double) 2>&1 |./getAdapterInfo.awk
				END=$(date +%s)
				DIFF=$((END-START))
				printf "Time: %d s\n" $DIFF | tee -a /tmp/$OUTFILENAME.log
				printf "Param: %d\n"  >> /tmp/$OUTFILENAME.log
				sleep 2
			done
			printf "Finish\n\n" >> /tmp/$OUTFILENAME.log
			;;
		esac

		# Extract data
		awk -v outfile=$OUTDIR/$DATETIME/"BBQ-"$SAMPLE"-N"$i"-P" -f extractData.awk /tmp/$OUTFILENAME.log
		awk -v outfile=$OUTDIR/$DATETIME/"BBQ-"$SAMPLE"-N"$i"-I"${NUMITER[$SEL]}"-P" -f extractData.awk /tmp/$OUTFILENAME.log



###################### Start the test #########################################

# BBQ or not BBQ?
if [ $BBQ == 0 ]; then
	echo "*************** Test WITHOUT The BarbequeRTRM *******************"
	SAMPLE_PREFIX=$AMD_SAMPLE_DIR
	SAMPLES=$AMD_SAMPLES
fi
if [ $BBQ == 1 ] && [ ! -f $BOSP_BASE/out/var/run/bbqued.pid ]; then
	echo "ERROR: BarbequeRTRM not running!"
	exit 2
fi

# Import setup data
setup $1

# Type of experiment (recipes setup)
case $1 in
	1)	# Evaluate the GPU co-scheduling combinations
		run_gpu_cosched_scenario
		exit 1
		;;
	0 | 2 | 3)
		;;
	*)
		print_help
		;;

esac

# Samples set iteration
for s in $SAMPLES; do
	SAMPLE=$s
	SEL=""
	CS=${s:0:2}
	case $CS in
		"nb")	#NBody
			SEL=1
			PVALUES=${NB_PARAMS[@]}
			;;
		"st")	#Stereomatching
			SEL=2
			;;
		"fl")	#Fluidsimulation2D
			SEL=3
			;;
		"mo")	#Montecarlo
			SEL=4
			PVALUES=${MC_PARAMS[@]}
			;;
		"ma")	#Mandelbrot
			SEL=5
			PVALUES=${MA_PARAMS[@]}
			;;
		*)
			SEL=0
			printf "No parameters for %s\n" $s
			;;
	esac
	done
done


