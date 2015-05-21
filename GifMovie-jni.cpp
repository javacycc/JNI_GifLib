#include <jni.h>
//#include<android/log.h>
#include <assert.h>
#include "GifMovie.h"
#include "giflib/gif_lib.h"
#include "GifCreateJavaInputStreamAdaptor.h"
typedef struct
{
	jobject stream;
	jclass streamCls;
	jmethodID readMID;
	jmethodID resetMID;
	jbyteArray buffer;
	JNIEnv * env;
} StreamContainer;

#define kClassPathName  "com/mediatek/gallery3d/gif/GifMovie"

#define RETURN_ERR_IF_NULL(value)   do { if (!(value)) { assert(0); return -1; } } while (false)
//#define LOG_TAG "GifMovie-jni"
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

static jclass       gMovie_class;
static jmethodID    gMovie_constructorMethodID;
static jfieldID     gMovie_nativeInstanceID;
static GifFileType* gifType = NULL;

jobject create_jmovie(JNIEnv* env, GifMovie* moov) {
    if (NULL == moov) {
        return NULL;
    }
    return env->NewObject(gMovie_class, gMovie_constructorMethodID,
            static_cast<jint>(reinterpret_cast<uintptr_t>(moov)));
}

static int streamReadFun(GifFileType* gif, GifByteType* bytes, int size)
 {
 	StreamContainer* sc = (StreamContainer*)gif->UserData;
 	JNIEnv* env = sc->env;
 	env->MonitorEnter(sc->stream);

 	if (sc->buffer == NULL)
 	{
 		jbyteArray buffer = env->NewByteArray(size < 256 ? 256 : size);
 		sc->buffer = (jbyteArray)env->NewGlobalRef(buffer);
 	}
 	else
 	{
 		jsize bufLen = env->GetArrayLength( sc->buffer);
 		if (bufLen < size)
 		{
 			env->DeleteGlobalRef(sc->buffer);
 			sc->buffer = NULL;
 			jbyteArray buffer = env->NewByteArray(size);
 			sc->buffer = (jbyteArray)env->NewGlobalRef( buffer);
 		}
 	}
 	//LOGI("CallIntMethod befor");
 	int len = env->CallIntMethod(sc->stream, sc->readMID, sc->buffer, 0,
 			size);
 	if (env->ExceptionOccurred())
 	{
 		env->ExceptionClear();
 		len = 0;
 	}
 	else if (len > 0)
 	{
 		env->GetByteArrayRegion( sc->buffer, 0, len, (jbyte*)bytes);
 	}

 	env->MonitorExit(sc->stream);

 	return len >= 0 ? len : 0;
 }


int openStream(GifFileType* gifFileType,JNIEnv * env,jobject stream)
{
	jclass streamCls = (jclass)env->NewGlobalRef(env->GetObjectClass(stream));
	jmethodID mid = env->GetMethodID(streamCls, "mark", "(I)V");
	jmethodID readMID = env->GetMethodID(streamCls, "read", "([BII)I");
	jmethodID resetMID = env->GetMethodID(streamCls, "reset", "()V");

	if (mid == 0 || readMID == 0 || resetMID == 0)
	{
		env->DeleteGlobalRef(streamCls);
		return (jint) NULL;
	}

	StreamContainer* container = (StreamContainer*)malloc(sizeof(StreamContainer));
	if (container == NULL)
	{
		return (jint) NULL;
	}
	container->readMID = readMID;
	container->resetMID = resetMID;
	container->stream = env->NewGlobalRef(stream);
	container->streamCls = streamCls;
	container->buffer = NULL;
	container->env = env;
	int Error = 0;
	gifType = DGifOpen(container, &streamReadFun);

	if(gifType == NULL){
		return GIF_ERROR;
	}else{
	}
	if(DGifSlurp(gifType) == GIF_ERROR){
		return GIF_ERROR;
	}
	return GIF_OK;
}


char* jstringtochar( JNIEnv *env, jstring jstr )
{
	char* rtn = NULL;
	jclass clsstring = env->FindClass("java/lang/String");
	jstring strencode = env->NewStringUTF("utf-8");
	jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
	jsize alen = env->GetArrayLength(barr);
	jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
	if (alen > 0)
	{
	rtn = (char*)malloc(alen + 1);
	memcpy(rtn, ba, alen);
	rtn[alen] = 0;
	}
	env->ReleaseByteArrayElements(barr, ba, 0);
	return rtn;
}

static GifMovie* J2Movie(JNIEnv* env, jobject movie) {
    GifMovie* m = (GifMovie*)env->GetIntField(movie, gMovie_nativeInstanceID);
    return m;
}

static int movie_width(JNIEnv* env, jobject movie) {
    return J2Movie(env, movie)->width();
}

static int movie_height(JNIEnv* env, jobject movie) {
    return J2Movie(env, movie)->height();
}


static int movie_duration(JNIEnv* env, jobject movie) {
    return J2Movie(env, movie)->duration();
}

static jboolean movie_setTime(JNIEnv* env, jobject movie, int ms) {
    return J2Movie(env, movie)->setTime(ms);
}

static void movie_destructor(JNIEnv* env, jobject, GifMovie* movie) {
    delete movie;
}

static jobject movie_decodeFile(JNIEnv* env,jobject movie,jstring jpath){
	char* path = jstringtochar(env,jpath);
	GifMovie* moov = new GifMovie(path);
	return create_jmovie(env, moov);
}

static jobject movie_decodeStream(JNIEnv* env,jobject movie,jobject jstream){
//	LOGI("movie_decodeStream");
//	GifFileType* gifFileType = NULL;
//	openStream(gifType,env,jstream);
//	GifMovie* moov = new GifMovie(gifType);
//	if(gifType==NULL){
//		LOGI("moov is null");
//	}else{
//		LOGI("moov is not null");
//	}
	 jbyteArray byteArray = env->NewByteArray(16*1024);
	 GifJavaInputStreamAdaptor* strm = GifJavaInputStreamAdaptor::CreateGifJavaInputStreamAdaptor(env, jstream, byteArray);
	 if (NULL == strm) {
	     return 0;
	 }

	GifMovie* moov = new GifMovie(strm);
	delete strm;
	return create_jmovie(env, moov);
}

static jintArray movie_getCurrColorData(JNIEnv* env,jobject movie){

	jintArray colIntArray = NULL;
	GifMovie* m = J2Movie(env, movie);
	if(m!=NULL){
	ColorData* data = m->getColorData();
		if(data!=NULL){
			int* col = data->getColorData();
			int width = data->getWidth();
			int height = data->getHeight();
			if(width>0&&height>0&&col!=NULL){
				colIntArray = env->NewIntArray(width*height);
				env->SetIntArrayRegion(colIntArray,0,width*height,col);
			}
		}
	}

	return colIntArray;
}

static int movie_totalFrameCount(JNIEnv* env, jobject movie){
	return J2Movie(env, movie)->getTotalFrameCount();
}

static int movie_frameDuration(JNIEnv* env, jobject movie,jint frameIndex){
	return J2Movie(env, movie)->getFrameDuration(frameIndex);
}

static jintArray movie_frameColorData(JNIEnv* env, jobject movie,jint frameIndex){
	jintArray colIntArray = NULL;
	GifMovie* m = J2Movie(env, movie);
	if(m!=NULL&&frameIndex>=0){
		ColorData *data = m->getFrameColorData(frameIndex);
		if(data!=NULL){
			int width = data->getWidth();
			int height = data->getHeight();
			int *col = data->getColorData();
			if(width>0&&height>0&&col!=NULL){
				colIntArray = env->NewIntArray(width*height);
				env->SetIntArrayRegion(colIntArray,0,width*height,col);
			}
		}
	}
	return colIntArray;
}

static JNINativeMethod gMethods[] = {
    {   "width",    "()I",  (void*)movie_width  },
    {   "height",   "()I",  (void*)movie_height  },
    {   "duration", "()I",  (void*)movie_duration  },
    {   "setTime",  "(I)Z", (void*)movie_setTime  },
    {   "getCurrColorData",     "()[I",
                            (void*)movie_getCurrColorData  },
    { "decodeFile", "(Ljava/lang/String;)Lcom/mediatek/gallery3d/gif/GifMovie;",
                                                        (void*)movie_decodeFile },
    { "decodeStream", "(Ljava/io/InputStream;)Lcom/mediatek/gallery3d/gif/GifMovie;",
                                                        (void*)movie_decodeStream },
    {   "getTotalFrameCount","()I", (void*)movie_totalFrameCount},
    {   "getFrameDuration","(I)I", (void*)movie_frameDuration},
    {   "getFrameColorData","(I)[I", (void*)movie_frameColorData},

    { "nativeDestructor","(I)V", (void*)movie_destructor }
};



/*
* 为某一个类注册本地方法
*/
static int registerNativeMethods(JNIEnv* env
        , const char* className
        , JNINativeMethod* gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

int register_android_graphics_Movie(JNIEnv* env)
{
    gMovie_class = env->FindClass(kClassPathName);
    RETURN_ERR_IF_NULL(gMovie_class);
    gMovie_class = (jclass)env->NewGlobalRef(gMovie_class);

    gMovie_constructorMethodID = env->GetMethodID(gMovie_class, "<init>", "(I)V");
    RETURN_ERR_IF_NULL(gMovie_constructorMethodID);

    gMovie_nativeInstanceID = env->GetFieldID(gMovie_class, "mNativeMovie", "I");
    RETURN_ERR_IF_NULL(gMovie_nativeInstanceID);

    return registerNativeMethods(env,kClassPathName,gMethods, 11);
}

/*
* System.loadLibrary("lib")时调用
* 如果成功返回JNI版本, 失败返回-1
*/
 JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    jint result = -1;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    if (!register_android_graphics_Movie(env)) {//注册
        return -1;
    }
    //成功
    result = JNI_VERSION_1_4;
    //LOGI("JNI_Onload Success...");
    return result;
}
