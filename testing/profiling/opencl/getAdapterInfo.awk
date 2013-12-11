#!/usr/bin/awk -f

###############################################################################
# Extract GPU status information (Load and temperature)                       #
###############################################################################

function dumpAdapterCounters() {
  # Adapters 0 and 1 metrics reading
  # Example output:
  # Current Clocks : 300 400 GPU load : 0 Sensor 0: Temperature - 36.00 C

  "DISPLAY=:0 aticonfig --odgc --odgt --adapter=0 | egrep -i \"clock|load|temperature\" | xargs echo | sed 's/%//'"|getline load;
  split(load, v, " ");
  clock_a0 = v[4];
  load_a0  = v[9];
  temp_a0 = v[14];
  printf("Adapter 0 - Clock: %d MHz, Load: %d %%, Temp: %d C\n", v[4], v[9], v[14]) >> "/tmp/BbqueOCLStats.log"

  "DISPLAY=:0 aticonfig --odgc --odgt --adapter=1 | egrep -i \"clock|load|temperature\" | xargs echo | sed 's/%//'"|getline load;
  split(load, v, " ");
  clock_a1 = v[4];
  load_a1  = v[9];
  temp_a1 = v[14];
  printf("Adapter 1 - Clock: %d MHz, Load: %d %%, Temp: %d C\n", v[4], v[9], v[14]) >> "/tmp/BbqueOCLStats.log"
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
