rasm: src/*.c
	gcc -o $@ -g $^

run:
	@./rasm test.s
	@./test.o