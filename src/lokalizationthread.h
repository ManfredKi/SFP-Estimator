#ifndef LOKALIZATIONTHREAD_H
#define LOKALIZATIONTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "ImageStack/img_stack.hpp"

class Estimator;

class LokalizationThread : public QThread
{
    Q_OBJECT
public:
    explicit LokalizationThread(QObject *parent = 0);
    ~LokalizationThread();
    
signals:
    void imageSaved(QString lokImgFileName);
    void printIntermediateImage(QString interImageName);
    void finishTime(int elapsedTime, int numSignals);
    void progress(int sliceNr);
    void numImages(int maxSlize);


public slots:

    void setNumthreads(int _numThreads){numThreads = _numThreads;}
    void setSeparateFactor(double factor){separateFactor = factor;}
    void setThresholdFactor(int factor){threasholdFactor = factor;}
    void setCutoffFactor(int factor){cutoffFactor=factor;}
    void setPixelSize(int pxs){pixelSize=pxs;}
    void setParameters();

    void startEstimator(double camPixelSize, double resPixelSize, QString fileName);
    void estimatorFinished(QString lokImgFileName);

    void firProgress(int sliceNr);
    void maxImage(int max);

protected:
    void connectMoveStart(Estimator *estim, QThread *thread);
    void saveInitFile();
    void readInitFile();
    void run();

protected slots:
    void emitFinishTime();
    void sendPrintIntermediateImageSignal(image16_ref image);

private:

    QMutex mutex;
    QWaitCondition condition;

    int numThreads;
    int pixelSize;
    double separateFactor;
    int threasholdFactor;
    int cutoffFactor;

    double camPixelSize;
    double resPixelSize;
    QString currFileName;
    bool abort;
    QString exePath;    
};

#endif // LOKALIZATIONTHREAD_H
