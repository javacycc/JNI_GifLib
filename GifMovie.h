#ifndef _Included_com_lewa_gif_GifMovie
#define _Included_com_lewa_gif_GifMovie
//#include <android/log.h>
//#define LOG_TAG "GifMovie_jni"
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#include "giflib/gif_lib.h"
#include "ColorData.h"
#include "GifCreateJavaInputStreamAdaptor.h"
#endif
/**
    映射java层GifMovie的C++类，维护了一个帧缓冲区，使用GifLib库解析Gif文件，
    将Gif动画的每一帧填充到颜色缓冲区内。

*/

class GifMovie  {
public:
    /**
        提供一个已经用GifLib解析完成的并存储Gif相关信息的指针初始化GifMovie
    */
	GifMovie(GifFileType* gif);
    /**
        提供一个Gif文件的路径初始化GifMovie
    **/
	GifMovie(char* fileName);
    /**
        提供一个java层java.io.InputStream输入流流初始化Gifmovie，GifJavaInputStreamAdaptor类是C++层用于适配java层输入流的对象，
        在GifMovie-jni.cpp文件中movie_decodeStream将java层的输入流封装成GifJavaInputStreamAdaptor
    **/
	GifMovie(GifJavaInputStreamAdaptor* stream);
    /**
        主要是关闭和释放GifLib库解析Gif文件所产生的资源
    **/
    virtual ~GifMovie();
    /**
    Gif文件播放的总时间
    **/
    int duration();
    /**
    Gif动画的宽度
    */
    int     width();
    /**
    Gif动画的高度
    */
    int     height();
    /**
    和duration()重复 ，主要是适配MTK平台
    **/
	int getTotalDuration();
    /**
    获得Gif动画帧的数量
    **/
    int getTotalFrameCount();
    /**
    获得某一帧动画的持续时间
    */
    int getFrameDuration(int frameIndex);
    /**
    解析Gif，填充帧缓冲区并返回帧缓冲区的指针
    **/
    ColorData* getFrameColorData(int frameIndex);
    /**
    设置动画播放的时间点，适配Android官方提供的可以播放Gif的Movie类
    **/
    bool setTime(int time);
    /**
    初始化或者重新解析Gif，并将解析的数据存储到GifMovie的类变量中
    **/
    void ensureInfo();
    /**
    返回当前帧缓冲区里的数据，同getFrameColorData(),适配Android官方提供的可以播放Gif的Movie类
    */
    ColorData* getColorData();
protected:
    /**
    保存Gif信息的结构体
    */
   struct Info {
            int  fDuration;
            int     fWidth;
            int     fHeight;
    };
    virtual bool onGetInfo(Info*);
    virtual bool onGetBitmap();
    virtual bool onSetTime(int time);

private:
    GifFileType* fGIF;
    int fCurrIndex;
    int fLastDrawIndex;
    ColorData fBackup ;
    ColorData fColorData ;

    Info        fInfo;
    int      fCurrTime;
    /**
    帧缓冲区
    **/
    ColorData   fcolData;
    /**
    是否需要重新绘制缓冲区
    */
    bool        fNeedBitmap;
    /**
    Gif文件路径。（这个地方以指针的形式存储Gif文件的路径是一个不太好的方式，应当存储一个对象，而不是一个指针）
    **/
    char* fName;
};
