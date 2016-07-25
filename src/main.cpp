#include <QApplication>
#include "imagedrawer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ImageDrawer w;

    w.show();

    return a.exec();
}
