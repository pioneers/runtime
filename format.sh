#!/usr/bin/env bash

find . ! -name *tester.c \( -iname *.h -o -iname *.cpp -o -iname *.c -o -iname *.proto \)  | xargs clang-format -i -style=file
