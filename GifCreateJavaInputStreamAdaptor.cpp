#include "GifCreateJavaInputStreamAdaptor.h"
#define RETURN_NULL_IF_NULL(value) \
    do { if (!(value)) {  return NULL ; } } while (false)

static jmethodID    gInputStream_resetMethodID;
static jmethodID    gInputStream_markMethodID;
static jmethodID    gInputStream_availableMethodID;
static jmethodID    gInputStream_readMethodID;
static jmethodID    gInputStream_skipMethodID;


GifJavaInputStreamAdaptor::GifJavaInputStreamAdaptor(JNIEnv* env, jobject js, jbyteArray ar)
        : fEnv(env), fJavaInputStream(js), fJavaByteArray(ar) {
        fCapacity   = env->GetArrayLength(ar);
        fBytesRead  = 0;
}

bool GifJavaInputStreamAdaptor::rewind() {
        JNIEnv* env = fEnv;

        fBytesRead = 0;

        env->CallVoidMethod(fJavaInputStream, gInputStream_resetMethodID);
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            return false;
        }
        return true;
    }

    size_t  GifJavaInputStreamAdaptor::doRead(void* buffer, size_t size) {
        JNIEnv* env = fEnv;
        size_t bytesRead = 0;
        // read the bytes
        do {
            size_t requested = size;
            if (requested > fCapacity)
                requested = fCapacity;

            jint n = env->CallIntMethod(fJavaInputStream,
                                        gInputStream_readMethodID, fJavaByteArray, 0, requested);
            if (env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
                return 0;
            }

            if (n < 0) { // n == 0 should not be possible, see InputStream read() specifications.
                break;  // eof
            }

            env->GetByteArrayRegion(fJavaByteArray, 0, n,
                                    reinterpret_cast<jbyte*>(buffer));
            if (env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
                return 0;
            }

            buffer = (void*)((char*)buffer + n);
            bytesRead += n;
            size -= n;
            fBytesRead += n;
        } while (size != 0);

        return bytesRead;
    }

    size_t GifJavaInputStreamAdaptor::doSkip(size_t size) {
        JNIEnv* env = fEnv;

        jlong skipped = env->CallLongMethod(fJavaInputStream,
                                            gInputStream_skipMethodID, (jlong)size);
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            return 0;
        }
        if (skipped < 0) {
            skipped = 0;
        }

        return (size_t)skipped;
    }

    size_t GifJavaInputStreamAdaptor::doSize() {
        JNIEnv* env = fEnv;
        jint avail = env->CallIntMethod(fJavaInputStream,
                                        gInputStream_availableMethodID);
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            avail = 0;
        }
        return avail;
    }

    size_t GifJavaInputStreamAdaptor::read(void* buffer, size_t size) {
        JNIEnv* env = fEnv;
        if (0 == buffer) {
            if (0 == size) {
                return this->doSize();
            } else {
                /*  InputStream.skip(n) can return <=0 but still not be at EOF
                    If we see that value, we need to call read(), which will
                    block if waiting for more data, or return -1 at EOF
                 */
                size_t amountSkipped = 0;
                do {
                    size_t amount = this->doSkip(size - amountSkipped);
                    if (0 == amount) {
                        char tmp;
                        amount = this->doRead(&tmp, 1);
                        if (0 == amount) {
                            // if read returned 0, we're at EOF
                            break;
                        }
                    }
                    amountSkipped += amount;
                } while (amountSkipped < size);
                return amountSkipped;
            }
        }
        return this->doRead(buffer, size);
    }





 GifJavaInputStreamAdaptor* GifJavaInputStreamAdaptor::CreateGifJavaInputStreamAdaptor(JNIEnv* env, jobject stream,
                                       jbyteArray storage, int markSize) {
    static bool gInited;

    if (!gInited) {
        jclass inputStream_Clazz = env->FindClass("java/io/InputStream");
        //RETURN_NULL_IF_NULL(inputStream_Clazz);

        gInputStream_resetMethodID      = env->GetMethodID(inputStream_Clazz,
                                                           "reset", "()V");
        gInputStream_markMethodID       = env->GetMethodID(inputStream_Clazz,
                                                           "mark", "(I)V");
        gInputStream_availableMethodID  = env->GetMethodID(inputStream_Clazz,
                                                           "available", "()I");
        gInputStream_readMethodID       = env->GetMethodID(inputStream_Clazz,
                                                           "read", "([BII)I");
        gInputStream_skipMethodID       = env->GetMethodID(inputStream_Clazz,
                                                           "skip", "(J)J");

//        RETURN_NULL_IF_NULL(gInputStream_resetMethodID);
//        RETURN_NULL_IF_NULL(gInputStream_markMethodID);
//        RETURN_NULL_IF_NULL(gInputStream_availableMethodID);
//        RETURN_NULL_IF_NULL(gInputStream_availableMethodID);
//        RETURN_NULL_IF_NULL(gInputStream_skipMethodID);

        gInited = true;
    }

    if (markSize) {
        env->CallVoidMethod(stream, gInputStream_markMethodID, markSize);
    }

    return new GifJavaInputStreamAdaptor(env, stream, storage);
}
