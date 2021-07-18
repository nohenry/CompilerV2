CCX=clang++
LIB_PATH=
LD=clang++
CFLAGS=-g `llvm-config --cxxflags`

all: dsl

out/%.o: src/%.cpp
	mkdir -p out
	$(CCX) -c $< $(CFLAGS) -O0 -o $@  -Isrc -Iout -Illvm/include

dsl: out/main.o out/Parser.o out/Tokenizer.o out/Codegen.o out/Trie.o out/Log.o out/Colors.o
	mkdir -p bin
	$(LD) $^ `llvm-config --cxxflags --ldflags --libs core executionengine analysis native bitwriter --system-libs` -g -O0 -o bin/$@

run: dsl
	bin/dsl

clean:
	rm -r bin out