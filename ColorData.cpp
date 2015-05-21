#include "ColorData.h"

ColorData::ColorData(){
	width = 0;
	height = 0;
	colorData = NULL;
}
ColorData::~ColorData(){
	if(colorData!=NULL){
		delete colorData;
		colorData = NULL;
	}
}
bool ColorData::allocPixels(){
	if(width<=0||height<=0){
		return false;
	}
	if(colorData==NULL){
		colorData = ( int*)malloc(width*height*sizeof(int));
		if(colorData==NULL){
			return false;
		}else{
			return true;
		}
	}
	return true;

}
void ColorData::eraseColor( int color){
	if(colorData==NULL){
		return;
	}
	 int* temp = colorData;
	for(int i=0;i<width*height;i++,temp++){
		*temp = color;
	}
}
int* ColorData::getColorData(){
	return colorData;
}

void ColorData::swap(ColorData& other){
//	if(this->width<=0||this->height<=0||other.width<=0||other.height<=0||this->width){
//
//	}
	//LOGI("Gif swap.....");
}

 int* ColorData::getColorData(int x,int y){
	return (colorData + y * width + x);
}

 int ColorData::getWidth(){
	return this->width;
}
 int ColorData::getHeight(){
	return this->height;
}
void ColorData::setWidth(int _width){
	this->width = _width;
}
void ColorData::setHeight(int _height){
	this->height = _height;
}
