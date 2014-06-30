cc=gcc
objects=test.o cutil.o ConvertUTF.o ../charset.o ../datatype.o ../filesystem.o ../math.o ../string.o ../test-c.o
flags=-lm -lpthread -liconv

all: $(objects)
	$(cc) -o testc $(flags) $(objects)

test.o: test.c
cutil.o: ../../cutil.h ../../cutil.c
	$(cc) -c -lpthread ../../cutil.h ../../cutil.c
ConvertUTF.o: ../../lib/ConvertUTF.c
	$(cc) -c ../../lib/ConvertUTF.c

clean:
	rm $(objects)
