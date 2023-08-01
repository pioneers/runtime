#!/bin/bash

# script to run formatting checks on runtime
# assume pwd is $HOME/runtime

# function to remove output files
function clean_up {
    rm -f clang_suggestions.txt
    rm -f diff_output.txt
}

# make sure to remove output.txt if Ctrl-C is pressed
trap 'clean_up' INT

# get a list of all runtime files that we could want to check
# i.e. all .c, .cpp, and .h files
ALL_RUNTIME_FILES=$(find -name "*.c" -o -name "*.cpp" -o -name "*.h")

# remove unwanted files
for file in $ALL_RUNTIME_FILES; do
    if [[ $file == *"net_handler/pbc_gen"* ]]; then
        continue
    elif [[ $file == *"lowcar/lib"* ]]; then
        continue
    elif [[ $file == *"executor/studentapi.c"* ]]; then
        continue
    fi
    FILES_TO_CHECK+="$file"$'\n' # append a newline to the end of each file name
done

# at this point, $FILES_TO_CHECK has a list of files, separated by newlines,
# of files that we want to run format checks on

exit_code=0 # set default exit code (0 = passing)

# depending on argument, either check all files and fail if not passing, or fix all the files
if [[ $1 == "check" ]]; then
    # for each file in list of files, run clang format and dump the output to a file
    # then, run diff to obtain a diff between the output and the actual file
    # if there is anything in the diff, the file did not pass the format check, so we set
    # the exit code to 1
    for file in $FILES_TO_CHECK; do
        clang-format --style=file $file > clang_suggestions.txt
        diff -u $file clang_suggestions.txt > diff_output.txt
        cat diff_output.txt # dumps diff output to stdout

        # sets exit code to 1 if there is a diff
        if [[ $(cat diff_output.txt) != "" ]]; then
            exit_code=1
        fi
    done
elif [[ $1 == "fix" ]]; then
    # fixes the errors in all the files
    clang-format --style=file -i $FILES_TO_CHECK
else
    printf "Specify one of \"check\" or \"fix\" as first and only argument! Exiting...\n"
    exit_code=2
fi

clean_up

exit $exit_code
