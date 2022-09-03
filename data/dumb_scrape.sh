#!/usr/bin/env bash

onwards="true"

for x in {1..1000}; do
    read -rt0.2
    wget "https://cdn.robovinci.xyz/imageframes/$x.png" -O "data/problems/$x.png" || onwards="";
    [ -z "$onwards" ] && rm "data/problems/$x.png" && break
    if [[ $x -gt 25 ]] ; then
        wget "https://cdn.robovinci.xyz/imageframes/$x.initial.json" -O "data/problems/$x.initial.json"
        wget "https://cdn.robovinci.xyz/imageframes/$x.initial.png" -O "data/problems/$x.initial.png"
    fi
done
