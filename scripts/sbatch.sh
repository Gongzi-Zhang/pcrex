#!/bin/bash

####################
# submitting jobs to grid 
####################

if [ $# -ne 1 ]; then
    echo "Incorrected # of paraters"
    echo "usage: $0 #_of_jobs"
    exit 4
fi

slugs=94
jobs=$1
let slugs_per_job=slugs/jobs
let e_slug=0
for ((job=1; job<=$jobs; job++)); do
    job_sub="sub_${job}.sh"
    [ -f $job_sub ] && rm $job_sub
    let b_slug=e_slug+1
    let e_slug=b_slug+slugs_per_job
    cat << END >> $job_sub
#!/bin/bash
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --cpus-per-task=1
#SBATCH --mem=4G
#SBATCH --output=/home/weibin/check/aggregate_%A_%a.out
#SBATCH --error=/home/weibin/check/aggregate_%A_%a.err

cd /home/weibin/check
/home/weibin/check/aggregate -c aggregate_1.conf -a no -s $b_slug-$e_slug
END
    sbatch $job_sub
done


#SBATCH --partition=production
#SBATCH --job-name=prex-respin2-rerun1
#SBATCH --time=24:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=4G
#SBATCH --output=/volatile/halla/parity/prex-respin2/tmp/prex-respin2-rerun1_%A_%a.out
#SBATCH --error=/volatile/halla/parity/prex-respin2/tmp/prex-respin2-rerun1_%A_%a.err
