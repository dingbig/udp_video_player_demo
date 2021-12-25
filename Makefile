

CFLAGS += -I /opt/local/include


LDFLAGS += -L /opt/local/lib -lavformat -lavcodec -lavutil -lswscale
LDFLAGS += -L /opt/local/lib -lSDL2

CC = gcc
LD = gcc

all: TCPVideoPlayer


UDPVideoPlayer: udp_video_player.o
	@echo "linking $<"
	@$(LD) -o demo $^ $(LDFLAGS)

TCPVideoPlayer: tcp_video_player.o ffmpeg_decoder_sdl.o
	@echo "linking $<"
	@$(LD) -o demo $^ $(LDFLAGS)

%.o: %.c 
	@echo "cc $<..."
	@$(CC) -c $^ -o $@ $(CFLAGS)

clean:
	@rm -rf *.o demo
	@echo "clean finish"