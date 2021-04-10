#!/bin/bash

# Be sure that this file has execution permissions:
# Use the nautilus explorer or chmod +x run_vad.sh
# for frames_mv in 4; do
# for frames_ms in 13; do
# for num_init in 10; do
# for alpha1 in 1.7; do
# for alpha2 in 5.48; do
# Write here the name and path of your program and database
DB=/Users/sergi24sanchez/PAV/P2/db.v4
CMD="bin/vad"

#echo "Parameters: MV: $frames_mv MS: $frames_ms     N: $num_init A1: $alpha1 A2: $alpha2"

for filewav in $DB/*/*wav; do
#    echo
    echo "**************** $filewav ****************"
    if [[ ! -f $filewav ]]; then 
	    echo "Wav file not found: $filewav" >&2
	    exit 1
    fi

    filevad=${filewav/.wav/.vad}

    $CMD -i $filewav -o $filevad || exit 1

# Alternatively, uncomment to create output wave files
#    filewavOut=${filewav/.wav/.vad.wav}
#    $CMD $filewav $filevad $filewavOut || exit 1

done

scripts/vad_evaluation.pl $DB/*/*lab
exit 0
