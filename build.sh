#!/bin/bash

config_option=""
target_option=""

if [ -n "$1" ]; then
    config=$1
fi
if [ -n "$2" ]; then
    target=$2
fi
if [ -e "${config}" ]; then
    config_option="--config ${config}"
fi
if [ -n "${target}" ]; then
    target_option="--target ${target}"
fi

cmake --build build ${config_option} ${target_option}
