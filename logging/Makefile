all: ../build/logging/lib/.stamp ../build/logging/include/.stamp

../build/logging/lib/.stamp: ../build/logging/lib/logging.a
touch $@

../build/logging/lib/logging.a: ../build/logging/obj/src/placeholder.o 
	ar rcs $@ $^

../build/logging/obj/src/placeholder.o: src/placeholder.c include/smallrag/logging.h 
	gcc -c -Iinclude  -o $@ src/placeholder.c

../build/logging/include/.stamp: include/smallrag/logging.h 
	cp -R include/. ../build/logging/include
	touch $@

