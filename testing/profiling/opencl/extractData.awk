#!/usr/bin/awk -f

BEGIN {
  printf("%s\n", outfile);
  i=0;
  j=0;
}

/\( / {
  hperc[i][j] = $4 " " $8 " " $18  " " $28 
  vperc[i][j] = $4 " " $9 " " $19  " " $29
  ++j;
}

/^Time/ {
  row[i,1] = $2;
}

/^Adapter/ {
  row[i,2+$2*2] = $8;
  row[i,3+$2*2] = $11;
}

/^Param/ {
  row[i,0] = $2;
  nr_cmds = j;
  ++i;
  j = 0;
}

/^Finish/ {
  j = nr_cmds;
  printf("%s, runs=%d, cmds=%d)\n", outfile, i, j);
  for (i--; i >= 0; --i) {
    printf("%s %d %d %d %d\n", row[i,1], row[i,2], row[i,3], row[i,4], row[i,5]) \
	>> outfile""row[i,0]"-Runtime.dat";

    for (j = nr_cmds-1; j >= 0; --j) {
      split(hperc[i][j], a, " ")
      printf("%s %s %s\n", a[2], a[3], a[4]) >> outfile""row[i,0]"-"a[1]"-H.dat"
      split(vperc[i][j], a, " ")
      printf("%s %s %s\n", a[2], a[3], a[4]) >> outfile""row[i,0]"-"a[1]"-V.dat"
    }
  }
  i=0;
  j=0;
}


END {

}
