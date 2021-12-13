

CFLAGS += -I /opt/local/include


LDFLAGS += -L /opt/local/lib -lavformat
LDFLAGS += -L /opt/local/lib -lSDL2

CC = gcc
LD = gcc

OBJS += main.o
OBJS += hello.o

all: $(OBJS)
	@echo "linking..."
	@$(LD) -o demo $< $(LDFLAGS)


%.o: %.c
	@echo "cc $<..."
	@$(CC) -c $< -o $@ $(CFLAGS)

clean:
	@rm -rf *.o
	@echo "clean"