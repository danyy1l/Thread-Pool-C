CC=cc
EXE=main
CFLAGS=-Wall -Wextra -std=gnu99 -pedantic -g
CLIBS=-pthread

OBJ=main.o thread_pool.o

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(EXE)

run: all
	./$(EXE)
