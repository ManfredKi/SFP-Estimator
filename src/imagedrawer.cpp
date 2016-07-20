/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QComboBox>
#include <QMenu>
#include <QFormLayout>
#include <QMenuBar>
#include <QSpinBox>
#include <QMessageBox>
#include <QPrintDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>

#include <QtGui>
#include "qtfiles.h"

#include "imagerender.h"
#include "imagedrawer.h"
#include "estimator.h"

const int ScrollStep = 20;

ImageDrawer::ImageDrawer() :
    QMainWindow(NULL)
{
    generateLayout();
    open("background.tiff");
}

ImageDrawer::ImageDrawer(QString fileName) :
    QMainWindow(NULL),
    imgName(fileName)
{
    generateLayout();
    loadImage();
}

ImageDrawer::~ImageDrawer()
{
    delete settingsWidget;
}

void ImageDrawer::generateLayout()
{
    init = false;
    adjustWSize = true;
    imageLabel = new QLabel;
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);

    QWidget * mainWidget = new QWidget;
    QGridLayout *mainLayout = new QGridLayout;

    mainWidget->setLayout(mainLayout);

    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);

    QGroupBox *bottomBox = new QGroupBox("");
    QHBoxLayout * bottomLayout = new QHBoxLayout;
    bottomBox->setLayout(bottomLayout);

    QGroupBox *infoBox = new QGroupBox("FileInfo");
    infoBox->setMaximumWidth(250);
    QGridLayout * infoLayout = new QGridLayout;
    infoBox->setLayout(infoLayout);



    fileLbl = new QLabel("nA");
    numPointsLbl = new QLabel("nA");
    meanErrorLbl = new QLabel("nA");
    pixelSizeLbl = new QLabel("nA");
    imgPixelSizeLbl = new QLabel("nA");
    elapsedTimeLbl = new QLabel("nA");

    int line = 0;    

    infoLayout->addWidget(new QLabel("#Points"),line,0);
    infoLayout->addWidget(numPointsLbl,line++,1);

    infoLayout->addWidget(new QLabel("MeanWidth"),line,0);
    infoLayout->addWidget(meanErrorLbl,line++,1);

    infoLayout->addWidget(new QLabel("ImgPixelSize"),line,0);
    infoLayout->addWidget(pixelSizeLbl,line++,1);

    infoLayout->addWidget(new QLabel("ResPixelSize"),line,0);
    infoLayout->addWidget(imgPixelSizeLbl,line++,1);

    infoLayout->addWidget(new QLabel("ElapsedTime"),line,0);
    infoLayout->addWidget(elapsedTimeLbl,line++,1);

    infoLayout->addWidget(new QLabel("File"),line,0);
    infoLayout->addWidget(fileLbl,line++,1);

    generateStackBox();

    bottomLayout->addWidget(infoBox);
    bottomLayout->addWidget(stackBox);
    bottomLayout->addStretch();

    QGroupBox * functionInfoBox = new QGroupBox("Details");
    QVBoxLayout * functionLayout = new QVBoxLayout;
    functionInfoBox->setLayout(functionLayout);

    progressBar = new QProgressBar();
    progressBar->setMaximum(100);
    progressBar->setValue(0);

    mainLayout->addWidget(functionInfoBox,0,0,5,1);
    mainLayout->addWidget(scrollArea,0,1,5,5);
    mainLayout->addWidget(bottomBox,5,0,1,6);
    mainLayout->addWidget(progressBar,6,0,1,6);

    setCentralWidget(mainWidget);

    generateSettingsWidget();


    QPushButton *restartBtn = new QPushButton("Restart");
    QGroupBox *localizationBox = new QGroupBox("Image Parameter");
    QFormLayout *localizationLayout = new QFormLayout;
    localizationBox->setLayout(localizationLayout);

    dataPixelSizeBox = new QSpinBox();
    dataPixelSizeBox->setRange(1,10000);
    dataPixelSizeBox->setValue(100);

    imagePixelSizeBox = new QSpinBox();
    imagePixelSizeBox->setRange(1,1000);
    imagePixelSizeBox->setValue(10);

    localizationLayout->addRow(new QLabel("Define Pixel sizes [nm]"));
    localizationLayout->addRow("Image Data", dataPixelSizeBox);
    localizationLayout->addRow("LocImgimage", imagePixelSizeBox);

    functionLayout->addWidget(localizationBox);
    functionLayout->addWidget(settingsWidget);
    functionLayout->addWidget(restartBtn);
    functionLayout->addStretch();

    createActions();
    createMenus();

    setWindowTitle(tr("FPS Estimator"));
    currentWidth = 517;
    currentHeight = 603;

    resize(517, 600);

    scaleFactor = 1.0;
    minFactor   = 0.2;
    enableZoom  = false;

    connect(&lokalizer,SIGNAL(progress(int)),progressBar,SLOT(setValue(int)));
    connect(&lokalizer,SIGNAL(numImages(int)),progressBar,SLOT(setMaximum(int)));

    connect(restartBtn,SIGNAL(clicked()),this,SLOT(getSettings()));

    connect(&oviewer,SIGNAL(ovImageStored(QString)),this,SLOT(open(QString)));
    connect(&oviewer,SIGNAL(ovImageStored(QString)),this,SLOT(getSettings()));
    connect(&render,SIGNAL(renderedImage(QImage,double)),this,SLOT(loadPixmap(QImage,double)));
    connect(&render,SIGNAL(loadedImage(QImage)),this,SLOT(scaleImage(QImage)));
    connect(&lokalizer,SIGNAL(imageSaved(QString)),this,SLOT(open(QString)));
    connect(&lokalizer,SIGNAL(finishTime(int,int)),this,SLOT(showElapsedTime(int,int)));
    connect(&lokalizer,SIGNAL(printIntermediateImage(QString)),this,SLOT(open(QString)));
}
//! [1]
//!
void ImageDrawer::generateStackBox()
{
  stackBox = new QGroupBox("Stack Operations");
  stackBox->hide();

  QGridLayout *stackMainLayout = new QGridLayout;
  stackBox->setLayout(stackMainLayout);
  QPushButton *stackBtn = new QPushButton("Show Stack");
  QPushButton *backgroundBtn = new QPushButton("Show Background");
  QPushButton *filterBtn = new QPushButton("Show Signals(Filtered)");
  QPushButton *overViewBtn = new QPushButton("Show Overview");
  QPushButton *lokImgBtn = new QPushButton("Show Lok-Image");


  sliderBox = new QGroupBox;
  QHBoxLayout *sliderBoxLayout = new QHBoxLayout;
  sliderBox->setLayout(sliderBoxLayout);

  stackInfoLabel = new QLabel("Stack");
  cntLbl = new QLabel("0");
  stackSlider = new QSlider;
  stackSlider->setOrientation(Qt::Horizontal);
  stackSlider->setRange(0,1000);
  stackSlider->setValue(10);

  QPushButton *downBtn = new QPushButton("down");
  QPushButton *upBtn = new QPushButton("up");
  sliderBox->hide();

  sliderBoxLayout->addWidget(cntLbl);
  sliderBoxLayout->addWidget(stackSlider);
  sliderBoxLayout->addWidget(downBtn);
  sliderBoxLayout->addWidget(upBtn);

  stackMainLayout->addWidget(stackBtn,0,0,1,3);
  stackMainLayout->addWidget(backgroundBtn,0,3,1,3);
  stackMainLayout->addWidget(filterBtn,0,6,1,3);
  stackMainLayout->addWidget(overViewBtn,0,9,1,3);
  stackMainLayout->addWidget(lokImgBtn,0,12,1,3);


  stackMainLayout->addWidget(stackInfoLabel,1,0,1,15);
  stackMainLayout->addWidget(sliderBox,2,0,1,15);

  connect(stackBtn,SIGNAL(clicked()),this,SLOT(loadStack()));
  connect(backgroundBtn,SIGNAL(clicked()),this,SLOT(loadBackroundStack()));
  connect(filterBtn,SIGNAL(clicked()),this,SLOT(loadFilteredSack()));
  connect(overViewBtn,SIGNAL(clicked()),this,SLOT(loadOverviewImage()));
  connect(lokImgBtn,SIGNAL(clicked()),this,SLOT(loadLokImg()));

  connect(stackSlider,SIGNAL(valueChanged(int)),this,SLOT(stackValueChanged()));
  connect(downBtn,SIGNAL(clicked()),this,SLOT(loadPrevImage()));
  connect(upBtn,SIGNAL(clicked()),this,SLOT(loadNextImage()));
}

void ImageDrawer::stackValueChanged()
{
  const int value = stackSlider->value();
  cntLbl->setText(QString::number(value));
  loadImage(stackFileName,value);
}

void ImageDrawer::loadStack()
{
  if(fileLbl->text().isEmpty() || fileLbl->text() == "nA") return;

  QString stackName = fileLbl->text();
  QFileInfo info(stackName);

  stackFileName = stackName;

  sliderBox->show();

  stackInfoLabel->setText("Stack " + info.fileName());
  cntLbl->setText("0");
  stackSlider->setValue(0);
}

void ImageDrawer::loadBackroundStack()
{
  if(fileLbl->text().isEmpty() || fileLbl->text() == "nA") return;

  QString stackName = fileLbl->text();
  QFileInfo info(stackName);
  stackFileName = info.absolutePath() + "/" + info.baseName() + "_bg.tif";

  stackSlider->show();

  stackInfoLabel->setText("Stack " + info.baseName() + "_bg.tif");
  cntLbl->setText("0");
  stackSlider->setValue(0);
}


void ImageDrawer::loadFilteredSack()
{
  if(fileLbl->text().isEmpty() || fileLbl->text() == "nA") return;

  QString stackName = fileLbl->text();
  QFileInfo info(stackName);
  stackFileName = info.absolutePath() + "/" + info.baseName() + "_fir.tif";

  stackSlider->show();

  stackInfoLabel->setText("Stack " + info.baseName() + "_fir.tif");
  cntLbl->setText("0");
  stackSlider->setValue(0);
}


void ImageDrawer::loadOverviewImage()
{
  QString stackName = fileLbl->text();
  QFileInfo info(stackName);
  render.loadTiffImage(info.absolutePath() + "/" + info.baseName() + "_ov.tiff");
}


void ImageDrawer::loadLokImg()
{
  QString stackName = fileLbl->text();
  QFileInfo info(stackName);
  render.loadTiffImage(info.absolutePath() + "/" + info.baseName() + "_lokimg.tiff");
}

void ImageDrawer::loadPrevImage()
{
  int newValue = stackSlider->value()-1;
  stackSlider->setValue(newValue);
  cntLbl->setText(QString::number(newValue));
}

void ImageDrawer::loadNextImage()
{
  int newValue = stackSlider->value()+1;
  stackSlider->setValue(newValue);
  cntLbl->setText(QString::number(newValue));
}

void ImageDrawer::generateSettingsWidget()
{
    settingsWidget = new QWidget;
    QGridLayout *settingsLayout = new QGridLayout;

    settingsWidget->setLayout(settingsLayout);

    separateBox = new QComboBox();
    separateBox->addItem("0.9");
    separateBox->addItem("0.8");
    separateBox->addItem("0.7");
    separateBox->addItem("0.6");
    separateBox->addItem("0.5");
    separateBox->addItem("0.4");
    separateBox->setCurrentIndex(2);
    separateBox->setToolTip("Minimum ratio of remaining intensity after separation to origin intensity in roi\n- default 0.7");
    thresholdBox = new QSpinBox();
    thresholdBox->setRange(1,50);
    thresholdBox->setValue(2);
    thresholdBox->setToolTip("Threashold over background: factor * sqrt(meanbg)\n- default: 2");
    cutoffBox = new QSpinBox();
    cutoffBox->setRange(1,4);
    cutoffBox->setValue(2);
    cutoffBox->setToolTip("Factor for cutoff: value - factor * sqrt(meanbg)\n- default: 2");

    settingsLayout->addWidget(new QLabel("Separate Factor"),0,0);
    settingsLayout->addWidget(separateBox,0,1);

    settingsLayout->addWidget(new QLabel("Threshold Factor"),1,0);
    settingsLayout->addWidget(thresholdBox,1,1);

    settingsLayout->addWidget(new QLabel("Cutoff Factor"),2,0);
    settingsLayout->addWidget(cutoffBox,2,1);

    QPushButton *applyBtn = new QPushButton("Apply Settings");

    settingsLayout->addWidget(applyBtn,3,0,1,2);

    //settingsWidget->hide();

    connect(applyBtn,SIGNAL(clicked()),this,SLOT(applySettings()));

}

void ImageDrawer::getSettings()
{
    QFileInfo info(stackName);

    QFileInfo initInfo( info.absolutePath() + "/parameters.ini");

    QPair<double,double> params;
    if(!initInfo.exists()){
        params = getParameters();
    }else{
        params = loadInitFile();
    }

    double camPixelSize = params.first;
    double resPixelSize = params.second;

    progressBar->setValue(0);
    progressBar->show();

#ifdef SAVE
    int numSlizes = StackOverview::getNumSlizes(stackName);
    stackSlider->setRange(0,numSlizes-1);
    stackBox->show();
#endif

    lokalizer.startEstimator(camPixelSize,resPixelSize,stackName);
}

QPair<double,double> ImageDrawer::getParameters()
{
  double dataPixelSize =  dataPixelSizeBox->value();
  double lokImgPixelSize = imagePixelSizeBox->value();

  pixelSizeLbl->setText(QString::number(dataPixelSize)+ " nm");
  imgPixelSizeLbl->setText(QString::number(lokImgPixelSize)+ " nm");
  return QPair<double,double>(dataPixelSize,lokImgPixelSize);
}

QPair<double,double> ImageDrawer::loadInitFile()
{
  QFileInfo info(imgName);
  QFile initFile(info.absolutePath() + "/parameters.ini");
  if (!initFile.open(QIODevice::ReadOnly | QIODevice::Text)){
      qDebug() << "File init.txt not found!";
      return getParameters();
  }

  QTextStream in(&initFile);
  QPair<double,double> params;
  int found = 0;
  while (!in.atEnd()) {
    QString line = in.readLine();
    if(line.left(15)== "CameraPixelSize"){
      int cameraPixelSize = line.section("\t",1,1).toInt();
      params.first = (double)cameraPixelSize;
      found++;
    }else if(line.left(14)== "ImagePixelSize"){
      double imagePixelSize = line.section("\t",1,1).toDouble();
      params.second = imagePixelSize;
      found++;
    }
    if(found>1){
      initFile.close();
      return params;
    }
  }

  initFile.close();
  return QPair<double,double>(0.0,0.0);
}


void ImageDrawer::open()
//! [1] //! [2]
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open File"), tr("C:/Users/Manfred/Qt/LokMik/Data"));
    if (!fileName.isEmpty()) {
      imgName = fileName;
      fileLbl->setText(fileName);
    }

    loadImage();
}

void ImageDrawer::open(const QString & fileName)
{
    QImage image(fileName);
    if (image.isNull()) {
        QMessageBox::information(this, tr("Image Viewer"),
                                 tr("Cannot load %1.").arg(fileName));
        return;
    }
    imgName = fileName;
    loadImage();
 }

//! [4]

//! [5]
void ImageDrawer::print()
//! [5] //! [6]
{
    Q_ASSERT(imageLabel->pixmap());
#ifndef QT_NO_PRINTER
//! [6] //! [7]
    QPrintDialog dialog(&printer, this);
//! [7] //! [8]
    if (dialog.exec()) {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        QSize size = imageLabel->pixmap()->size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(imageLabel->pixmap()->rect());
        painter.drawPixmap(0, 0, *imageLabel->pixmap());
    }
#endif
}
//! [8]

//! [9]
void ImageDrawer::zoomIn()
//! [9] //! [10]
{
    adjustWSize = true;
    scaleImage(origImage, scaleFactor * 1.25);
}

void ImageDrawer::zoomOut()
{
    adjustWSize = true;
    scaleImage(origImage, scaleFactor * 0.8);
}

//when render returns loadPixmap is called
void ImageDrawer::scaleImage(double factor)
{
    render.scaleImage(factor,imageLabel->pixmap()->toImage());
}

void ImageDrawer::scaleImage(const QImage& image, double factor)
{
    render.scaleImage(factor,image);
}

void ImageDrawer::scaleImage(const QImage& image)
{
    origImage = image;
    double factor = ((double)scrollArea->height())/origImage.height();
    render.scaleImage(factor,origImage);
}

//! [10] //! [11]
void ImageDrawer::normalSize()
//! [11] //! [12]
{
    imageLabel->adjustSize();
    scaleImage(origImage,minFactor);
}
//! [12]

//! [13]
void ImageDrawer::fitToWindow()
//! [13] //! [14]
{
    bool fitToWindow = fitToWindowAct->isChecked();
    scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow) {
        normalSize();
    }

    updateActions();
}
//! [14]

void ImageDrawer::invertColors()
{
    origImage.invertPixels(QImage::InvertRgb);
    scaleImage(origImage, scaleFactor);
}

//! [15]
void ImageDrawer::about()
//! [15] //! [16]
{
    QMessageBox::about(this, tr("About Image Viewer"),
            tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
               "and QScrollArea to display an image. QLabel is typically used "
               "for displaying a text, but it can also display an image. "
               "QScrollArea provides a scrolling view around another widget. "
               "If the child widget exceeds the size of the frame, QScrollArea "
               "automatically provides scroll bars. </p><p>The example "
               "demonstrates how QLabel's ability to scale its contents "
               "(QLabel::scaledContents), and QScrollArea's ability to "
               "automatically resize its contents "
               "(QScrollArea::widgetResizable), can be used to implement "
               "zooming and scaling features. </p><p>In addition the example "
               "shows how to use QPainter to print an image.</p>"));
}
//! [16]

//! [25]
void ImageDrawer::adjustScrollBar(QScrollBar *scrollBar, double factor)
//! [25] //! [26]
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}
//! [26]

void ImageDrawer::scroll(QScrollBar *scrollBar, int delta)
{
    scrollBar->setValue(int(scrollBar->value() + delta));
}

void ImageDrawer::scroll(int deltaX, int deltaY)
{
    scroll(scrollArea->horizontalScrollBar(),deltaX);
    scroll(scrollArea->verticalScrollBar(),deltaY);
}

void ImageDrawer::updateLabel()
{
    adjustScrollBar(scrollArea->horizontalScrollBar(), scaleFactor);
    adjustScrollBar(scrollArea->verticalScrollBar(), scaleFactor);

    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > minFactor);
}
//! [17]
void ImageDrawer::createActions()
//! [17] //! [18]
{
    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    printAct = new QAction(tr("&Print..."), this);
    printAct->setShortcut(tr("Ctrl+P"));
    printAct->setEnabled(false);
    connect(printAct, SIGNAL(triggered()), this, SLOT(print()));

    settingsAct = new QAction(tr("&Settings..."), this);
    settingsAct->setShortcut(tr("Ctrl+S"));
    settingsAct->setEnabled(true);
    connect(settingsAct, SIGNAL(triggered()), settingsWidget, SLOT(show()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    zoomInAct = new QAction(tr("Zoom &In (25%)"), this);
    zoomInAct->setShortcut(tr("Ctrl++"));
    zoomInAct->setEnabled(false);
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

    zoomOutAct = new QAction(tr("Zoom &Out (25%)"), this);
    zoomOutAct->setShortcut(tr("Ctrl+-"));
    zoomOutAct->setEnabled(false);
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

    normalSizeAct = new QAction(tr("&Normal Size"), this);
    normalSizeAct->setShortcut(tr("Ctrl+S"));
    normalSizeAct->setEnabled(false);
    connect(normalSizeAct, SIGNAL(triggered()), this, SLOT(normalSize()));

    fitToWindowAct = new QAction(tr("&Fit to Window"), this);
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setChecked(false);
    fitToWindowAct->setShortcut(tr("Ctrl+F"));
    connect(fitToWindowAct, SIGNAL(triggered()), this, SLOT(fitToWindow()));

    invertColorsAct = new QAction(tr("&Invert Colors"), this);
    invertColorsAct->setEnabled(false);
    invertColorsAct->setCheckable(false);
    invertColorsAct->setShortcut(tr("Ctrl+I"));
    connect(invertColorsAct, SIGNAL(triggered()), this, SLOT(invertColors()));

    testAct = new QAction(tr("&Test"), this);
    connect(testAct, SIGNAL(triggered()), this, SLOT(test()));

    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}
//! [18]

//! [19]
void ImageDrawer::createMenus()
//! [19] //! [20]
{
    fileMenu = new QMenu(tr("&File"), this);
    fileMenu->addAction(openAct);
    fileMenu->addAction(printAct);
    fileMenu->addAction(settingsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    viewMenu = new QMenu(tr("&View"), this);
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addAction(normalSizeAct);
    viewMenu->addSeparator();
    viewMenu->addAction(fitToWindowAct);
    viewMenu->addAction(invertColorsAct);

    helpMenu = new QMenu(tr("&Help"), this);
    helpMenu->addAction(testAct);
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(viewMenu);
    menuBar()->addMenu(helpMenu);
}
//! [20]

//! [21]
void ImageDrawer::updateActions()
//! [21] //! [22]
{
    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}
//! [22]

void ImageDrawer::test()
{
    qDebug() << "LabelWidth:" << imageLabel->width();
    qDebug() << "AreaWidth:" << scrollArea->width();
    qDebug() << "WindowWidth:" << this->width();
    qDebug() << "CurrentWidth:" << currentWidth;

    Estimator::wakeAll();
}

void ImageDrawer::loadImage()
{
    if (!imgName.isEmpty()) {
        if(imgName.right(4)=="tiff"){
            loadTiffImage();
        }else if(imgName.right(3)=="tif"){
            stackName = imgName;
            loadImageStack();
        }        
    }
}

void ImageDrawer::loadImage(const QString & fileName, int sliceNr)
{
  const QString sliceFileName = StackOverview::openImageInStack(fileName,sliceNr);
  open(sliceFileName);
}

void ImageDrawer::loadImageStack()
{
    oviewer.genOverview(imgName);
}

void ImageDrawer::loadTiffImage()
{
    render.loadTiffImage(imgName);
}


void ImageDrawer::loadPixmap(const QImage& newImage, double factor)
{
    scaleFactor = factor;
    imageLabel->setPixmap(QPixmap::fromImage(newImage));
    scrollArea->setWidget(imageLabel);
   //! [3] //! [4]    

    printAct->setEnabled(true);
    fitToWindowAct->setEnabled(true);
    invertColorsAct->setEnabled(true);

    updateActions();

    if (!fitToWindowAct->isChecked()){
        imageLabel->adjustSize();
    }

    adjustWindowSize();

    updateLabel();

    show();
}

void ImageDrawer::adjustWindowSize()
{
    if(true){
        init=false;

        int newWidth  = imageLabel->width()+2;
        int newHeight = imageLabel->height()+28;

        if(newWidth > currentWidth) newWidth = currentWidth;
        if(newHeight > currentHeight) newHeight = currentHeight;

    //    resize(newWidth,newHeight);

        adjustWSize = false;
    }
}


//! [10]
void ImageDrawer::resizeEvent(QResizeEvent * /* event */)
{
    if (!fitToWindowAct->isChecked() && init){
        scaleImage(origImage,((double)scrollArea->height())/origImage.height());
    }
    if(init){
        currentWidth = this->width();
        currentHeight = this->height();
    }
    init = true;
}
//! [10]

//! [11]
void ImageDrawer::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Y:
        zoomIn();
        break;
    case Qt::Key_X:
        zoomOut();
        break;
    case Qt::Key_Plus:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    case Qt::Key_Left:        
        scroll(-ScrollStep,0);
        break;
    case Qt::Key_Right:
        scroll(ScrollStep,0);
        break;
    case Qt::Key_Down:
        scroll(0,-ScrollStep);
        break;
    case Qt::Key_Up:
        scroll(0,ScrollStep);
        break;
    case Qt::Key_Control:
        enableZoom = true;
        qDebug()<< "Enable zoom";
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}
//! [11]

void ImageDrawer::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Control:
        enableZoom = false;
        qDebug()<< "Disable zoom";
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

//! [12]
void ImageDrawer::wheelEvent(QWheelEvent *event)
{
    if(enableZoom){
        int numDegrees = event->delta() / 8;
        if(numDegrees<0){
            double numSteps = -numDegrees / 15.0f;
            int posY = scrollArea->verticalScrollBar()->value();
            int posX = scrollArea->horizontalScrollBar()->value();
            for(int i=0; i<numSteps; i++){
                zoomIn();
                int newPosY = scrollArea->verticalScrollBar()->value();
                int newPosX = scrollArea->horizontalScrollBar()->value();
                scroll(posX-newPosX,posY-newPosY);
            }

        }else{
            double numSteps = numDegrees / 15.0f;
            int posY = scrollArea->verticalScrollBar()->value();
            int posX = scrollArea->horizontalScrollBar()->value();
            for(int i=0; i<numSteps; i++){
                zoomOut();
                int newPosY = scrollArea->verticalScrollBar()->value();
                int newPosX = scrollArea->horizontalScrollBar()->value();
                scroll(posX-newPosX,posY-newPosY);
            }
        }
        event->accept();
    }
}
//! [12]

//! [13]
void ImageDrawer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        lastDragPos = event->pos();
}
//! [13]

//! [14]
void ImageDrawer::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        pixmapOffset = event->pos() - lastDragPos;
        lastDragPos = event->pos();
        int deltaX = -pixmapOffset.x();
        int deltaY = -pixmapOffset.y();
        scroll(deltaX,deltaY);
    }
}
//! [14]

//! [15]
void ImageDrawer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = QPoint();
    }
}
//! [15]

void ImageDrawer::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (static_cast<QWindowStateChangeEvent*>(event)->oldState() == windowState()){
            return;
        }
        if (isMaximized()){
            if (!fitToWindowAct->isChecked() && init){
                scaleImage(origImage,((double)scrollArea->height())/origImage.height());
            }
            if(init){
                currentWidth = this->width();
                currentHeight = this->height();
            }
            init = true;
        }
    }
    QWidget::changeEvent(event);
}

void ImageDrawer::applySettings()
{
    int cutoff = cutoffBox->value();
    int threashold = thresholdBox->value();
    double separateFactor = separateBox->currentText().toDouble();

    lokalizer.setCutoffFactor(cutoff);
    lokalizer.setThresholdFactor(threashold);
    lokalizer.setSeparateFactor(separateFactor);
    lokalizer.setParameters();
    //settingsWidget->hide();
    QString informationText =   "Parameters set to:"
                                "\n- Separate Factor  : " +QString::number(separateFactor)+
                                "\n- Threashold Facor : " +QString::number(threashold) +
                                "\n- Cutoff Facor     : " +QString::number(cutoff);

    QMessageBox::information(this,"Parameters set",informationText);
}


void ImageDrawer::showElapsedTime(int elapsedTime, int numSpots)
{
  numPointsLbl->setText(QString::number(numSpots));
  elapsedTimeLbl->setText(QString::number(((double)elapsedTime)/1000.0) + " sec");
  progressBar->setValue(progressBar->maximum());

  QMessageBox::information(this,"Image processing finished","Elapsed time: " + QString::number(((double)elapsedTime)/1000)+" seconds.\n" +
                                                            "Signals found: " + QString::number(numSpots));
}

