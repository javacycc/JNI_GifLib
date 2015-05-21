#ifndef _COLOR_DATA_H_
#define _COLOR_DATA_H_
#include <stdlib.h>
//#include <android/log.h>
//#define LOG_TAG "GifMovie_jni"
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
class ColorData {
	private :
		 int* colorData ;
		 int width;
		 int height;

	public :
		ColorData();
	    ~ColorData();
		bool allocPixels();
		void eraseColor( int color);
		 int* getColorData();
		 int* getColorData(int x,int y);
		void swap(ColorData& other);
		 int getWidth();
		 int getHeight();
	    void setWidth(int width);
	    void setHeight(int height);
};
#endif
