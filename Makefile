.PRECIOUS : tests/%.ll

all : main test

cpp_files := $(wildcard *.cpp)

CXXFLAGS=-O3 -Wall -Wno-unused-value $(shell llvm-config --cxxflags --ldflags --libs --system-libs) -std=c++17

$(cpp_files:%.cpp=%.o) : %.o : %.cpp | .
	$(CXX) $(CXXFLAGS) -c -o $@ $<

main: main.o extract_cfg.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(shell llvm-config --cxxflags --ldflags --libs --system-libs)

test : $(patsubst tests/%.c,do-%,$(wildcard tests/*.c))

tests/%.ll : tests/%.c
	clang -Wno-implicit-function-declaration -emit-llvm -fno-discard-value-names  -g -S $< -o $@

test-rules = do-$(1) : tests/$(1).ll main

$(call test-rules,%)
	./main  $<

clean:
	rm -f *.o main tests/*.ll
