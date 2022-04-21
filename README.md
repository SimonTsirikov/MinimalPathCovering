# MinimalPathCovering

## Requirements
Install llvm package and add it to LDFLAGS.

Run with `make`.

Remove garbage files with `make clean`.

## Weighted Edges
```
clang -Wno-implicit-function-declaration -emit-llvm -fno-discard-value-names  -g -S <program>.c -o <program>.ll
clang -fprofile-instr-generate -fcoverage-mapping <program>.c -o <program>
LLVM_PROFILE_FILE="<name>.profraw" ./<program> 
llvm-profdata merge -sparse <name1>.profraw ... <nameN>.profraw -o <name>.profdata  
llvm-cov show ./<program> -instr-profile=<name>.profdata --show-branches=count > <name>.prof
./main <program.ll> <name.prof>
```