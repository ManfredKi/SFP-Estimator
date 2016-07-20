#ifndef ESTIMATOR_H
#define ESTIMATOR_H

#include <QObject>
#include <QMutex>
#include <QPair>
#include <QFile>
#include <QWaitCondition>

#include <atomic>

#include "threadsavequeue.h"
#include "ImageStack/img_stack.hpp"
#include "roi.h"

class QTime;
class QTextStream;

class Estimator : public QObject
{
    Q_OBJECT
  public:
    explicit Estimator(int _id, QObject *parent = 0);
    ~Estimator();
    static void close();


  signals:
    void roiChanged(Roi *roi);
    void bgReady();
    void firReady();
    void roiReady();
    void findReady();
    void resultReady();
    void spotReady();
    void imageSaved(QString);
    void finished(int id);
    void firProgress(int sliceNr);
    void maxImage(int max);
    void printIntermediateImage(image16_ref image);
    void finishTime(int elapsedTime, int numSpots);

  public slots:

    /*Function Chain is:
      read->find->insertSpots
      filter->estimateGenerate
      find->insertSpots
      estimate->generateSpots
    */

    void runSingleThreaded();
    //void load();
    void read();
    void filter();
    void find();
    void findAll();
    void saveStacks();
    void generateSpotFromPendingResults();
    void insertRoisInResultImage();

    void separate(int posX, int posY, int sliceNr, uint16_t const *const *data);
    void estimate();
    void estimateGenerate();

    void generateNewStack();

    static void wakeAll();
    image16_ref* fillNewImage(int nrSpots, int length, int width, int dir_nr);

    void fillImageWithBackground(image16_ref *newImage);
    void addNewSpot(image16_ref *newImage);

    Roi * generateSpot(int Qmax, double mx, double my, double sigma, int dimX, int dimY);
    Roi * generateSpot(Roi::Result * res);

    void insertSpot(image16_ref *newImage, Roi *roi,int posX,int posY);
    uint16_t expo2D(int x, int y, int A ,double mx, double my, double sig);
    uint16_t getPoissonRnd(double mean);
    bool isMax( uint16_t const *const * data, int posX, int posY, double threashold);

    static bool initEstimatorStatics();
    static bool initEstimatorStatics(double camPixelSize, double resPixelSize, QString fileName);
    static void closeFiles();

    void restartOthers();

    static QString saveResultImage();

private:
    static void logHeader();
    static void unlockMutexs();

    void printResults(Roi::Result * res);
    void insertRoi(Roi * roi);


  private slots:
    void generateFirstBGImage();
    void generateDiffImages(double bgWeight);
    void printImg(image16_ref &image);


  private:
    int id;

public:
    static QString currFileName;

    static double dataPixelSize;
    static double lokImgPixelSize;

    static int threasholdFactor;
    static double separateFactor;
    static int cutoffFactor;

    static int dimZ;
    double bgWeight;

    static std::atomic<uint32_t> deletedRois;
    
    image16_ref * bgimg;

    static image16_ref * resultImage;
    static QVector<int> meanBgVec;

    static img_stack *tiffStack;
    static img_stack *diffStack;
    static img_stack *firStack;

    static int currResNr;

    static QFile resultFile;
    static QFile challengeFile;
    static QFile loggerFile;

    static ThreadSaveQueue<image16_ref > toFilterQueue;
    static ThreadSaveQueue< QPair< image16_ref*,image16_ref* > > toFindQueue;
    static ThreadSaveQueue< QPair< image16_ref*,image16_ref* > > toSaveQueue;
    static ThreadSaveQueue< Roi > toPrintQueue;
    static ThreadSaveQueue< Roi > roiQueue;
    static ThreadSaveQueue< Roi::Result > resultQueue;

    static QTime globalWatch;

};

#endif // ESTIMATOR_H
