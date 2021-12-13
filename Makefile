

CFLAGS =+ -I /opt/local/include


LDFLAGS += -L /opt/local/lib -lavformat
LDFLAGS += -L /opt/local/lib -lSDL2



all: 
	gcc -o demo main.c hello.c -I $(CFLAGS) $(LDFLAGS)
