#!/bin/bash

PWD=`pwd`
DIR=~/check
pushd $DIR
declare -A at_runs
at_runs[Ca48]="6344-6348,6354-6357,6380-6385,6405-6408"
at_runs[Ca40]="6349-6352,6394-6396,6398-6404"
at_runs[C]="6359-6363,6386-6391"
at_runs[Pb]="6367-6378"

for target in ${!at_runs[*]}
do
    for IHWP in IN OUT; 
    do
	TMP=$target.tmp
	bin/checkmini -c conf/checkmini_at.conf -S -r ${at_runs[$target]} -i $IHWP -n checkmini_${target}_$IHWP  > $TMP
	for var in asym_us_dd reg_asym_us_dd dit_asym_us_dd
	do
	    mean=$(grep OUTPUT $TMP | grep "\<$var.mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
	    err=$(grep OUTPUT $TMP | grep "\<$var.mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f2)
	    rms=$(grep OUTPUT $TMP | grep "\<$var.rms\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
	    printf "minirun:\t$target\t$IHWP\t$var\t\t%.2f ± %.2f\t%.2f\n" $mean $err $rms | tee -a at.out
	done
	bin/mulplot -c conf/mul_at.conf -l -r ${at_runs[$target]} -i $IHWP -n mulplot_${target}_$IHWP  > $TMP
	for var in asym_us_dd reg_asym_us_dd dit_asym_us_dd
	do
	    mean=$(grep OUTPUT $TMP | grep "\<$var--mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
	    mean=$(perl -e "print $mean*1000")
	    err=$(grep OUTPUT $TMP | grep "\<$var--mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f2)
	    err=$(perl -e "print $err*1000")
	    rms=$(grep OUTPUT $TMP | grep "\<$var--rms\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
	    printf "mulplot:\t$target\t$IHWP\t$var\t\t%.2f ± %.2f\t%.2f\n" $mean $err $rms | tee -a at.out
	done
	rm $TMP
    done

    TMP=$target.tmp
    bin/checkmini -c conf/checkmini_at.conf -S -r ${at_runs[$target]} -n checkmini_${target} > $TMP
    for var in asym_us_dd reg_asym_us_dd dit_asym_us_dd
    do
	mean=$(grep OUTPUT $TMP | grep "\<$var.mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
	err=$(grep OUTPUT $TMP | grep "\<$var.mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f2)
	rms=$(grep OUTPUT $TMP | grep "\<$var.rms\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
	printf "minirun:\t$target\tboth\t$var\t\t%.2f ± %.2f\t%.2f\n" $mean $err $rms | tee -a at.out
    done
    bin/mulplot -c conf/mul_at.conf -l -r ${at_runs[$target]} -n mulplot_${target} > $TMP
    for var in asym_us_dd reg_asym_us_dd dit_asym_us_dd
    do
	mean=$(grep OUTPUT $TMP | grep "\<$var--mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
	mean=$(perl -e "print $mean*1000")
	err=$(grep OUTPUT $TMP | grep "\<$var--mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f2)
	err=$(perl -e "print $err*1000")
	rms=$(grep OUTPUT $TMP | grep "\<$var--rms\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
	printf "mulplot:\t$target\tboth\t$var\t\t%.2f ± %.2f\t%.2f\n" $mean $err $rms | tee -a at.out
    done
    rm $TMP
done
popd
