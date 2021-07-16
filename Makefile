CCX=C:/Program\ Files/LLVM/bin/clang++.exe
LIB_PATH=
LD=C:/Program\ Files/LLVM/bin/clang++.exe
CFLAGS=-g -Winitializer-overrides -D_CRT_SECURE_NO_WARNINGS -DDEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -IE:\llvm-project\build\include -IE:/llvm-project/llvm/include -DPLATFORM_WINDOWS

all: dsl

out/%.o: src/%.cpp
	mkdir -p out
	$(CCX) -c $< $(CFLAGS) -O0 -o $@  -Isrc -Iout -Illvm/include

dsl: out/main.o out/Parser.o out/Tokenizer.o out/Codegen.o out/Trie.o out/Log.o out/Colors.o
	mkdir -p bin
	$(LD) $^ `./config.sh` -g -O0 -o bin/$@.exe

run: dsl
	bin/dsl.exe

clean:
	rm -r bin out