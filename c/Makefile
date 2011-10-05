cc = gcc
cflags = -fnested-functions -g

objects = debugger/debugger.o icfp.o um.o

.c.o:
	$(cc) $(cflags) -c $< -o $@

all: $(objects)
	$(cc) -o icfp $(objects)

clean:
	rm icfp $(objects)


