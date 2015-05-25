#giflib_lewa module
include $(CLEAR_VARS)

LOCAL_MODULE    := libgiflib_lewa
OCAL_CPP_EXTENSION := .cpp
LOCAL_SRC_FILES := \
      ColorData.cpp \
      ColorData.h \
      GifMovie.cpp \
      GifCreateJavaInputStreamAdaptor.cpp \
      GifMovie-jni.cpp \
      GifMovie.h \
      giflib/gif_err.c \
      giflib/dgif_lib.c \
      giflib/gifalloc.c
LOCAL_MULTILIB := 32
LOCAL_SDK_VERSION := 9
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
