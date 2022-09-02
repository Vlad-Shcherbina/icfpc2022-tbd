#!/usr/bin/env bash

onwards="true"

for x in {1..100}; do
    read -rt0.2
    wget "https://cdn.robovinci.xyz/imageframes/$x.png" -O "data/problems/$x.png" || onwards="";
    [ -z "$onwards" ] && rm "data/problems/$x.png" && break
done
