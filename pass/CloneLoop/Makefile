CXXFLAGS=-std=c++11 $(shell llvm-config --cxxflags) -I$(BOOST_DIR)/include

#LDFLAGS=$(shell llvm-config --ldflags --system-libs --libs)
#-fno-rtti
#opt -load ./LoopSplit.so -split-loop sample.ll -S > /dev/null

all:
	@echo making binary...
	@clang++ -c $(CXXFLAGS) main.cpp -o main.o $(LDFLAGS)
	@clang++ -shared -fPIC main.o -o LoopSplit.so
	@echo build done

src: sample.c
	clang -cc1 -std=c99 -O2 -S -emit-llvm sample.c -o sample.ll

try: src
	opt -load ./LoopSplit.so -S -split-loop sample.ll -o output.ll
	llc -filetype=obj output.ll -o foo.o
	clang -c sample_main.c -o main.o
	clang main.o foo.o -o out

clean:
	@rm -f *.so *.o *.ll *.s
