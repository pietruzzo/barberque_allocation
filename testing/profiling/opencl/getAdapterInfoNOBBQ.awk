#!/usr/bin/awk -f

###############################################################################
# Extract GPU status information (Load and temperature)                       #
###############################################################################

function dumpAdapterCounters() {
  # Adapters 0 and 1 metrics reading
  # Example output:
  # Current Clocks : 300 400 GPU load : 0 Sensor 0: Temperature - 36.00 C

  "DISPLAY=:0 aticonfig --odgc --odgt --adapter=0 | egrep -i \"clock|load|temperature\" | xargs echo | sed 's/%//'"|getline load;
  split(load, v0, " ");
  clock_a0 = v0[4];
  load_a0  = v0[9];
  temp_a0 = v0[14];

  "DISPLAY=:0 aticonfig --odgc --odgt --adapter=1 | egrep -i \"clock|load|temperature\" | xargs echo | sed 's/%//'"|getline load;
  split(load, v1, " ");
  clock_a1 = v1[4];
  load_a1  = v1[9];
  temp_a1 = v1[14];
  printf("%d %d %d %d\n", v0[9], v0[14], v1[9], v1[14]) > "/tmp/OCLSampleRuntime.dat"
}

BEGIN {

}

/^real/ {
  print "\nTime: " $2 >> "/tmp/BbqueOCLStats.log"
}

END {
 dumpAdapterCounters();
 printf("Measured load: A0 %.3f %%, A1 %.3f %%\n", load_a0, load_a1);
}
