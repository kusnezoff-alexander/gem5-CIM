#!/bin/bash

# Compile each .c file in the current directory
for file in *.c; do
    # Extract filename without extension
    filename=$(basename -- "$file")
    filename="${filename%.*}"

    # Compile the .c file into an executable binary with the same name
	gcc -static -std=c99 -O3 -msse2 -I ../gem5/util/m5 m5op_x86.S rowop_gem5-CIM.S "$file" -o "${filename}_gem5-CIM.exe"
done
