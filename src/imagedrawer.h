#ifndef IMAGEDRAWER_H
#define IMAGEDRAWER_H

#include "imagerender.h"
#include "lokalizationthread.h"
#include "stackoverview.h"

#include <QMainWindow>
#include <QPrinter>
#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
class QComboBox;
class QSpinBox;
class QSlider;
class QProgressBar;
QT_END_NAMESPACE


//! [0]
class ImageDrawer : public QMainWindow
{
    Q_OBJECT

public:
    ImageDrawer(QString fileName);
    ImageDrawer();
    ~ImageDrawer();
    void setFileName(QString fileName){imgName = fileName;}

signals:
    void scaleSignal(double factor);
    void gotSettings(double camPixelsize,double resPixelSize,QString stackName);

public slots:
    void getSettings();
    QPair<double,double> getParameters();
    QPair<double,double> loadInitFile();

private slots:
    void open();
    void open(const QString &fileName);
    void print();
    void zoomIn();
    void zoomOut();
    void normalSize();
    void fitToWindow();
    void invertColors();
    void adjustWindowSize();
    void about();
    void test();
    void scaleImage(const QImage &image, double factor);
    void scaleImage(const QImage &image);
    void scaleImage(double factor);
    void loadImage();
    void loadImage(const QString & fileName, int sliceNr);

    void loadPixmap(const QImage &image, double factor);
    void updateLabel();
    void applySettings();
    void loadTiffImage();
    void loadImageStack();
    void showElapsedTime(int elapsedTime, int numSpots);
    void generateStackBox();
    void showStackBox(bool show){stackBox->setVisible(show);}

    void loadStack();
    void loadBackroundStack();
    void loadFilteredSack();
    void loadOverviewImage();
    void loadLokImg();

    void stackValueChanged();

    void loadPrevImage();
    void loadNextImage();

private:
    void generateLayout();
    void generateSettingsWidget();

    void createActions();
    void createMenus();
    void updateActions();
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void scroll(int deltaX, int deltaY);
    void scroll(QScrollBar *scrollBar, int delta);
    QRgb toRGB(int value);

protected:
    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void changeEvent(QEvent* event);
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    double scaleFactor;
    double minFactor;
    QString imgName;
    QString stackName;

    ImageRender render;
    LokalizationThread lokalizer;
    StackOverview oviewer;

#ifndef QT_NO_PRINTER
    QPrinter printer;
#endif

    QAction *openAct;
    QAction *printAct;
    QAction *settingsAct;
    QAction *exitAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *normalSizeAct;
    QAction *fitToWindowAct;
    QAction *invertColorsAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *testAct;

    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;

    QImage origImage;
    QPoint pixmapOffset;
    QPoint lastDragPos;

    QWidget *settingsWidget;
    QComboBox *separateBox;
    QSpinBox *dataPixelSizeBox;
    QSpinBox *imagePixelSizeBox;
    QSpinBox *thresholdBox;
    QSpinBox *cutoffBox;

    int currentWidth;
    int currentHeight;

    bool enableZoom;
    bool adjustWSize;
    bool init;

    QLabel *fileLbl;
    QLabel *numPointsLbl;
    QLabel *meanErrorLbl;
    QLabel *pixelSizeLbl;
    QLabel *imgPixelSizeLbl;
    QLabel *elapsedTimeLbl;

    QGroupBox *stackBox;

    QProgressBar * progressBar;

    QString stackFileName;
    QLabel *stackInfoLabel;
    QLabel *cntLbl;
    QSlider *stackSlider;
    QGroupBox *sliderBox;
};
//! [0]

#endif // IMAGEDRAWER_H
