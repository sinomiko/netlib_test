#!/bin/bash

test_method=$1
packet_size=$2
thread_num=$3

test_method=${test_method:-pingpong}
packet_size=${packet_size:-64}
thread_num=${thread_num:-0}

echo "test_method = $test_method"

./asio_echo_serv --host=127.0.0.1 --port=8090 --mode=echo --test=$test_method --packet-size=$packet_size --thread-num=$thread_num
