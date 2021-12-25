//
//  ffmpeg_decoder_sdl.h
//  my_xcode
//
//  Created by Owen on 2021/12/22.
//

#ifndef ffmpeg_decoder_sdl_h
#define ffmpeg_decoder_sdl_h

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/common.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_pixels.h>

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

int setupDecoder(VideoInfo * vInfo);
int setupSDL2(VideoInfo *vInfo, const char *title);
int upload_texture(VideoInfo *vInfo);

#endif /* ffmpeg_decoder_sdl_h */
