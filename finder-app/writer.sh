#!/usr/bin/env bash

# Colorado at Boulder ECEN 5713 Advanced Embedded Software Development
# Assignment 1

# Accepts the following arguments: the first argument is a full path to
# a file (including filename) on the filesystem, referred to below as
# writefile; the second argument is a text string which will be written
# within this file, referred to below as writestr
#
# Exits with value 1 error and print statements if any of the arguments
# above were not specified
#
# Creates a new file with name and path writefile with content writestr,
# overwriting any existing file and creating the path if it doesn’t
# exist. Exits with value 1 and error print statement if the file could
# not be created.

if [ "$#" -ne 2 ]; then
	echo "Usage: $(basename "$0") writefile writestr"
	exit 1
fi

writefile=$1
writestr=$2

writedir=$(dirname "$writefile")

if ! mkdir -p "$writedir"; then
	exit 1
fi

if ! echo "$writestr" > "$writefile"; then
	echo "cannot write file $writefile"
	exit 1
fi
