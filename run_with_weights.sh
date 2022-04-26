#!/bin/bash

cd tests
IN='*.c'

for in in $IN
do
	printf "${in}\n"
	name="`basename $in .c`" 
	ll="${name}.ll"
	obj="${name}.o"
	profraw="${name}.profraw"
	profdata="${name}.profdata"
	prof="${name}.prof"
	
    clang -Wno-implicit-function-declaration -emit-llvm -fno-discard-value-names  -g -S $in -o $ll
    clang -fprofile-instr-generate -fcoverage-mapping $in -o $name
    LLVM_PROFILE_FILE=$profraw ./$name
	llvm-profdata merge -sparse $profraw $profraw -o $profdata
	llvm-cov show $name -instr-profile=$profdata --show-branches=count > $prof
	rm -rf $name
	cd ../
	./main tests/$ll tests/$prof
	cd tests
done

