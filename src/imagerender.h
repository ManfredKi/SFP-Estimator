#ifndef IMAGERENDER_H
#define IMAGERENDER_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QImage>
#include <QString>

class ImageRender : public QThread
{
    Q_OBJECT

public:
    ImageRender(QObject *parent = 0);
    ~ImageRender();

    void scaleImage(double scaleFac,const QImage& image);
    void loadTiffImage(QString tiffImgName);


signals:
    void renderedImage(const QImage& renderedImage, double scaleFac);
    void loadedImage(const QImage& loadedImage);

protected:
    void run();
    void load();
    void scale();

private:
    QRgb toRGB(int value);

    QMutex mutex;
    QWaitCondition condition;
    double scaleFactor;
    bool restart;
    bool abort;
    bool loadImg;
    QString tiffImageName;
    QImage renderImage;
};
//! [0]

#endif
