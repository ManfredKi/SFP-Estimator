#include "qtfiles.h"

#include <QMutexLocker>
#include "estimator.h"

#include "lokalizationthread.h"

LokalizationThread::LokalizationThread(QObject *parent) :
    QThread(parent)
{
    numThreads = 2;
    abort = false;
    exePath = QDir::currentPath();
    readInitFile();

    qRegisterMetaType<Roi>("Roi");
    qRegisterMetaType<image16_ref>("image16_ref");
}

LokalizationThread::~LokalizationThread()
{
  Estimator::close();

  abort = true;
  condition.wakeAll();

  quit();
  wait();
}

void LokalizationThread::emitFinishTime()
{
  emit finishTime(Estimator::globalWatch.elapsed(), Estimator::currResNr);
}

void LokalizationThread::startEstimator(double camPixelSize, double resPixelSize, QString fileName)
{
  {
    QMutexLocker locker(&mutex);

    this->camPixelSize = camPixelSize;
    this->resPixelSize = resPixelSize;
    this->currFileName = fileName;
  }
  if (!isRunning()) {
      start(LowPriority);
  }
  condition.wakeOne();
}

void LokalizationThread::sendPrintIntermediateImageSignal(image16_ref image)
{
    QString interImageName = "tmp_image.tiff";
    img_stack tmp_stack(interImageName.toStdString().c_str(),"w");
    tmp_stack.append_image(image);
    emit printIntermediateImage(interImageName);
}

void LokalizationThread::run()
{
    QVector<QThread*> threadVec;

    Estimator readEstim(0);

    connect(&readEstim,SIGNAL(maxImage(int)),this,SLOT(maxImage(int)));
    forever
    {

      QVector<QThread*> threadVec;

      if(!readEstim.initEstimatorStatics(camPixelSize,resPixelSize,currFileName)){
          qDebug()<<"No File Selected!";
      }else{

        for(int thread=0; thread<numThreads; thread++){
          Estimator * filterEstim = new Estimator(10+thread);
          QThread *filterThread   = new QThread;
          connect(filterThread,SIGNAL(started()),filterEstim,SLOT(filter()));
          connectMoveStart(filterEstim,filterThread);
          threadVec << filterThread;

          connect(filterEstim,SIGNAL(firProgress(int)),this,SLOT(firProgress(int)));

  #ifndef CHAIN
          Estimator * calcEstim   = new Estimator(30+thread);
          QThread *calcThread     = new QThread;
          connect(calcThread,SIGNAL(started()),calcEstim,SLOT(estimateGenerate()));
          connectMoveStart(calcEstim,calcThread);
          threadVec << calcThread;
    #endif
        }

    #ifndef CHAIN

        Estimator * findEstim   = new Estimator(20);
        QThread *findThread     = new QThread;
        connect(findThread,SIGNAL(started()),findEstim,SLOT(find()));
        connectMoveStart(findEstim,findThread);
        threadVec << findThread;


        Estimator * insertEstim   = new Estimator(50);
        QThread *insertThread     = new QThread;
        connect(insertThread,SIGNAL(started()),insertEstim,SLOT(insertRoisInResultImage()));
        connectMoveStart(insertEstim,insertThread);
        threadVec << insertThread;


//        Estimator * generateEstim   = new Estimator(40);
//        QThread *generateThread     = new QThread;
//        connect(generateThread,SIGNAL(started()),generateEstim,SLOT(generateSpotFromPendingResults()));
//        connectMoveStart(generateEstim,generateThread);
//        threadVec << generateThread;

    #ifdef SAVE
        Estimator * saveEstim   = new Estimator(60);
        QThread *saveThread     = new QThread;
        connect(saveThread,SIGNAL(started()),saveEstim,SLOT(saveStacks()));
        connectMoveStart(saveEstim,saveThread);
        threadVec << saveThread;
    #endif

    #endif

        readEstim.read();
        qDebug()<<"Estimator end!";

        for(auto & th : threadVec){
          th->quit();
          th->wait();
        }

        for(auto & th : threadVec){
          delete th;
        }

        const auto resName = Estimator::saveResultImage();

        estimatorFinished(resName);
        emitFinishTime();
      }

      QMutexLocker locker(&mutex);
      condition.wait(&mutex);
      if(abort){
        return;
      }
    }

    qDebug() << "Lokalization Thread will finish";
}

void LokalizationThread::connectMoveStart(Estimator *estim, QThread *thread)
{
    connect(estim,  SIGNAL(finished(int)), thread, SLOT(quit()));
    connect(estim,  SIGNAL(finished(int)), estim, SLOT(deleteLater()));

    connect(estim,  SIGNAL(printIntermediateImage(image16_ref)),this, SLOT(sendPrintIntermediateImageSignal(image16_ref)));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    estim->moveToThread(thread);

    thread->start(QThread::HighPriority);
}

void LokalizationThread::estimatorFinished(QString lokImgFileName)
{
    emit imageSaved(lokImgFileName);
}

void LokalizationThread::readInitFile()
{
    QFile initFile(exePath + "/setup.ini");
    if (!initFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "File setup.ini not found!";
        numThreads = 2;
        separateFactor = 0.7;
        threasholdFactor = 2;
        cutoffFactor = 2;
        saveInitFile();
        initFile.open(QIODevice::ReadOnly | QIODevice::Text);
    }

    QTextStream in(&initFile);
    numThreads = -1;
    separateFactor = -1.0;
    threasholdFactor = -1;
    cutoffFactor = -1;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if(line.left(15)== "NumberOfThreads"){
            numThreads = line.section("\t",1,1).toInt();
        }else if(line.left(14)== "SeparateFactor"){
            separateFactor = line.section("\t",1,1).toDouble();
        }else if(line.left(16)== "ThreasholdFactor"){
            threasholdFactor = line.section("\t",1,1).toInt();
        }else if(line.left(12)== "CutoffFactor"){
            cutoffFactor = line.section("\t",1,1).toInt();
        }
    }
    if(numThreads<0 || separateFactor<0||threasholdFactor<0 || cutoffFactor<0){
        qDebug() << "Error: Initfile not complete!";
        numThreads = 2;
        separateFactor = 0.7;
        threasholdFactor = 2;
        cutoffFactor = 2;
        saveInitFile();
    }

    initFile.close();
}

void LokalizationThread::setParameters()
{
    Estimator::cutoffFactor     = cutoffFactor;
    Estimator::threasholdFactor = threasholdFactor;
    Estimator::separateFactor   = separateFactor;
}

void LokalizationThread::firProgress(int sliceNr)
{
  emit progress(sliceNr);
}

void LokalizationThread::maxImage(int max)
{
  emit numImages(max);
}


void LokalizationThread::saveInitFile()
{
    QFile file(exePath + "/setup.ini");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
       return;

    QTextStream out(&file);
    out << "########################################\n";
    out << "##### LokMik initialisation File\n";
    out << "########################################\n\n";
    out << "NumberOfThreads:\t" << numThreads << "\n";
    out << "SeparateFactor:\t" << separateFactor << "\n";
    out << "ThreasholdFactor:\t" << threasholdFactor << "\n";
    out << "CutoffFactor:\t" << cutoffFactor << "\n";

    file.close();
}
