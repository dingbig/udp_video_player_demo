//
//  tcp_video_player.c
//  my_xcode
//
//  Created by Owen on 2021/12/22.
//

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

#include "ffmpeg_decoder_sdl.h"

#define PORT 9999

static int recv_all(int sock, uint8_t * buffer, ssize_t size) {
    while (size > 0) {
        ssize_t recv_size = recv(sock, buffer, size, 0);
        if (0 == recv_size) {
            return -1;
        }
        size = size - recv_size;
        buffer += recv_size;
    }
    return 0;
}

static int start_decode(VideoInfo * vInfo) {
    int send_packet_result = avcodec_send_packet(vInfo->pCodecCtx, vInfo->packet);
    if (send_packet_result != 0) {
        printf("avcodec_receive_frame error:%d \n", send_packet_result);
        return -1;;
    }
    
    while (1) {
        int receive_frame_result = avcodec_receive_frame(vInfo->pCodecCtx, vInfo->pFrame);
        
        if (receive_frame_result == 0)
        {
            upload_texture(vInfo);

        } else if (receive_frame_result == AVERROR_EOF) {
            printf("fail receive frame AVERROR_EOF error:%d\n", receive_frame_result);
            break;
        } else if (receive_frame_result == AVERROR(EAGAIN)) {
            printf("fail receive frame AVERROR(EAGAIN) error:%d\n", receive_frame_result);
            break;
        } else {
            printf("fail receive frame error:%d  \n", receive_frame_result);
            break;
        }
    }
    return 0;
}

int main() {
    
    int socketfd;
    struct sockaddr_in servaddr, cliaddr;
    
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (socketfd == -1) {
        printf("Create socket failed! \n");
        return -1;
    } else {
        printf("Socket successfully created..\n");
    }
    
    bzero(&servaddr, sizeof(servaddr));
        
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    
    if (bind(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        printf("socket bind failed...\n");
        return -1;
    } else {
        printf("Socket successfully binded..\n");
    }
    
    if (listen(socketfd, 5) != 0) {
        printf("Listen failed...\n");
        return -1;
    } else {
        printf("Server listening..\n");
    }
    
    socklen_t cli_len = sizeof(cliaddr);
    
    int connfd = accept(socketfd, (struct sockaddr *)&cliaddr, &cli_len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        return -1;
    } else {
        printf("server accept the client connfd:%d\n", connfd);
    }
    
    char msg[] = "恭喜你连接成功";
    write(connfd, msg, sizeof(msg));
    
    struct VideoInfo *vInfo = (VideoInfo *)malloc(sizeof(VideoInfo));
    vInfo->screen_w = 0;
    vInfo->screen_h = 0;
    vInfo->sdl_window = NULL;
    vInfo->sdl_render = NULL;
    vInfo->sdl_texture = NULL;
    vInfo->img_convert_ctx = NULL;

    if (setupDecoder(vInfo) < 0)
    {
        printf("init decoder fail\n");
        return -1;
    }

    SDL_Event e;
        
    uint8_t *my_buffer = (uint8_t *)malloc(4);
    
    for (; ; ) {
        uint8_t data[4];
        ssize_t readed_len;
        readed_len = read(connfd, (uint8_t *)data, 4);
        printf("From client video data:%d\n", readed_len);
        show_bytes(data, readed_len);
        if (readed_len <= 0) {
            break;
        }
        if (readed_len == 4) {
            int packet_size = data[0] << (8 * 3) | data[1] << (8 * 2) | data[2] << (8 * 1) | data[3];
            uint8_t packet_buffer[packet_size];
            recv_all(connfd, packet_buffer, packet_size);
            my_buffer = realloc(my_buffer, packet_size);
            memcpy(my_buffer, packet_buffer, packet_size);
            vInfo->packet->data = my_buffer;
            vInfo->packet->size = packet_size;

            printf("From client packet_size:%d\n", packet_size);
        } else {
            continue;
        }
        
        start_decode(vInfo);
        
        SDL_PollEvent(&e);
        if(e.type == SDL_QUIT) {
            break;
        }
    }
    
    SDL_DestroyTexture(vInfo->sdl_texture);
    SDL_DestroyRenderer(vInfo->sdl_render);
    SDL_DestroyWindow(vInfo->sdl_window);
    SDL_Quit();

    av_frame_free(&vInfo->pFrameYUV);
    av_frame_free(&vInfo->pFrame);
    avcodec_close(vInfo->pCodecCtx);
    free(vInfo);
    
    close(socketfd);
    
    return 0;
}
