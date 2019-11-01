TESTS := $(shell find . -name "*.c")
LL := $(TESTS:.c=.ll)
OPT := $(LL:.ll=.opt)
EXEC := $(LL:.ll=.o)
PASSLIB := ../build/gep/libGepPass.so
OPTLVL := 0
CLANG_OPT := -O$(OPTLVL)  -Xclang  -disable-O0-optnone -S -emit-llvm


test: $(OPT)

exec: $(EXEC)

%.ll: %.c
	clang $(CLANG_OPT) $<

%.opt: %.ll $(PASSLIB)
	opt -load $(PASSLIB) --gepsafe $< -S -o $@ 2> $@.txt

%.s: %.opt
	llc -o $@ $<

%.exec: %.s
	gcc -o $@ $<

clean: phony
	rm -f *.ll *.opt *.opt.txt *~ *.s *.o

.PHONY: phony