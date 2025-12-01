#!/bin/bash

#./run_caida16.sh
#wait
#./run_caida19.sh
#wait
#./run_mawi24.sh
#wait
#./run_mawi25.sh
#wait
# ./run_threshold_card.sh
# wait
# ./run_threshold_k.sh
# wait
# ./params.sh
# wait
./run_caida16_time_32KB.sh
wait
./run_caida16_time_128KB.sh
wait
./run_caida16_time_512KB.sh
wait
./run_caida19_time_32KB.sh
wait
./run_caida19_time_128KB.sh
wait
./run_caida19_time_512KB.sh
wait
./run_mawi24_time_32KB.sh
wait
./run_mawi24_time_128KB.sh
wait
./run_mawi24_time_512KB.sh
wait
./run_mawi25_time_32KB.sh
wait
./run_mawi25_time_128KB.sh
wait
./run_mawi25_time_512KB.sh