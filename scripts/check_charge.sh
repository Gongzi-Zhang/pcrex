#!/bin/bash

####################
# check run charge
####################

if [ $# -eq 0 ]; then
    echo "At least one parameter needed"
    echo "usage: $0 run [run...]"
    exit 4
fi

while [ $# -gt 0 ]; 
do
    run=$1
    job_sub="sub_${run}.sh"
    [ -f $job_sub ] && rm $job_sub
cat << END >> $job_sub
#!/bin/bash
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --cpus-per-task=1
#SBATCH --mem=4G
#SBATCH --output=${run}.out
#SBATCH --error=${run}.err

cd /volatile/halla/parity/weibin/pcrex
bin/runinfo -r $run --charge
END
    sbatch $job_sub

    shift
done
