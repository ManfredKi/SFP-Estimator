#include "qtfiles.h"

#include <cmath>
#include "roi.h"

Roi::Roi() :
    dimX(7),dimY(7),
    sliceNr(0),meanbg(0),
    posX(0),posY(0)
{
  const auto dim = dimX*dimY;

  data = new quint16[dim];
  for(int i=0; i<dim; i++)
  {
    data[i]=0;
  }
}

Roi::Roi(int _posX, int _posY, int _sliceNr, int _meanbg, int _dimX, int _dimY) :
    dimX(_dimX),dimY(_dimY),
    sliceNr(_sliceNr),meanbg(_meanbg),
    posX(_posX),posY(_posY)
{
  const auto dim = dimX*dimY;
  data = new quint16[dim];
  for(int i=0; i<dim; i++)
  {
    data[i]=0;
  }
}

Roi::Roi(Roi const& roi)
{
  dimX = roi.dimX;
  dimY = roi.dimY;

  data = new quint16[dimX*dimY];

  for(int i=0; i<dimX; ++i){
    data[i]=roi.data[i];
  }
}

Roi& Roi::operator =(Roi const& roi)
{
  dimX = roi.dimX;
  dimY = roi.dimY;

  data = new quint16[dimX*dimY];

  for(int i=0; i<dimX; ++i){
    data[i]=roi.data[i];
  }

  return *this;
}

Roi::~Roi()
{
    delete [] data;
    //qDebug() << "Roi deleted!";
}

void Roi::setData(quint16 *newData)
{
    if(data){
      delete [] data;
    }
    data=newData;
}

void Roi::setGlobalPos(int _posX, int _posY)
{
    posX=_posX;
    posY=_posY;
}

void Roi::setValue(quint16 value, int x, int y)
{
  if((y<dimY)&&(x<dimX)){
    data[y*dimX+x]=value;
  }else{
    qDebug() <<"ROI:dimError set" << x <<y;
  }
}

quint16 Roi::val(int x, int y) const
{
    if((y<dimY)&&(x<dimX))
        return data[y*dimX+x];

    qDebug() <<"ROI:dimError get" << x <<y;
    return 0;
}

void Roi::print() const
{
  for(int y=0; y<dimY; ++y){
    QString output;
    for(int x=0; x<dimX; ++x){
      output += QString::number(val(x,y))+ " ";
    }
    qDebug()<<output;
  }
}

Roi::Result *Roi::calc() const
{
  Result *res = new Result;

  int mxacc  = 0;
  int myacc  = 0;
  int sxacc = 0;
  int syacc = 0;
  int Qacc  = 0;
  int QMax  = 0;

  int dxacc = 0;
  int dyacc = 0;

  int dx2acc = 0;
  int dy2acc = 0;

  int nacc = 0;

  for(double y=0; y<dimX; y++){
    for(double x=0; x<dimX; x++){
      int value = (int)val(x,y);

      if(value>0){

        nacc++;

        dx2acc += x*x;
        dy2acc += y*y;

        dxacc += x;
        dyacc += y;

        Qacc += value;
        if(value>QMax){
          QMax = value;
        }

        int xval = value*x;
        mxacc +=xval;
        sxacc += x*xval;

        int yval = value*y;
        myacc +=yval;
        syacc += y*yval;
      }
    }
  }

  res->my  = ((double)myacc)/Qacc;
  res->sy2 = ((double)syacc)/Qacc - (res->my*res->my);
  double dy1 = res->sy2;
  double dy2 = 1.0/12.0;
  double dy3 = meanbg/Qacc*(dy2acc + (res->my * (nacc * res->my - 2*dyacc)));

  res->dy2 = (dy1+dy2+dy3)/Qacc;

  res->my += posY;

  res->mx  = ((double)mxacc)/Qacc;
  res->sx2 = ((double)sxacc)/Qacc - (res->mx*res->mx);

  double dx1 = res->sx2;
  double dx2 = 1.0/12.0;
  double dx3 = meanbg/Qacc*(dx2acc - res->mx * (2*dxacc - nacc * res->mx));

  res->dx2 = (dx1+dx2+dx3)/Qacc;

  res->mx += posX;

  res->gesQ = Qacc;
  res->QMax = QMax;
  res->sliceNr = sliceNr;

  return res;
}


int Roi::cutEdges()
{
  //print();
  cutX();
  cutY();
  cutX();
  return getQ();
}

void Roi::cutX()
{
  int mid  = (dimX-1)/2;

  for(int y=0; y<dimY; y++){
    int row = y*dimX;
    int comp = 1;
    for(int x=0; x<dimX; x++){
      if(x==mid-1){
        x+=3;
        comp ++;
      }
      int value = data [row+x];
      int compData = data[row+comp];
      int noise = (compData==0)? 0 : (compData<16)? 3 : sqrt(compData) ;
      data[row+x] = ((value+noise)> compData) ? 0 : value ;
      comp++;
    }
  }
  //print();
}

void Roi::cutY()
{
  int mid  = (dimY-1)/2;
  int comp = dimX;

  for(int y=0; y<dimY; y++){
    if(y==mid-1){
      y+=3;
      comp += dimX;
    }
    int row = y*dimX;
    for(int x=0; x<dimX; x++){
      int value = data[row+x];
      int compData = data[x+comp];
      int noise = (compData==0)? 0 : (compData<16)? 3 : sqrt(compData) ;
      data[row+x] = ((value+noise)> compData) ? 0 : value ;
    }
    comp += dimX;
  }
  //print();
}

int Roi::getQ()
{
  int qacc=0;
  for(int y=0; y<dimY; y++){
    int row = y*dimX;
    for(int x=0; x<dimX; x++){
      qacc += data[row+x];
    }
  }

  return qacc;
}
