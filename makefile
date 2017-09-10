#makefile liblz4f

ifndef ISIZE
ISIZE=-m64
endif

cc=c++
cflags= -c -w $(ISIZE) -O3 -D_REENTRANT -Wno-multichar

%.o : %.cpp
	$(cc) $(cflags) $(incs) $< -o $@

%.o : %.c
	$(cc) $(cflags) $(incs) $< -o $@

o = 				\
	lz4fio.o		\
	lz4/lz4.o		\
	lz4/lz4hc.o		\
	lz4/xxhash.o	\

liblz4f.a: $(o)
	@echo
	@echo making liblz4f for
	@echo $(MACHTYPE)
	@echo $(OSTYPE)
	@echo ...............
	ar rcs liblz4f.a $(o)
	@echo done

test: test.o liblz4f.a
	@echo
	@echo making test for:
	@echo $(MACHTYPE)
	@echo $(OSTYPE)
	@echo ...............
	$(cc) -o test test.o liblz4f.a

clean:
	@echo
	@echo making clean liblz4f
	@echo --------------------
	rm -f *.o *.a test
	rm -f lz4/*.o




