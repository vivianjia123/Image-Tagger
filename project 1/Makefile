CC = gcc
CFLAGS = -std=c99 -O3 -Wall -Wpedantic
EXE = image_tagger
OBJ = image_tagger.o http.o dict.o

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

.o%: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm server
