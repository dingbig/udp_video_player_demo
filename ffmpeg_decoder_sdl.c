//
//  ffmpeg_decoder_sdl.c
//  my_xcode
//
//  Created by Owen on 2021/12/22.
//

#include "ffmpeg_decoder_sdl.h"

/// Copy from ffplay.c FFmpeg的格式和SDL的格式对应表
static const struct TextureFormatEntry {
    enum AVPixelFormat format;
    int texture_fmt;
} sdl_texture_format_map[] = {
    { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
    { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
    { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
    { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
    { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
    { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
    { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
    { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
    { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
    { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
    { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
    { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
    { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
    { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
    { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
    { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
    { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
    { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
    { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
};

/// Copy from ffplay.c  获取对应格式
static void get_sdl_pix_fmt_and_blendmode(int format, Uint32 *sdl_pix_fmt, SDL_BlendMode *sdl_blendmode)
{
    int i;
    *sdl_blendmode = SDL_BLENDMODE_NONE;
    *sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    if (format == AV_PIX_FMT_RGB32   ||
        format == AV_PIX_FMT_RGB32_1 ||
        format == AV_PIX_FMT_BGR32   ||
        format == AV_PIX_FMT_BGR32_1)
        *sdl_blendmode = SDL_BLENDMODE_BLEND;
    for (i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++) {
        if (format == sdl_texture_format_map[i].format) {
            *sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
            return;
        }
    }
}

/// Copy from ffplay.c 初始化sdl textture
static int realloc_texture(VideoInfo *vInfo, SDL_Texture **texture, Uint32 new_format, int new_width, int new_height, SDL_BlendMode blendmode, int init_texture)
{
    Uint32 format;
    int access, w, h;
    if (!*texture || SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w || new_height != h || new_format != format) {
        void *pixels;
        int pitch;
        if (*texture)
            SDL_DestroyTexture(*texture);
        if (!(*texture = SDL_CreateTexture(vInfo->sdl_render, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height)))
            return -1;
        if (SDL_SetTextureBlendMode(*texture, blendmode) < 0)
            return -1;
        if (init_texture) {
            if (SDL_LockTexture(*texture, NULL, &pixels, &pitch) < 0)
                return -1;
            memset(pixels, 0, pitch * new_height);
            SDL_UnlockTexture(*texture);
        }
        av_log(NULL, AV_LOG_VERBOSE, "Created %dx%d texture with %s.\n", new_width, new_height, SDL_GetPixelFormatName(new_format));
    }
    return 0;
}


/// 初始化解码器
int setupDecoder(VideoInfo * vInfo) {
    
    avcodec_register_all();
    
    vInfo->pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    
    if (!vInfo->pCodec)
    {
        printf("Can not find H264 codec!!\n");
        return -1;
    }
    
    vInfo->pCodecCtx = avcodec_alloc_context3(vInfo->pCodec);
    if (!vInfo->pCodecCtx)
    {
        printf("Can not alloc video context!!!\n");
        return -1;
    }
    
    AVCodecParameters *codecParameters = avcodec_parameters_alloc();
    
    codecParameters->codec_id = AV_CODEC_ID_H264;        //码流ID    H264
    codecParameters->codec_type = AVMEDIA_TYPE_VIDEO;    //类型        视频
    codecParameters->format = AV_PIX_FMT_BGR4_BYTE;    //解码的格式        YUV420p
    
    avcodec_parameters_to_context(vInfo->pCodecCtx, codecParameters);
    
    if (avcodec_open2(vInfo->pCodecCtx, vInfo->pCodec, NULL) < 0)
    {
        printf("avcodec_open2 fail\n");
        return -1;
    }
    
    vInfo->pFrame = av_frame_alloc();
    vInfo->pFrameYUV = av_frame_alloc();
    vInfo->packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    
    return 0;
}

int setupSDL2(VideoInfo *vInfo, const char *title) {
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        return -1;
    }
    vInfo->sdl_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                         vInfo->pFrame->width, vInfo->pFrame->height, SDL_WINDOW_OPENGL);
    if (!vInfo->sdl_window)
    {
        printf("SDL could not create window\n");
        return -1;
    }
    vInfo->sdl_render = SDL_CreateRenderer(vInfo->sdl_window, -1, 0);
    if (!vInfo->sdl_render)
    {
        printf("SDL could not create sdl_render\n");
        SDL_DestroyWindow(vInfo->sdl_window);
        return -1;
    }
    
    // texture先不用初始化，后面会按需重建
//    vInfo->sdl_texture = SDL_CreateTexture(vInfo->sdl_render, SDL_PIXELFORMAT_IYUV,
//                                           SDL_TEXTUREACCESS_STREAMING, vInfo->pCodecCtx->width,
//                                           vInfo->pCodecCtx->height);
//    if (!vInfo->sdl_texture) {
//        printf("SDL could not create sdl_texture\n");
//        return -1;
//    }
    
    vInfo->sdl_rect.x = 0;
    vInfo->sdl_rect.y = 0;
    vInfo->sdl_rect.w = vInfo->pFrame->width;
    vInfo->sdl_rect.h = vInfo->pFrame->height;
    
    return 0;
}

static int update_yuv_format(VideoInfo *vInfo) {
    int ret = 0;
    if (vInfo->pFrame->linesize[0] > 0 && vInfo->pFrame->linesize[1] > 0 && vInfo->pFrame->linesize[2] > 0) {
        ret = SDL_UpdateYUVTexture(vInfo->sdl_texture, &vInfo->sdl_rect,
                                   vInfo->pFrame->data[0], vInfo->pFrame->linesize[0],
                                   vInfo->pFrame->data[1], vInfo->pFrame->linesize[1],
                                   vInfo->pFrame->data[2], vInfo->pFrame->linesize[2]);

    } else if (vInfo->pFrame->linesize[0] < 0 && vInfo->pFrame->linesize[1] < 0 && vInfo->pFrame->linesize[2] < 0) {
        ret = SDL_UpdateYUVTexture(vInfo->sdl_render, &vInfo->sdl_rect,
                                   vInfo->pFrame->data[0] + vInfo->pFrame->linesize[0] * (vInfo->pFrame->height                    - 1), -vInfo->pFrame->linesize[0],
                                   vInfo->pFrame->data[1] + vInfo->pFrame->linesize[1] * (AV_CEIL_RSHIFT(vInfo->pFrame->height, 1) - 1), -vInfo->pFrame->linesize[1],
                                   vInfo->pFrame->data[2] + vInfo->pFrame->linesize[2] * (AV_CEIL_RSHIFT(vInfo->pFrame->height, 1) - 1), -vInfo->pFrame->linesize[2]);
    } else {
        printf("Cannot initialize the conversion context\n");
    }
    
    return ret;
}

static int update_unknown_format(VideoInfo *vInfo) {
    int ret = 0;
    
    if (!vInfo->img_convert_ctx) {
        vInfo->img_convert_ctx = sws_getContext(vInfo->pCodecCtx->width, vInfo->pCodecCtx->height,
                                                vInfo->pCodecCtx->pix_fmt,
                                                vInfo->pCodecCtx->width, vInfo->pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
        
        vInfo->out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, vInfo->pCodecCtx->width, vInfo->pCodecCtx->height, 1));
    }
    
    av_image_fill_arrays(vInfo->pFrameYUV->data, vInfo->pFrameYUV->linesize, vInfo->out_buffer, AV_PIX_FMT_YUV420P, vInfo->pCodecCtx->width, vInfo->pCodecCtx->height, 1);
    printf("av_image_fill_arrays\n");
    sws_scale(vInfo->img_convert_ctx, (const uint8_t *const *)vInfo->pFrame->data,
              vInfo->pFrame->linesize, 0, vInfo->pCodecCtx->height,
              vInfo->pFrameYUV->data, vInfo->pFrameYUV->linesize);
    printf("SDL_UpdateTexture\n");
    ret = SDL_UpdateTexture(vInfo->sdl_texture, &vInfo->sdl_rect, vInfo->pFrameYUV->data[0], vInfo->pFrameYUV->linesize[0]);
    return ret;
}

int upload_texture(VideoInfo *vInfo)
{
    int ret = 0;
    printf("upload_texture screen_w:%d  frame_width:%d frame_height:%d\n", vInfo->screen_w, vInfo->pFrame->width, vInfo->pFrame->height);
    if (!vInfo->sdl_window) {
        printf("No window! \n");
    }
    if (!vInfo->sdl_window && vInfo->pFrame->width > 1)
    {
        printf("First frame\n");
        setupSDL2(vInfo, "Video Player");
    }
    
    vInfo->screen_w = vInfo->pFrame->width;
    vInfo->screen_h = vInfo->pFrame->height;
    printf("frame width:%d hight:%d    pixel width:%d hight:%d \n", vInfo->screen_w, vInfo->screen_h, vInfo->pCodecCtx->width, vInfo->pCodecCtx->height);
    printf("is keyframe:%d pFrame.format:%d ctxFormat:%d\n", vInfo->pFrame->key_frame, vInfo->pFrame->format, vInfo->pCodecCtx->pix_fmt);
    
    if (vInfo->sdl_window == NULL) {
        return -1;
    }
    
    Uint32 sdl_pix_fmt;
    SDL_BlendMode sdl_blendmode;
    get_sdl_pix_fmt_and_blendmode(vInfo->pFrame->format, &sdl_pix_fmt, &sdl_blendmode);
    
    if (realloc_texture(vInfo, &vInfo->sdl_texture, sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888 : sdl_pix_fmt, vInfo->pFrame->width, vInfo->pFrame->height, sdl_blendmode, 0) < 0)
        return -1;
    
    switch (sdl_pix_fmt)
    {
        
        case SDL_PIXELFORMAT_IYUV:
            // frame格式对应SDL_PIXELFORMAT_IYUV，不用进行图像格式转换，调用SDL_UpdateYUVTexture()更新SDL texture
            printf("sdl_pix_fmt:%d SDL_PIXELFORMAT_IYUV\n", sdl_pix_fmt);
            update_yuv_format(vInfo);
//            update_unknown_format(vInfo);
            break;
        
        case SDL_PIXELFORMAT_UNKNOWN:
            // frame格式是SDL不支持的格式，则需要进行图像格式转换
            printf("sdl_pix_fmt:%d SDL_PIXELFORMAT_UNKNOWN\n", sdl_pix_fmt);
            update_unknown_format(vInfo);
            break;
            
        default:
            // frame格式对应其他SDL像素格式，不用进行图像格式转换，调用SDL_UpdateTexture()更新SDL texture
            printf("sdl_pix_fmt:%d default\n", sdl_pix_fmt);
            if (vInfo->pFrame->linesize[0] < 0)
            {
                ret = SDL_UpdateTexture(vInfo->sdl_texture, &vInfo->sdl_rect, vInfo->pFrame->data[0] + vInfo->pFrame->linesize[0] * (vInfo->pFrame->height - 1), vInfo->pFrame->linesize[0]);
            } else {
                ret = SDL_UpdateTexture(vInfo->sdl_texture, &vInfo->sdl_rect, vInfo->pFrame->data[0], vInfo->pFrame->linesize[0]);
            }
            
            break;
    }

    SDL_RenderClear(vInfo->sdl_render);
    SDL_RenderCopy(vInfo->sdl_render, vInfo->sdl_texture, NULL, &vInfo->sdl_rect);
    SDL_RenderPresent(vInfo->sdl_render);
    
    return 0;
}
