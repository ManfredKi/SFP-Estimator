#include "qtfiles.h"

#include <math.h>

#include "estimator.h"
#include <unistd.h>

#define ROISIZE 7
#define ROIRAD  (ROISIZE-1)/2
#define CATCHROIS 100000

//default values
int Estimator::threasholdFactor = 3;
int Estimator::cutoffFactor = 2;
double Estimator::separateFactor = 0.7;

ThreadSaveQueue< image16_ref > Estimator::toFilterQueue;
ThreadSaveQueue< QPair< image16_ref*,image16_ref* > > Estimator::toFindQueue;
ThreadSaveQueue< QPair< image16_ref*,image16_ref* > > Estimator::toSaveQueue;
ThreadSaveQueue< Roi > Estimator::toPrintQueue;
ThreadSaveQueue< Roi > Estimator::roiQueue;
ThreadSaveQueue< Roi::Result > Estimator::resultQueue;

QMutex estimateMutex;
QMutex fillLokImgMutex;

QVector<int> Estimator::meanBgVec;

image16_ref * Estimator::resultImage = nullptr;

img_stack* Estimator::tiffStack  = nullptr;
img_stack* Estimator::firStack  = nullptr;
img_stack* Estimator::diffStack = nullptr;

double Estimator::dataPixelSize = 0;
double Estimator::lokImgPixelSize = 0;

int Estimator::dimZ = 0;

int Estimator::currResNr = 0;

std::atomic<uint32_t> Estimator::deletedRois(0);

QString Estimator::currFileName="lokImg.tif";
QFile Estimator::resultFile;
QFile Estimator::challengeFile;
QFile Estimator::loggerFile;
QTime Estimator::globalWatch;


Estimator::Estimator(int _id, QObject *parent)
: QObject(parent),
  id(_id),
  bgimg(nullptr)
{
}

Estimator::~Estimator()
{
  if(bgimg){
    delete bgimg;
    bgimg = nullptr;
  }
  qDebug() << "Estimator deleted" << id;
}

void Estimator::closeFiles()
{
  if(resultFile.isOpen())
    resultFile.close();

  if(challengeFile.isOpen())
  challengeFile.close();

  if(loggerFile.isOpen())
    loggerFile.close();
}

void Estimator::close()
{
  closeFiles();

  toFilterQueue.close();
  toFindQueue.close();
  toPrintQueue.close();
  toSaveQueue.close();
  roiQueue.close();
  resultQueue.close();
}

bool Estimator::initEstimatorStatics()
{
    QString fileName = QFileDialog::getOpenFileName(0, tr("Open File"),
                                                    QDir::currentPath(),
                                                    tr("TiffImages (*.tif *.tiff)"));
   if(fileName.isEmpty()){
      return false;
   }

   dataPixelSize = 100.0;
   lokImgPixelSize = 10.0;

   dataPixelSize = QInputDialog::getDouble(nullptr,"Set Data Pixel Size","Define camera pixel dimensions in nanometers",102,1,2000,0);
   lokImgPixelSize = QInputDialog::getDouble(nullptr,"Set Lokalisation image Pixel Size","Define pixel dimensions of final lokalization image in nanometers",10,0.01,2000,1);

   return initEstimatorStatics(dataPixelSize,lokImgPixelSize,fileName);
}

bool Estimator::initEstimatorStatics(double camPixelSize, double resPixelSize, QString fileName)
{
  QFileInfo info(fileName);
  currFileName = info.baseName();

  QDir::setCurrent(info.absolutePath());

  tiffStack = new img_stack(fileName.toStdString(),std::string("r"));

#ifdef SAVE
  firStack  = new img_stack((currFileName+"_fir.tif").toStdString(),std::string("w"));
  diffStack = new img_stack((currFileName+"_bg.tif" ).toStdString(),std::string("w"));
#endif

  dataPixelSize = camPixelSize;
  lokImgPixelSize = resPixelSize;

  if(resultFile.isOpen()){
    resultFile.close();
  }

  resultFile.setFileName(info.baseName()+"_locations.txt");
  if (!resultFile.open(QIODevice::WriteOnly | QIODevice::Text))
    qDebug()<< "error: result file could not be opened!";

  if(challengeFile.isOpen()){
    challengeFile.close();
  }
  challengeFile.setFileName(info.baseName()+"_result_locations.csv");
  if (!challengeFile.open(QIODevice::WriteOnly | QIODevice::Text))
      qDebug()<< "error: challenge file could not be opened!";


  if(loggerFile.isOpen()){
      loggerFile.close();
  }
  loggerFile.setFileName(info.baseName()+"_log.txt");
  if (!loggerFile.open(QIODevice::WriteOnly | QIODevice::Text))
      qDebug()<< "error: result file could not be opened!";


  logHeader();

  dimZ = tiffStack->img_count();

  currResNr = 0;
  deletedRois = 0;

  resultImage = new image16_ref((double)tiffStack->get_image(0).get_length()*dataPixelSize/lokImgPixelSize,
                                tiffStack->get_image(0).get_width()*dataPixelSize/lokImgPixelSize,
                                tiffStack->get_image(0).get_bits_per_pixel(),0);

  meanBgVec.clear();

  toFilterQueue.reset();
  toFindQueue.reset();
  toPrintQueue.reset();
  toSaveQueue.reset();
  roiQueue.reset();
  resultQueue.reset();

  globalWatch.restart();

  return true;
}


//void Estimator::load()
//{
//    QTime loadWatch;
//    loadWatch.start();
//    for(int z=0; z<dimZ; z++){
//        rawImages.push_back( new image16_ref(tiffStack->get_image(z)));
//    }
//    qDebug() << "loadTime:" << loadWatch.elapsed();
//}

void Estimator::read()
{
  emit maxImage(dimZ);

  generateFirstBGImage();

  bgWeight = 1.0/8.0;
  generateDiffImages(bgWeight);
#ifdef CHAIN
  find();
#endif
}

void Estimator::generateFirstBGImage()
{
  bgimg = new image16_ref(tiffStack->get_image(0));
  for(int i=1; i<4; i++){
    *bgimg += tiffStack->get_image(i);
  }
  *bgimg >>=2;
}

void Estimator::generateDiffImages(double bgWeight)
{
  toFilterQueue.setThreashold(dimZ/10);

  QTextStream out(&loggerFile);
#ifdef LOG
  out << globalWatch.elapsed() <<" "<< id <<" diff start " << toFilterQueue.size() << "\n";
#endif
  QTime readWatch;
  readWatch.start();
  for(int z=0; z<dimZ; z++){

    image16_ref *diffimg = new image16_ref(tiffStack->get_image(z));

    int meanbg = diffimg->subtr_and_update_bg(*bgimg,bgWeight);
    meanBgVec.push_back(meanbg);
    toFilterQueue.push_back(diffimg);
  }

  qDebug() << "++++readTime:" << readWatch.elapsed();
  qDebug() <<"\n" << globalWatch.elapsed() << "++++Estimator"<< id <<"Last Image Subtracted" <<dimZ-1;

  out << "Estimator "<< id << "  Elapsed time " << globalWatch.elapsed() << "  Background subtracting - num frames: " <<dimZ-1 << "\n";

  toFilterQueue.finish(dimZ);

  delete tiffStack;
}

void Estimator::filter()
{
  int sliceNr = -1;

  toFindQueue.setThreashold(dimZ/10);

  QTextStream out(&loggerFile);
#ifdef LOG
  out << globalWatch.elapsed() <<" "<< id <<" filter sleep " << toFilterQueue.size() << "\n";
#endif

  image16_ref *diffImg = nullptr;

  while(toFilterQueue.pop_front(diffImg))
  {
    sliceNr = diffImg->get_dir_number();

    image16_ref *firImg = new image16_ref(diffImg->get_length(),diffImg->get_width(),diffImg->get_scanline_size(),diffImg->get_dir_number());
    if(!firImg){
      qDebug() << "Filter: not enough Memory";
      throw "Filter: Out of memory";
    }

    diffImg->apply_highpass(1,firImg);

    toFindQueue.push_back(new QPair< image16_ref*,image16_ref* >(diffImg,firImg) );

    if( toFindQueue.size()>=100 )
    {
      emit firProgress(sliceNr);
#ifdef LOG
      out << globalWatch.elapsed() <<" "<< id <<" filter " << toFindQueue.size() << " " << toFindQueue.getNumHandles() << "\n";
#endif
    }
  }

  qDebug() << globalWatch.elapsed() << "Estimator"<< id <<"Last Image Filtered" <<sliceNr;

  toFilterQueue.close();  
  toFindQueue.finish(toFilterQueue.getPops());


#ifdef LOG
  out << "Estimator "<< id << "  Elapsed time " << globalWatch.elapsed()  <<" Image filtering - num frames: " <<sliceNr << "\n";
#endif

#ifdef CHAIN
  estimateGenerate();
#endif

  emit finished(id);
}


void Estimator::findAll()
{
  find();
}

void Estimator::find()
{
  int padding = 3;

  roiQueue.setThreashold(3000);
  toSaveQueue.setThreashold(dimZ/10);

  QTextStream out(&loggerFile);
#ifdef LOG
  out << globalWatch.elapsed() <<" "<< id <<" find start " << toFindQueue.size() << "\n";
#endif

  QPair< image16_ref*,image16_ref* > * findPair = nullptr;

  while(toFindQueue.pop_front(findPair))
  {
    image16_ref * diffImg = findPair->first;
    image16_ref * firImg  = findPair->second;

    int sliceNr = firImg->get_dir_number();

    double threashold = threasholdFactor * sqrt(meanBgVec[sliceNr]);
    int dimX = firImg->get_width();
    int dimY = firImg->get_length();

    const auto diffData = diffImg->get_data();
    const auto firData  = firImg->get_data();

    for(int y=padding; y<dimY-padding; y++){
      for(int x=padding; x<dimX-padding; x++){
        if(isMax(firData,x,y,threashold)){
           separate(x,y,sliceNr,diffData);
        }
      }
    }

#ifdef SAVE
    toSaveQueue.push_back(findPair);
#else
    delete findPair;
    delete firImg;
    delete diffImg;
#endif        
  }

  qDebug() << globalWatch.elapsed() << "Estimator" << id << "Last image Searched";

  toFindQueue.close();
  roiQueue.finish(-1);

#ifdef LOG
  out << "Estimator " << id << "  Elapsed time " << globalWatch.elapsed() <<  "  Searching spots - num frames: "<< toFindQueue.getNumHandles() << "\n";
#endif

#ifdef SAVE
  toSaveQueue.finish(toFindQueue.getPops());
#endif

#ifdef CHAIN
  insertRoisInResultImage();
#endif

  emit finished(id);
}

bool Estimator::isMax( uint16_t const *const * data, int posX, int posY, double threashold)
{
  int midValue = data[posY][posX];
  if(midValue<threashold){
    return false;
  }

  int topValue = data[posY-1][posX];
  if(midValue==topValue){
    return false;
  }

  int leftValue = data[posY][posX-1];
  if(midValue==leftValue){
    return false;
  }

  for(int y=-1; y<=1; y++ ){
    int Y = posY+y;
    for(int x=-1; x<=1; x++ ){
      int surValue = data[Y][posX+x];
      if(midValue<surValue){
         return false;
      }
    }
  }

  return true;
}

void Estimator::separate(int posX, int posY, int sliceNr, const uint16_t *const*data)
{

  double meanbg = meanBgVec.at(sliceNr);

  Roi *roi = new Roi(posX-ROIRAD,posY-ROIRAD,sliceNr,meanbg,ROISIZE,ROISIZE);

  double cutoff = cutoffFactor*sqrt(meanbg);

  int QOld = 0;
  for(int y=0; y<ROISIZE; y++){
    for(int x=0; x<ROISIZE; x++){
      quint16 value = (quint16) data[posY-ROIRAD+y][posX-ROIRAD+x];
      value = (value>cutoff)? value-cutoff : 0;
      QOld += value;
      roi->setValue(value,x,y);
    }
  }

  int QNew = roi->cutEdges();

  if(QNew > (QOld * separateFactor)){
    roiQueue.push_back(roi);
  }else{
    deletedRois++;
    delete roi;
  }
}

void Estimator::estimate()
{

  resultQueue.setThreashold(1000);

  QTextStream out(&loggerFile);
#ifdef LOG
  out << globalWatch.elapsed() <<" "<< id <<" estimate start " << roiQueue.size() << " " << roiQueue.getNumHandles() << "\n";
#endif

  Roi *roi = nullptr;

  while(roiQueue.pop_front(roi))
  {
#ifdef LOG
    out << globalWatch.elapsed() <<" "<< id <<" estimate " << roiQueue.size() << " " << roiQueue.getNumHandles() << "\n";
#endif

    Roi::Result * res = roi->calc();
    delete roi;

    res->convertMetric(dataPixelSize);

    printResults(res);

    resultQueue.push_back(res);
  }

  roiQueue.close();
  resultQueue.finish(roiQueue.getPops());

#ifdef CHAIN
  generateSpotFromPendingResults();
#endif

  closeFiles();

  emit finished(id);
}

void Estimator::printResults(Roi::Result * res)
{
  QTextStream challengeOut(&challengeFile);
  QTextStream out(&resultFile);

  QMutexLocker locker(&estimateMutex);

  out << currResNr++    << "\t";
  out << res->QMax      << "\t";
  out << res->mx        << "\t";
  out << res->my        << "\t";
  out << sqrt(res->dx2) << "\t";
  out << sqrt(res->dy2) << "\t";
  out << sqrt(res->sx2) << "\t";

  out << sqrt(res->sy2) << "\t";
  out << res->gesQ      << "\t";
  out << res->sliceNr   << "\n";

  challengeOut << res->mx << ";";
  challengeOut << res->my << ";";
  challengeOut << 0       << "\n";
}

void Estimator::estimateGenerate()
{
  toPrintQueue.setThreashold(100);

  QTextStream out(&loggerFile);
#ifdef LOG
  out << globalWatch.elapsed() <<" "<< id<<" estimateGenerate start " << roiQueue.size() << " " << roiQueue.getNumHandles() << "\n";
#endif

  Roi *roi = nullptr;

  while(roiQueue.pop_front(roi))
  {

#ifdef LOG
    out << globalWatch.elapsed() <<" "<< id <<" estimateGenerate " << roiQueue.size() << " " << roiQueue.getNumHandles() << "\n";
#endif

    Roi::Result * res = roi->calc();
    delete roi;

    res->convertMetric(dataPixelSize);

    printResults(res);

    Roi * genRoi = generateSpot(res);

    toPrintQueue.push_back(genRoi);
  }

  roiQueue.close();
  toPrintQueue.finish(roiQueue.getPops());

#ifdef CHAIN
#ifdef SAVE
   saveStacks();
#endif
#endif

  emit finished(id);
}

void Estimator::saveStacks()
{
  qDebug() << "Saving Background and Filtered Stacks! Hold on...";

  QTextStream out(&loggerFile);
#ifdef LOG
  out << globalWatch.elapsed() <<" "<< id <<" save sleep " << toSaveQueue.size() << "\n";
#endif

  QPair< image16_ref*,image16_ref* > * savePair = nullptr;

  while(toSaveQueue.pop_front(savePair))
  {
    image16_ref * diffImg = savePair->first;
    image16_ref * firImg  = savePair->second;

#ifdef LOG
    out << globalWatch.elapsed() <<" "<< id <<" save " << toSaveQueue.size() << "\n";
#endif
    diffStack->append_image(*diffImg);
    firStack ->append_image(*firImg);

    delete savePair;
    delete diffImg;
    delete firImg;
  }

  toSaveQueue.close();

  emit finished(id);

  delete diffStack;
  delete firStack;
}

void Estimator::generateSpotFromPendingResults()
{
  QTextStream out(&loggerFile);
#ifdef LOG
  out << globalWatch.elapsed() <<" "<< id <<" generateSpot sleep " << resultQueue.size() << " " << resultQueue.getNumHandles() << "\n";
#endif

  Roi::Result * res = nullptr;

  while(resultQueue.pop_front(res))
  {
#ifdef LOG
    out << globalWatch.elapsed() <<" "<< id <<" generateSpot " << resultQueue.size() << " " << resultQueue.getNumHandles() << "\n";
#endif
    Roi * genRoi = generateSpot(res);

    toPrintQueue.push_back(genRoi);
  }

  resultQueue.close();
  toPrintQueue.finish(resultQueue.getPops());

#ifdef CHAIN
#ifdef SAVE
   saveStacks();
#endif
#endif
   emit finished(id);

}

void Estimator::insertRoi(Roi * roi)
{
  const auto pos =  roi->getGlobalPos();
  const int posX = pos.first;
  const int posY = pos.second;

  const auto roiSize = roi->getSize();
  const int dimX = roiSize.first;
  const int dimY = roiSize.second;

  QMutexLocker locker(&fillLokImgMutex);
  const int width  = resultImage->get_width();
  const int length = resultImage->get_length();

  uint16_t *const *data = resultImage->get_data();
  for(int y=0; y<dimY; y++){
    for(int x=0; x<dimX; x++){
      if(posX+x>0 && posX+x<width && posY+y>0 && posY+y<length){
          data[posY+y][posX+x] += roi->val(x,y);
      }
    }
  }

  delete roi;
}

void Estimator::insertRoisInResultImage()
{
  QTextStream out(&loggerFile);
#ifdef LOG
  out << globalWatch.elapsed() <<" "<< id <<" insertRois sleep " << toPrintQueue.size() << " " << toPrintQueue.getNumHandles() << "\n";
#endif

  Roi * roi = nullptr;

  while(toPrintQueue.pop_front(roi))
  {

#ifdef LOG
    out << globalWatch.elapsed() <<" "<< id <<" insertRois " << toPrintQueue.size() << " " << toPrintQueue.getNumHandles() << "\n";
#endif
    insertRoi(roi);

    if(toPrintQueue.getPops()%5000 == 100){

      emit printIntermediateImage(resultImage->copy());
    }
  }
  toPrintQueue.close();

#ifdef LOG
 loggerFile.close();
#endif

#ifndef SAVE
  usleep(1000);
  wakeAll();
#endif

  emit finished(id);
}


QString Estimator::saveResultImage()
{
  QString resultName = currFileName+"_lokimg.tiff";

  img_stack saveLokImgStack(resultName.toStdString(),"w");
  saveLokImgStack.append_image(*resultImage);

  delete resultImage;
  resultImage = nullptr;

  return resultName;
}


void Estimator::runSingleThreaded()
{
  initEstimatorStatics();

  read();

  filter();

  find();

  estimate();

  saveStacks();

  generateSpotFromPendingResults();

  insertRoisInResultImage();

  saveResultImage();

  closeFiles();
}

void Estimator::generateNewStack()
{
  QString fileName = QFileDialog::getSaveFileName(0, tr("Open File"),
                                                  QDir::currentPath(),
                                                  tr("TiffImages (*.tif *.tiff)"));

  img_stack newStack(fileName.toStdString().c_str(),"w");

  QTime midnight(0, 0, 0);
  qsrand(midnight.secsTo(QTime::currentTime()));

  int dimX = 200;
  int dimY = 200;

  for(int i=0; i<1000; i++){

    int nrSpots = qrand() % 5;
    image16_ref * newImage = fillNewImage(nrSpots,dimY,dimX,i);
    newStack.append_image(*newImage);
    delete newImage;
  }
}

image16_ref* Estimator::fillNewImage(int nrSpots, int length, int width, int dir_nr)
{
  image16_ref *newImage = new image16_ref(length,width,16,dir_nr);

  fillImageWithBackground(newImage);

  for(int i=0; i<nrSpots; i++){
    addNewSpot(newImage);
  }

  return newImage;
}

void Estimator::fillImageWithBackground(image16_ref *newImage)
{
  uint16_t *const *data = newImage->get_data();

  int length = newImage->get_length();
  int width  = newImage->get_width();

  for(int y=0; y<length; y++){
    for(int x=0; x<width; x++){
      uint16_t value = getPoissonRnd(100);
      data[y][x] = value;
    }
  }
}


void Estimator::addNewSpot(image16_ref *newImage)
{
  int length = newImage->get_length();
  int width  = newImage->get_width();

  double mx =  ((double)(qrand() % ((width-10)*100))) / 100.0;
  double my =  ((double)(qrand() % ((length-10)*100))) / 100.0;

  mx = mx + 5.0;
  my = my + 5.0;


  int posX = mx-3;
  int posY = my-3;

  mx -= posX;
  my -= posY;

  int QMax = 100 + (qrand() % 500);

  qDebug() << "(x/y)" << "(" << posX+mx << "/" << posY+my << ")" << "Max:" << QMax;

  Roi * roi = generateSpot(QMax,mx,my,1.2,7,7);

  insertSpot(newImage,roi,posX,posY);

  delete roi;
}

Roi * Estimator::generateSpot(Roi::Result * res)
{
  double widthX =  (sqrt(res->dx2)/lokImgPixelSize);
  double widthY =  (sqrt(res->dy2)/lokImgPixelSize);
  double meanwidth = (widthX+widthY)/2.0;

  int rad = 3 + ((meanwidth>1)? meanwidth*2 : 0);

  int globalX = res->mx/lokImgPixelSize - rad;
  int globalY = res->my/lokImgPixelSize - rad;

  delete res;

  int dimX = (rad*2)+1;
  int dimY = dimX;

  double mx = (dimX-1)/2;
  double my = mx;

  int Qmax = 1000.0/meanwidth;

  auto genRoi = generateSpot(Qmax,mx,my,meanwidth,dimX, dimY);

  genRoi->setGlobalPos(globalX,globalY);

  return genRoi;
}

Roi * Estimator::generateSpot(int Qmax, double mx, double my, double sigma, int dimX, int dimY)
{
  Roi * roi= new Roi(0,0,0,0,dimX,dimY);

  for(int y=0; y<dimY; y++){
    for(int x=0; x<dimX; x++){
      uint16_t value = expo2D(x,y,Qmax,mx,my,sigma);
      roi->setValue(value,x,y);
    }
  }
  return roi;
}

void Estimator::insertSpot(image16_ref *newImage, Roi *roi, int posX, int posY)
{
  const auto roiSize = roi->getSize();
  int dimX = roiSize.first;
  int dimY = roiSize.second;

  uint16_t *const *data = newImage->get_data();
  for(int y=0; y<dimY; y++){
    const int Y = posY + y;
    for(int x=0; x<dimX; x++){
      uint16_t value = roi->val(x,y);
      data[Y][posX+x] += value;
    }
  }
}

uint16_t Estimator::expo2D(int x, int y, int A ,double mx, double my, double sig)
{
    return A*exp(-0.5*(x-mx)*(x-mx)/((sig)*(sig)))*exp(-0.5*(y-my)*(y-my)/((sig)*(sig)));
}


uint16_t Estimator::getPoissonRnd(double mean)
{
  double L = exp(-mean);
  double p = 1.0;
  uint16_t k = 0;

  do {
    k++;
    p *= ((double)(qrand()%10000))/10000.0;
  } while (p > L);

  return k - 1;
}


void Estimator::printImg(image16_ref &image)
{
  uint16_t const *const * data = image.get_data();

  for(int y=0;y<10;y++){
    QString out;
    for(int x=0; x<10; x++){
      out += QString::number(data[y][x]) + "\t";
    }
    qDebug() << out;
  }
}


void Estimator::restartOthers()
{
  emit bgReady();
  emit firReady();
  emit roiReady();
  emit findReady();
  emit resultReady();
  emit spotReady();
}

void Estimator::wakeAll()
{
  toFilterQueue.wake();
  toFindQueue.wake();
  toSaveQueue.wake();
  toPrintQueue.wake();
  roiQueue.wake();
  resultQueue.wake();
}


void Estimator::logHeader()
{
  QDateTime currTime(QDateTime::currentDateTime());

  QTextStream out(&loggerFile);
  out << "##############################################\n";
  out << "### File: " << currFileName<< "\n";
  out << "### Date: " << currTime.toString("hh:mm:ss  dd/MM/yyyy") << "\n";
  out << "##############################################\n";
  out << "### Estimator parameters:\n";
  out << "### - Threashold factor = " << threasholdFactor<< "\n";
  out << "### - Cutoff factor     = " << cutoffFactor<< "\n";
  out << "### - Seperate factor   = " << separateFactor<< "\n";
  out << "##############################################";
  out << "### Data Parameters:\n";
  out << "### - Number of frames  = " << dimZ<< "\n";
  out << "### - Camera pixel size = " << dataPixelSize<< "\n";
  out << "### - Result pixel size = " << lokImgPixelSize<< "\n";
  out << "##############################################\n\n";
}
