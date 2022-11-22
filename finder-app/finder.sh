#!/usr/bin/env bash

# Colorado at Boulder ECEN 5713 Advanced Embedded Software Development
# Assignment 1

# Accepts the following runtime arguments: the first argument is a path
# to a directory on the filesystem, referred to below as filesdir; the
# second argument is a text string which will be searched within these
# files, referred to below as searchstr
#
# Exits with return value 1 error and print statements if any of the
# parameters above were not specified
#
# Exits with return value 1 error and print statements if filesdir does
# not represent a directory on the filesystem
#
# Prints a message "The number of files are X and the number of matching
# lines are Y" where X is the number of files in the directory and all
# subdirectories and Y is the number of matching lines found in
# respective files.

if [ "$#" -ne 2 ]; then
	echo "Usage: $(basename "$0") filesdir searchstr"
	exit 1
fi

filesdir=$1

if [ ! -d "$filesdir" ]; then
	echo "$filesdir is not a directory"
	exit 1
fi

searchstr=$2

number_of_matching_files=$(ls -1 "$filesdir" | wc -l)
number_of_matching_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

echo "The number of files are $number_of_matching_files and the number of matching lines are $number_of_matching_lines"
