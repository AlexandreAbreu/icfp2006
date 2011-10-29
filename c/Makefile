cc = gcc
cflags = -fnested-functions -g

objects = debugger/debugger.o debugger/parser.o icfp.o um.o

.c.o:
	$(cc) $(cflags) -c $< -o $@

all: $(objects)
	$(cc) -o icfp $(objects)

clean:
	rm icfp $(objects)


