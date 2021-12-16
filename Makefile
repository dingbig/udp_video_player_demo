

CFLAGS += -I /opt/local/include


LDFLAGS += -L /opt/local/lib -lavformat -lavcodec -lavutil -lswscale
LDFLAGS += -L /opt/local/lib -lSDL2

CC = gcc
LD = gcc

# CFILENAME = hellosdlimg
# CFILENAME = videoplayer
CFILENAME = udp_video_player

all: $(CFILENAME).o
	@echo "linking..."
	@$(LD) -o demo $< $(LDFLAGS)

$(CFILENAME).o: $(CFILENAME).c 
	@echo "cc $<..."
	@$(CC) -c $^ -o $@ $(CFLAGS)

clean:
	@rm -rf *.o demo
	@echo "clean finish"