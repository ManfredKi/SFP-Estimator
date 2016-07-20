#ifndef ROI_H
#define ROI_H

#include <QPair>

class Roi
{
  public:
    explicit Roi(int _posX, int _posY, int _sliceNr=0, int _meanbg=0, int _dimX=7, int _dimY=7);
    Roi(Roi const& roi);
    Roi();
    Roi& operator =(Roi const& roi);
    ~Roi();

    void setData(quint16 *newData);
    void setValue(quint16 value,int x, int y);
    void setGlobalPos(int _posX, int _posY);
    inline QPair<int,int> getGlobalPos(){return QPair<int,int> (posX,posY);}
    inline QPair<int,int> getSize(){return QPair<int,int>(dimX,dimY);}

    struct Result{
        void convertMetric(const double dataPixelSize){
          const double pxSize2 = dataPixelSize*dataPixelSize;
          mx  *= dataPixelSize;
          my  *= dataPixelSize;
          sx2 *= pxSize2;
          sy2 *= pxSize2;
          dx2 *= pxSize2;
          dy2 *= pxSize2;
        }

        int gesQ;
        int QMax;
        double mx;
        double my;
        double dx2;
        double dy2;
        double sx2;
        double sy2;
        int sliceNr;
    };

    quint16 val(int x,int y) const;
    void print() const;
    Result * calc() const;
    int cutEdges();
    int getQ();

  private:
    void cutX();
    void cutY();

  private:
    int dimX;      //defines width of ROI
    int dimY;      //defines length of ROI
    int sliceNr;   //defines slice number in global image stack
    double meanbg; //value needed for error calculation
    int posX;      //defines left boarder of ROI in global Image
    int posY;      //defines upper boarder of ROI in global Image
    quint16 *data;
    
};

#endif // ROI_H
