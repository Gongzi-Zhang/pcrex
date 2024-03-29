#!/bin/bash

export RCDB_TARGET='Carbon 1%,D-208Pb4-D,40Ca,48Ca'

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
    # for IHWP in IN OUT; 
    # do
    #     TMP=$target.tmp
    #     bin/checkmini -c conf/checkmini_at.conf -S -r ${at_runs[$target]} -i $IHWP -n checkmini_${target}_$IHWP  > $TMP
    #     for var in asym_us_dd reg_asym_us_dd dit_asym_us_dd
    #     do
    #         mean=$(grep OUTPUT $TMP | grep "\<$var.mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
    #         err=$(grep OUTPUT $TMP | grep "\<$var.mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f2)
    #         rms=$(grep OUTPUT $TMP | grep "\<$var.rms\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
    #         printf "minirun:\t$target\t$IHWP\t$var\t\t%.2f ± %.2f\t%.2f\n" $mean $err $rms | tee -a at.out
    #     done
    #     bin/mulplot -c conf/mul_at.conf -l -r ${at_runs[$target]} -i $IHWP -n mulplot_${target}_$IHWP  > $TMP
    #     for var in asym_us_dd reg_asym_us_dd dit_asym_us_dd
    #     do
    #         mean=$(grep OUTPUT $TMP | grep "\<$var--mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
    #         mean=$(perl -e "print $mean*1000")
    #         err=$(grep OUTPUT $TMP | grep "\<$var--mean\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f2)
    #         err=$(perl -e "print $err*1000")
    #         rms=$(grep OUTPUT $TMP | grep "\<$var--rms\>" | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
    #         printf "mulplot:\t$target\t$IHWP\t$var\t\t%.2f ± %.2f\t%.2f\n" $mean $err $rms | tee -a at.out
    #     done
    #     rm $TMP
    # done

    TMP=checkmini_$target.out
    conf="conf/checkmini_at.conf"
    # if [ -f "${conf%.conf}_${target}.conf" ]
    # then
    #     conf="${conf%.conf}_${target}.conf"
    # fi
    bin/checkmini -c ${conf} -S -r ${at_runs[$target]} -n checkmini_${target} -f png > $TMP
    # for var in asym_us_dd reg_asym_us_dd dit_asym_us_dd
    for var in diff_reg_dit_asym_us_dd diff_reg_raw_asym_us_dd diff_dit_raw_asym_us_dd
    do
        mean=$(grep OUTPUT $TMP | grep "\<$var.mean\>" | head -n 1 | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
        err=$(grep OUTPUT $TMP | grep "\<$var.mean\>" | head -n 1 | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f2)
        rms=$(grep OUTPUT $TMP | grep "\<$var.rms\>" | head -n 1 | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
        printf "minirun:\t$target\tboth\t$var\t\t%.2f ± %.2f\t%.2f\n" $mean $err $rms | tee -a at.out
    done

    # TMP=mulplot_$target.out
    # conf="conf/mul_at.conf"
    # if [ -f "${conf%.conf}_${target}.conf" ]
    # then
    #     conf="${conf%.conf}_${target}.conf"
    # fi
    # bin/mulplot -c ${conf} -l -r ${at_runs[$target]} -n mulplot_${target} > $TMP
    # # for var in asym_us_dd reg_asym_us_dd dit_asym_us_dd
    # for var in asym_bcm_an_us asym_bcm_an_ds asym_bcm_an_ds3 asym_bcm_dg_us asym_bcm_dg_ds  \
    #     reg_asym_bcm_an_us reg_asym_bcm_an_ds reg_asym_bcm_an_ds3 reg_asym_bcm_dg_us reg_asym_bcm_dg_ds  \
    #     reg_asym_us_avg_diff_bpm1X reg_asym_us_avg_diff_bpm4aY reg_asym_us_avg_diff_bpm4eX reg_asym_us_avg_diff_bpm4eY reg_asym_us_avg_diff_bpm12X  \
    #     reg_asym_us_dd_diff_bpm1X reg_asym_us_dd_diff_bpm4aY reg_asym_us_dd_diff_bpm4eX reg_asym_us_dd_diff_bpm4eY reg_asym_us_dd_diff_bpm12X
    # do
    #     mean=$(grep OUTPUT $TMP | grep "\<$var--mean\>" | head -n 1 | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
    #     mean=$(perl -e "print $mean*1000")
    #     err=$(grep OUTPUT $TMP | grep "\<$var--mean\>" | head -n 1 | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f2)
    #     err=$(perl -e "print $err*1000")
    #     rms=$(grep OUTPUT $TMP | grep "\<$var--rms\>" | head -n 1 | sed -r 's/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[m]//g' | cut -f3 | cut -d'±' -f1)
    #     printf "mulplot:\t$target\tboth\t$var\t\t%.2f ± %.2f\t%.2f\n" $mean $err $rms | tee -a at.out
    # done
done
popd
