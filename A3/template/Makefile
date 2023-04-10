CC = gcc
CFLAGS = -g

LIB=lib
SRC=src
TEST_SRC=test-cases
INC=include
LIB_OBJ=lib
OBJ=obj
BENCH_SRC=benchmark-test
BENCH_OBJ=bech-obj


# src = $(wildcard Examples/*.c)

# obj = $(src:.c=.o)

HDRS=$(shell ls $(INC)/*.h)

all: test1 create-dev bench# library #examples

bench: dma dma_interrupt mmap mmap_interrupt mmio mmio_interrupt

create-dev: $(BENCH_SRC)/file_create.c
	$(CC) -o $@ $<

test1: $(OBJ)/test1.o $(LIB)/libcrypter.so
	$(CC) -o $@ -I$(INC) $< -L$(LIB) -lcrypter

dma: $(BENCH_OBJ)/dma.o $(LIB)/libcrypter.so
	$(CC) -o $@ -I$(INC) $< -L$(LIB) -lcrypter

dma_interrupt: $(BENCH_OBJ)/dma_interrupt.o $(LIB)/libcrypter.so
	$(CC) -o $@ -I$(INC) $< -L$(LIB) -lcrypter

mmap: $(BENCH_OBJ)/mmap.o $(LIB)/libcrypter.so
	$(CC) -o $@ -I$(INC) $< -L$(LIB) -lcrypter

mmap_interrupt: $(BENCH_OBJ)/mmap_interrupt.o $(LIB)/libcrypter.so
	$(CC) -o $@ -I$(INC) $< -L$(LIB) -lcrypter

mmio: $(BENCH_OBJ)/mmio.o $(LIB)/libcrypter.so
	$(CC) -o $@ -I$(INC) $< -L$(LIB) -lcrypter

mmio_interrupt: $(BENCH_OBJ)/mmio_interrupt.o $(LIB)/libcrypter.so
	$(CC) -o $@ -I$(INC) $< -L$(LIB) -lcrypter

$(OBJ)/%.o: $(TEST_SRC)/%.c $(HDRS)
	mkdir -p obj
	$(CC) -c -I$(INC) $< -o $@

$(BENCH_OBJ)/%.o: $(BENCH_SRC)/%.c $(HDRS)
	mkdir -p bech-obj
	$(CC) -c -I$(INC) $< -o $@

library: $(LIB)/libcrypter.so

$(LIB)/libcrypter.so: $(LIB_OBJ)/crypter.o
	gcc -shared -o $@ $^

$(LIB_OBJ)/%.o: $(SRC)/%.c $(HDRS)
	mkdir -p lib
	$(CC) -fPIC -c -I$(INC) $< -o $@

%-pa-cs614.tar.gz:	clean
	tar cf - `find . -type f | grep -v '^\.*$$' | grep -v '/CVS/' | grep -v '/\.svn/' | grep -v '/\.git/' | grep -v '[0-9].*\.tar\.gz' | grep -v '/submit.token$$'` | gzip > $@

.PHONY: prepare-submit
prepare-submit: $(RNO)-pa-cs614.tar.gz

.PHONY: clean
clean:
	rm -rf test1 create-dev dma dma_interrupt mmap mmap_interrupt mmio mmio_interrupt $(OBJ) $(LIB) $(BENCH_OBJ)
