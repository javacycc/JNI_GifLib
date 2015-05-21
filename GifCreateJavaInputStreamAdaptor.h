#ifndef GifJavaInputStreamAdaptor_DEFINED___
#define GifJavaInputStreamAdaptor_DEFINED___

//#include <android_runtime/AndroidRuntime.h>
#include <jni.h>
#define size_t int
class GifJavaInputStreamAdaptor{
private:
    JNIEnv*     fEnv;
    jobject     fJavaInputStream;   // the caller owns this object
    jbyteArray  fJavaByteArray;     // the caller owns this object
    size_t      fCapacity;
    size_t      fBytesRead;
public:
    GifJavaInputStreamAdaptor(JNIEnv* env, jobject js, jbyteArray ar);
	 size_t read(void* buffer, size_t size) ;
	 bool rewind();
	 size_t  doRead(void* buffer, size_t size) ;
	 size_t  doSkip(size_t size) ;
	 size_t doSize() ;
	 static GifJavaInputStreamAdaptor* CreateGifJavaInputStreamAdaptor(JNIEnv* env, jobject stream,
	                                        jbyteArray storage, int markSize = 0);
};



#endif
