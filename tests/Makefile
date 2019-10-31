TESTS := $(shell find . -name "*.c")
LL := $(TESTS:.c=.ll)
OPT := $(LL:.ll=.opt)
PASSLIB := ../build/gep/libGepPass.so
CLANG_OPT := -Xclang -disable-O0-optnone -S -emit-llvm
test: $(OPT)

%.ll: %.c
	clang $(CLANG_OPT) $<

%.opt: %.ll
	opt -load $(PASSLIB) --gepsafe $< -S -o $@ 2> $@.txt

clean: phony
	rm -f *.ll *.opt *.opt.txt *~

.PHONY: phony