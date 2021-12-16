
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>


#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#define PORT 9999
#define MAXLINE 65535

typedef struct VideoInfo
{
    AVCodecContext      *pCodecCtx;
    AVCodec             *pCodec;
    AVPacket            *packet;
    AVFrame             *pFrame;
    AVFrame             *pFrameYUV;
    unsigned char       *out_buffer;
    int                 screen_w;
    int                 screen_h;
    struct SwsContext   *img_convert_ctx;
    SDL_Window          *sdl_window;
    SDL_Renderer        *sdl_render;
    SDL_Texture         *sdl_texture;
    SDL_Rect            sdl_rect;
    
} VideoInfo;

struct VideoInfo *vInfo;

int main() {
    int socketfd;
    struct sockaddr_in servaddr, cliaddr;
    
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;
        
    if (bind(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Start UDPServer\n");
    
    uint8_t data[MAXLINE];
    
    socklen_t clilen = sizeof(cliaddr);

    vInfo = (VideoInfo *)malloc(sizeof(VideoInfo));

    avcodec_register_all();
    vInfo->pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);

    if (!vInfo->pCodec)
    {
        printf("Can not find H264 codec!!\n");
		return -1;
    } else {
        printf("find H264 codec!!\n");
    }

    vInfo->pCodecCtx = avcodec_alloc_context3(vInfo->pCodec);
    if (!vInfo->pCodecCtx)
    {
        printf("Can not alloc video context!!!\n");
		return -1;
    } else {
        printf("alloc video context!!\n");
    }

    if (avcodec_open2(vInfo->pCodecCtx, vInfo->pCodec, NULL) < 0)
    {
        printf("avcodec_open2 fail\n");
		return -1;
    } else {
        printf("avcodec_open2 success\n");
    }
    
    vInfo->pFrame = av_frame_alloc();
    vInfo->pFrameYUV = av_frame_alloc();
    vInfo->packet = (AVPacket *)av_malloc(sizeof(AVPacket));

    //初始化SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
		return -1;
	} else {
        printf("初始化SDL\n");
    }

    SDL_Event e;
    
    while (1) {
        ssize_t recvstrlen = recvfrom(socketfd, (uint8_t *)data, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &clilen);
        
        printf("Receive from client, length:%zd\n", recvstrlen);

        vInfo->packet->data = data;
        vInfo->packet->size = recvstrlen;

        int res_send = avcodec_send_packet(vInfo->pCodecCtx, vInfo->packet);
        if (res_send == 0)
        {
            // printf("avcodec_send_packet success\n");
            int res_receive_frame = avcodec_receive_frame(vInfo->pCodecCtx, vInfo->pFrame);
            if (res_receive_frame == 0)
            {
                // printf("avcodec_receive_frame success\n");
                if (vInfo->screen_w < 1 && vInfo->pFrame->width > 1)
                {
                    printf("First frame\n");
                    vInfo->sdl_window = SDL_CreateWindow("udp video player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                                        vInfo->pFrame->width, vInfo->pFrame->height , SDL_WINDOW_OPENGL);
                    if (!vInfo->sdl_window)
                    {
                        printf("SDL could not create window\n");
                    } else {
                        printf("create window success\n");
                    }
                    vInfo->sdl_render = SDL_CreateRenderer(vInfo->sdl_window, -1, 0);
                    if (!vInfo->sdl_render)
                    {
                        printf("SDL could not create sdl_render\n");
                        SDL_DestroyWindow(vInfo->sdl_window);
                    } else {
                        printf("create sdl_render success\n");
                    }
                    
                    vInfo->sdl_texture = SDL_CreateTexture(vInfo->sdl_render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 
                                                            vInfo->pCodecCtx->width, vInfo->pCodecCtx->height);
                    if (!vInfo->sdl_texture)
                    {
                        printf("SDL could not create sdl_texture\n");
                    } else {
                        printf("create sdl_texture success\n");
                    }

                    vInfo->sdl_rect.x = 0;
                    vInfo->sdl_rect.y = 0;
                    vInfo->sdl_rect.w = vInfo->pFrame->width;
                    vInfo->sdl_rect.h = vInfo->pFrame->height;

                    vInfo->img_convert_ctx = sws_getContext(vInfo->pCodecCtx->width, vInfo->pCodecCtx->height, vInfo->pCodecCtx->pix_fmt, 
                                                            vInfo->pCodecCtx->width, vInfo->pCodecCtx->height, 
                                                            AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
                    
                }

                vInfo->screen_w = vInfo->pFrame->width;
                vInfo->screen_h = vInfo->pFrame->height;
                printf("frame width:%d hight:%d \n", vInfo->screen_w, vInfo->screen_h);
                // printf("pixels width:%d hight:%d \n", vInfo->pCodecCtx->width, vInfo->pCodecCtx->height);
                printf("is keyframe:%d \n", vInfo->pFrame->key_frame);

                vInfo->out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, vInfo->pCodecCtx->width, vInfo->pCodecCtx->height, 1));
                av_image_fill_arrays(vInfo->pFrameYUV->data, vInfo->pFrameYUV->linesize, vInfo->out_buffer, AV_PIX_FMT_YUV420P, vInfo->pCodecCtx->width, vInfo->pCodecCtx->height, 1);
                sws_scale(vInfo->img_convert_ctx, (const uint8_t *const *)vInfo->pFrame->data, 
                            vInfo->pFrame->linesize, 0, vInfo->pCodecCtx->height, 
                            vInfo->pFrameYUV->data, vInfo->pFrameYUV->linesize); 

                SDL_UpdateTexture(vInfo->sdl_texture, &vInfo->sdl_rect, vInfo->pFrameYUV->data[0], vInfo->pFrameYUV->linesize[0]);
                SDL_RenderClear(vInfo->sdl_render);
                SDL_RenderCopy(vInfo->sdl_render, vInfo->sdl_texture, NULL, &vInfo->sdl_rect);
                SDL_RenderPresent(vInfo->sdl_render);

                SDL_PollEvent(&e);
                if(e.type == SDL_QUIT) {
                    break;
                }

            }  else {
                printf("res_receive_frame error:%d \n", res_receive_frame);
            }         

        } else {
            printf("avcodec_receive_frame error:%d \n", res_send);
        }

    }

    SDL_DestroyTexture(vInfo->sdl_texture);
    SDL_DestroyRenderer(vInfo->sdl_render);
    SDL_DestroyWindow(vInfo->sdl_window);    
    SDL_Quit();

    av_frame_free(&vInfo->pFrameYUV);
    av_frame_free(&vInfo->pFrame);
    avcodec_close(vInfo->pCodecCtx);

    close(socketfd);
    return 0;
    
}
