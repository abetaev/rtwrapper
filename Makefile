all: rtwrapper.so

CFLAGS = -fPIC -std=c99 -m32
LDFLAGS = -shared -m32
LIBS=-lnetlink -ldl

rtwrapper.so: rtwrapper.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

rtwrapper: rtwrapper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS)

clean:
	$(RM) rtwrapper.o rtwrapper.so
