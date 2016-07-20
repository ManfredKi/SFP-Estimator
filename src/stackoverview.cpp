#include "ImageStack/img_stack.hpp"

#include "stackoverview.h"
#include <QFileInfo>

StackOverview::StackOverview(QObject *parent) :
    QThread(parent)
{
    abort = false;
}


StackOverview::~StackOverview()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}


int StackOverview::getNumSlizes(const QString & stackName)
{
  img_stack tiffStack(stackName.toStdString(),std::string("r"));
  return tiffStack.img_count();
}


QString StackOverview::openImageInStack(const QString &stackName, int sliceNr)
{
  img_stack tiffStack(stackName.toStdString(),std::string("r"));

  int dimZ = tiffStack.img_count();
  if(dimZ<sliceNr) return "";

  image16_ref img =  tiffStack.get_image(sliceNr);

  QFileInfo info(stackName);

  QString path = info.absolutePath();
  QString name = info.baseName();
  QString imageName = path+"/"+name+"_slice.tiff";
  img_stack ovImageStack(imageName.toStdString().c_str(),"w");
  ovImageStack.append_image(img);

  return imageName;
}


void StackOverview::genOverview(const QString & imageName)
{
    QMutexLocker locker(&mutex);

    this->stackName = imageName;

    if (!isRunning()) {
        start(LowPriority);
    }

    condition.wakeOne();
}


void StackOverview::run()
{
    forever {

        mutex.lock();
        QString stackName = this->stackName;
        mutex.unlock();

        QString ovImageName = generateOverview(stackName);

        emit ovImageStored(ovImageName);

        mutex.lock();
        condition.wait(&mutex);
        if (abort){
            return;
        }
        mutex.unlock();
    }
}


QString StackOverview::generateOverview(const QString &stackName)
{
      img_stack tiffStack(stackName.toStdString(),std::string("r"));

      int dimZ = tiffStack.img_count();
      if(dimZ>50)
          dimZ = 49;

      image16_ref img =  tiffStack.overview_img(0,dimZ);

      QFileInfo info(stackName);

      QString path = info.absolutePath();
      QString name = info.baseName();
      QString imageName = path+"/"+name+"_ov.tiff";
      img_stack ovImageStack(imageName.toStdString().c_str(),"w");
      ovImageStack.append_image(img);

      return imageName;
}
