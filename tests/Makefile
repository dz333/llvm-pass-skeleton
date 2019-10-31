TESTS := $(shell find . -name "*.c")
LL := $(TESTS:.c=.ll)
OPT := $(LL:.ll=.opt)
PASSLIB := ../build/gep/libGepPass.so

test: $(OPT)

%.ll: %.c
	clang -S -emit-llvm $<

%.opt: %.ll
	opt -load $(PASSLIB) --gepsafe $< -S -o $@ 2> $@.txt

clean: phony
	rm -f *.ll *.opt *.opt.txt *~

.PHONY: phony