
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */



#include "GifMovie.h"
static inline  int PackARGB32( int a,  int r,  int g,  int b) {
    return (a << 24) |(r << 16)  |
           (g << 8) | (b << 0);
}


static int Decode(GifFileType* fileType, GifByteType* out, int size) {
	GifJavaInputStreamAdaptor* stream = (GifJavaInputStreamAdaptor*) fileType->UserData;
    return (int) stream->read(out, size);
}

GifMovie::GifMovie(GifJavaInputStreamAdaptor* stream)
{
    fGIF = DGifOpen( stream, Decode );
    if (NULL == fGIF)
        return;

    if (DGifSlurp(fGIF) != GIF_OK)
    {
        DGifCloseFile(fGIF);
        fGIF = NULL;
    }
    fCurrIndex = -1;
    fLastDrawIndex = -1;
}

GifMovie::GifMovie(char* fileName)
{
	this->fName = fileName;
    fGIF = DGifOpenFileName( fileName );
    if (NULL == fGIF)
        return;

    if (DGifSlurp(fGIF) != GIF_OK)
    {
        DGifCloseFile(fGIF);
        fGIF = NULL;
    }
    fCurrIndex = -1;
    fLastDrawIndex = -1;
}

GifMovie::GifMovie(GifFileType* gif)
{
    fGIF = gif;
    if (NULL == fGIF)
        return;

    if (DGifSlurp(fGIF) != GIF_OK)
    {
        DGifCloseFile(fGIF);
        fGIF = NULL;
    }
    fCurrIndex = -1;
    fLastDrawIndex = -1;
}

GifMovie::~GifMovie()
{
	if(fName)
		free(fName);
    if (fGIF)
        DGifCloseFile(fGIF);
}

static int savedimage_duration(const SavedImage* image)
{
    for (int j = 0; j < image->ExtensionBlockCount; j++)
    {
        if (image->ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE)
        {
            int size = image->ExtensionBlocks[j].ByteCount;
            const char* b = (const char*)image->ExtensionBlocks[j].Bytes;
            return ((b[2] << 8) | b[1]) * 10;
        }
    }
    return 0;
}

bool GifMovie::onGetInfo(Info* info)
{
    if (NULL == fGIF)
        return false;

    int dur = 0;
    for (int i = 0; i < fGIF->ImageCount; i++)
        dur += savedimage_duration(&fGIF->SavedImages[i]);

    info->fDuration = dur;
    info->fWidth = fGIF->SWidth;
    info->fHeight = fGIF->SHeight;
    return true;
}

bool GifMovie::onSetTime(int time)
{

    if (NULL == fGIF)
        return false;

    int dur = 0;
    for (int i = 0; i < fGIF->ImageCount; i++)
    {
        dur += savedimage_duration(&fGIF->SavedImages[i]);
        if (dur >= time)
        {
            fCurrIndex = i;
            return fLastDrawIndex != fCurrIndex;
        }
    }
    fCurrIndex = fGIF->ImageCount - 1;
    return true;
}

static void copyLine( int* dst, const unsigned char* src, const ColorMapObject* cmap,
                     int transparent, int width)
{

    for (; width > 0; width--, src++, dst++) {
        if (*src != transparent) {
            const GifColorType& col = cmap->Colors[*src];
            *dst = PackARGB32(0xFF, col.Red, col.Green, col.Blue);
        }
    }

}

static void copyInterlaceGroup(ColorData* bm, const unsigned char*& src,
                               const ColorMapObject* cmap, int transparent, int copyWidth,
                               int copyHeight, const GifImageDesc& imageDesc, int rowStep,
                               int startRow)
{
    int row;
    // every 'rowStep'th row, starting with row 'startRow'
    for (row = startRow; row < copyHeight; row += rowStep) {
         int* dst = bm->getColorData(imageDesc.Left, imageDesc.Top + row);
        copyLine(dst, src, cmap, transparent, copyWidth);
        src += imageDesc.Width;
    }

    // pad for rest height
    src += imageDesc.Width * ((imageDesc.Height - row + rowStep - 1) / rowStep);
}

static void blitInterlace(ColorData* bm, const SavedImage* frame, const ColorMapObject* cmap,
                          int transparent)
{
    int width = bm->getWidth();
    int height = bm->getHeight();
    GifWord copyWidth = frame->ImageDesc.Width;
    if (frame->ImageDesc.Left + copyWidth > width) {
        copyWidth = width - frame->ImageDesc.Left;
    }

    GifWord copyHeight = frame->ImageDesc.Height;
    if (frame->ImageDesc.Top + copyHeight > height) {
        copyHeight = height - frame->ImageDesc.Top;
    }

    // deinterlace
    const unsigned char* src = (unsigned char*)frame->RasterBits;

    // group 1 - every 8th row, starting with row 0
    copyInterlaceGroup(bm, src, cmap, transparent, copyWidth, copyHeight, frame->ImageDesc, 8, 0);

    // group 2 - every 8th row, starting with row 4
    copyInterlaceGroup(bm, src, cmap, transparent, copyWidth, copyHeight, frame->ImageDesc, 8, 4);

    // group 3 - every 4th row, starting with row 2
    copyInterlaceGroup(bm, src, cmap, transparent, copyWidth, copyHeight, frame->ImageDesc, 4, 2);

    copyInterlaceGroup(bm, src, cmap, transparent, copyWidth, copyHeight, frame->ImageDesc, 2, 1);
}

static void blitNormal(ColorData* bm, const SavedImage* frame, const ColorMapObject* cmap,
                       int transparent)
{
    int width = bm->getWidth();
    int height = bm->getHeight();
    const unsigned char* src = (unsigned char*)frame->RasterBits;
     int* dst = bm->getColorData(frame->ImageDesc.Left, frame->ImageDesc.Top);
    GifWord copyWidth = frame->ImageDesc.Width;
    if (frame->ImageDesc.Left + copyWidth > width) {
        copyWidth = width - frame->ImageDesc.Left;
    }

    GifWord copyHeight = frame->ImageDesc.Height;
    if (frame->ImageDesc.Top + copyHeight > height) {
        copyHeight = height - frame->ImageDesc.Top;
    }

    int srcPad, dstPad;
    dstPad = width - copyWidth;
    srcPad = frame->ImageDesc.Width - copyWidth;
    for (; copyHeight > 0; copyHeight--) {
        copyLine(dst, src, cmap, transparent, copyWidth);
        src += frame->ImageDesc.Width;
        dst += width;
    }
}

static void fillRect(ColorData* bm, GifWord left, GifWord top, GifWord width, GifWord height,
                      int col)
{
    int bmWidth = bm->getWidth();
    int bmHeight = bm->getHeight();
     int* dst = bm->getColorData(left, top);
    GifWord copyWidth = width;
    if (left + copyWidth > bmWidth) {
        copyWidth = bmWidth - left;
    }

    GifWord copyHeight = height;
    if (top + copyHeight > bmHeight) {
        copyHeight = bmHeight - top;
    }

    for (; copyHeight > 0; copyHeight--) {
        memset(dst, col, copyWidth<<2);
        dst += bmWidth;
    }
}

static void drawFrame(ColorData* bm, const SavedImage* frame, const ColorMapObject* cmap)
{
    int transparent = -1;

    for (int i = 0; i < frame->ExtensionBlockCount; ++i) {
        ExtensionBlock* eb = frame->ExtensionBlocks + i;
        if (eb->Function == GRAPHICS_EXT_FUNC_CODE &&
            eb->ByteCount == 4) {
            bool has_transparency = ((eb->Bytes[0] & 1) == 1);
            if (has_transparency) {
                transparent = (unsigned char)eb->Bytes[3];
            }
        }
    }

    if (frame->ImageDesc.ColorMap != NULL) {
        // use local color table
        cmap = frame->ImageDesc.ColorMap;
    }

    if (cmap == NULL || cmap->ColorCount != (1 << cmap->BitsPerPixel)) {
        //SkDEBUGFAIL("bad colortable setup");
        return;
    }

    if (frame->ImageDesc.Interlace) {
        blitInterlace(bm, frame, cmap, transparent);
    } else {
        blitNormal(bm, frame, cmap, transparent);
    }
}

static bool checkIfWillBeCleared(const SavedImage* frame)
{
    for (int i = 0; i < frame->ExtensionBlockCount; ++i) {
        ExtensionBlock* eb = frame->ExtensionBlocks + i;
        if (eb->Function == GRAPHICS_EXT_FUNC_CODE &&
            eb->ByteCount == 4) {
            // check disposal method
            int disposal = ((eb->Bytes[0] >> 2) & 7);
            if (disposal == 2 || disposal == 3) {
                return true;
            }
        }
    }
    return false;
}

static void getTransparencyAndDisposalMethod(const SavedImage* frame, bool* trans, int* disposal)
{
    *trans = false;
    *disposal = 0;
    for (int i = 0; i < frame->ExtensionBlockCount; ++i) {
        ExtensionBlock* eb = frame->ExtensionBlocks + i;
        if (eb->Function == GRAPHICS_EXT_FUNC_CODE &&
            eb->ByteCount == 4) {
            *trans = ((eb->Bytes[0] & 1) == 1);
            *disposal = ((eb->Bytes[0] >> 2) & 7);
        }
    }
}

// return true if area of 'target' is completely covers area of 'covered'
static bool checkIfCover(const SavedImage* target, const SavedImage* covered)
{
    if (target->ImageDesc.Left <= covered->ImageDesc.Left
        && covered->ImageDesc.Left + covered->ImageDesc.Width <=
               target->ImageDesc.Left + target->ImageDesc.Width
        && target->ImageDesc.Top <= covered->ImageDesc.Top
        && covered->ImageDesc.Top + covered->ImageDesc.Height <=
               target->ImageDesc.Top + target->ImageDesc.Height) {
        return true;
    }
    return false;
}

static void disposeFrameIfNeeded(ColorData* colorData, const SavedImage* cur, const SavedImage* next,
		ColorData* backup,  int color)
{
    // We can skip disposal process if next frame is not transparent
    // and completely covers current area
    bool curTrans;
    int curDisposal;
    getTransparencyAndDisposalMethod(cur, &curTrans, &curDisposal);
    bool nextTrans;
    int nextDisposal;
    getTransparencyAndDisposalMethod(next, &nextTrans, &nextDisposal);
    if ((curDisposal == 2 || curDisposal == 3)
        && (nextTrans || !checkIfCover(next, cur))) {
        switch (curDisposal) {
        // restore to background color
        // -> 'background' means background under this image.
        case 2:
            fillRect(colorData, cur->ImageDesc.Left, cur->ImageDesc.Top,
                     cur->ImageDesc.Width, cur->ImageDesc.Height,
                     color);
            break;

        // restore to previous
        case 3:
        	colorData->swap(*backup);
            break;
        }
    }

    // Save current image if next frame's disposal method == 3
    if (nextDisposal == 3) {
        const  int* src = colorData->getColorData();
         int* dst = backup->getColorData();
        int cnt = colorData->getWidth() * colorData->getHeight();
        memcpy(dst, src, cnt*sizeof(  int));
    }
}

bool GifMovie::onGetBitmap()
{
    const GifFileType* gif = fGIF;
    if (NULL == gif)
        return false;

    if (gif->ImageCount < 1) {
        return false;
    }

    const int width = gif->SWidth;
    const int height = gif->SHeight;

    if (width <= 0 || height <= 0) {

        return false;
    }

    // no need to draw
    if (fLastDrawIndex >= 0 && fLastDrawIndex == fCurrIndex) {
        return true;
    }

    int startIndex = fLastDrawIndex + 1;
    if (fLastDrawIndex < 0 ) {

        startIndex = 0;
        fColorData.setWidth(width);
        fColorData.setHeight(height);
        if (!fColorData.allocPixels()) {
            return false;
        }
        // create bitmap for backup

        fBackup.setWidth(width);
        fBackup.setHeight(height);
        if (!fBackup.allocPixels()) {
            return false;
        }


    } else if (startIndex > fCurrIndex) {
        // rewind to 1st frame for repeat
        startIndex = 0;
    }

    int lastIndex = fCurrIndex;
    if (lastIndex < 0) {
        // first time
        lastIndex = 0;
    } else if (lastIndex > fGIF->ImageCount - 1) {
        // this block must not be reached.
        lastIndex = fGIF->ImageCount - 1;
    }

     int bgColor = PackARGB32(0, 0, 0, 0);
    if (gif->SColorMap != NULL) {
        const GifColorType& col = gif->SColorMap->Colors[fGIF->SBackGroundColor];
        bgColor = PackARGB32(0xFF, col.Red, col.Green, col.Blue);
    }

    static  int paintingColor = PackARGB32(0, 0, 0, 0);
    // draw each frames - not intelligent way
    for (int i = startIndex; i <= lastIndex; i++) {
        const SavedImage* cur = &fGIF->SavedImages[i];
        if (i == 0) {
            bool trans;
            int disposal;
            getTransparencyAndDisposalMethod(cur, &trans, &disposal);
            if (!trans && gif->SColorMap != NULL) {
                paintingColor = bgColor;
            } else {
                paintingColor = PackARGB32(0, 0, 0, 0);
            }

            fColorData.eraseColor(paintingColor);
            fBackup.eraseColor(paintingColor);
        } else {
            // Dispose previous frame before move to next frame.
            const SavedImage* prev = &fGIF->SavedImages[i-1];
            disposeFrameIfNeeded(&fColorData, prev, cur, &fBackup, paintingColor);
        }

        // Draw frame
        // We can skip this process if this index is not last and disposal
        // method == 2 or method == 3
        if (i == lastIndex || !checkIfWillBeCleared(cur)) {
            drawFrame(&fColorData, cur, gif->SColorMap);

        }
    }

    // save index
    fLastDrawIndex = lastIndex;
    return true;
}
void GifMovie::ensureInfo()
{
     this->onGetInfo(&fInfo);
}
int GifMovie::duration()
{
    this->ensureInfo();
    return fInfo.fDuration;
}

int GifMovie::width()
{
    this->ensureInfo();
    return fInfo.fWidth;
}

int GifMovie::height()
{
    this->ensureInfo();
    return fInfo.fHeight;
}


bool GifMovie::setTime(int time)
{
    int dur = this->duration();
    if (time > dur)
        time = dur;

    bool changed = false;
    if (time != fCurrTime)
    {
        fCurrTime = time;
        changed = this->onSetTime(time);
        fNeedBitmap |= changed;
    }
    return changed;
}
ColorData* GifMovie::getColorData(){
	if(fNeedBitmap){
		this->onGetBitmap();
		fNeedBitmap = false;
	}
	return &fColorData;
}

int GifMovie::getTotalDuration(){
	return this->duration();
}
int GifMovie::getTotalFrameCount(){
	this->ensureInfo();
	if (NULL == fGIF) return 0;
	return fGIF->ImageCount;
}
int GifMovie::getFrameDuration(int frameIndex){
	if (NULL == fGIF) return -1;
	if(frameIndex>=fGIF->ImageCount||frameIndex<0){
		return -1;
	}

	return savedimage_duration(&fGIF->SavedImages[frameIndex]);
}
ColorData* GifMovie::getFrameColorData(int frameIndex){
	if (NULL == fGIF) return NULL;
	if(frameIndex>=fGIF->ImageCount||frameIndex<0){
			return NULL;
	}
//	const SavedImage* cur = &fGIF->SavedImages[i];
//	drawFrame(&fColorData, cur, fGIF->SColorMap);
	int tempIndex = fCurrIndex;
	fCurrIndex = frameIndex;
	this->onGetBitmap();
	fCurrIndex = tempIndex;
	fLastDrawIndex = frameIndex;
	return &fColorData;
}
///////////////////////////////////////////////////////////////////////////////
