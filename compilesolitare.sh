#!/bin/bash
#compile script for solitare
#compiles solitare and creates executable named 'solitare'
#defaults to using gcc, but uses clang if user supplies '-c' or '--clang' as argument to script. 
#use argumnet --help to bring up help info

#prints the help message to terminal.
printHelp() {
    printf "compile script for solitare\n"
    printf "compiles solitare.c and creates executable names 'solitare'\n"
    printf "defaults to using gcc, but uses clang if user supplies '-c' to script.\n\n"
    printf "options:\n"
    printf " -c   --clang   use clang instead of gcc\n"
    printf "-help  --help   prints this page then exits\n"
}

#checks if GCC is installed, and if so it compiles the game with given flags.  if not installed then gives error message and exits.
useGCC() {
    (gcc -v >/dev/null 2>&1 && 
    printf "using GCC\n" &&
    gcc solitare.c -orogueclone -Wall -Wextra -O2  -finline-functions -Wswitch-enum -lncurses -std=gnu11 -osolitare ) ||
    printf "error, gcc seems to not be installed.\n"
}

#compiles game using Clang
useClang() {
    printf "using Clang\n"
    clang solitare.c -Wall -Wextra -O2 -lncurses -finline-functions -Wswitch-enum -std=gnu11 -osolitare   
}

main() {
    case "$1" in
        -help | --help)
            printHelp && exit
            ;;
        -c | --clang)
            (clang -v >/dev/null 2>&1 && useClang) || (printf "falling back to GCC.\n" && useGCC)
            ;;
        *)
            useGCC
            ;;
    esac
}

main "$1"
