#include "qtfiles.h"

#include "imagerender.h"

ImageRender::ImageRender(QObject *parent)
    : QThread(parent),
      scaleFactor(1.0)
{
    restart = false;
    abort = false;
    loadImg = false;
}


ImageRender::~ImageRender()
{
    abort = true;
    condition.wakeAll();

    quit();
    wait();
}


void ImageRender::scaleImage(double scaleFac, const QImage& image)
{
    QMutexLocker locker(&mutex);

    this->scaleFactor = scaleFac;
    this->renderImage = image;

    if (!isRunning()) {
        start(LowPriority);
    }
    loadImg = false;
    restart = true;
    condition.wakeOne();
}

void ImageRender::loadTiffImage(QString tiffImgName)
{
    QMutexLocker locker(&mutex);

    this->tiffImageName = tiffImgName;

    if (!isRunning()) {
        start(LowPriority);
    }

    loadImg = true;
    restart = true;
    condition.wakeOne();
}

void ImageRender::run()
{
    forever {

        mutex.lock();

        if (!restart)
            condition.wait(&mutex);
        restart = false;

        if (abort)
            return;

        if(loadImg)
            load();
        else
            scale();

    }
}

void ImageRender::scale()
{
    double factor = this->scaleFactor;
    QImage unscaledImage = this->renderImage;

    mutex.unlock();

    QImage scaledImage = unscaledImage.scaled(unscaledImage.width()*factor,unscaledImage.height()*factor,Qt::KeepAspectRatio);

    if (!restart){
        emit renderedImage(scaledImage, factor);
    }
}

void ImageRender::load()
{
    QString imageName = this->tiffImageName;

    mutex.unlock();

    QImage origImage(imageName,"TIFF");
    if (origImage.isNull()) {
        return;
    }
    int width  = origImage.width();
    int length = origImage.height();

    int maxVal = 0;
    for(int y=0; y<length; y++){
        for(int x=0; x<width; x++){
            QRgb rgbVal = origImage.pixel(x,y);
            int value = qGray(rgbVal);
            if(maxVal<value){
                maxVal = value;
            }
        }
    }
    double iFactor = (double)((1<<8)-1)/maxVal;

    for(int y=0; y<length; y++){
        for(int x=0; x<width; x++){
            int value = qGray(origImage.pixel(x,y))*iFactor;
            origImage.setPixel(x,y,toRGB(value));
        }
    }

    if(!restart)
        emit loadedImage(origImage);
}


QRgb ImageRender::toRGB(int value)
{
    int r = value;
    int g = value;
    int b = value;

    return qRgb(r,g,b);
}
