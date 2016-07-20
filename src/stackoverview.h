#ifndef STACKOVERVIEW_H
#define STACKOVERVIEW_H

#include <QPixmap>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QString>

class StackOverview : public QThread
{
    Q_OBJECT
public:
    explicit StackOverview(QObject *parent = 0);
    ~StackOverview();
    static QString openImageInStack(const QString & stackName, int sliceNr);
    static int getNumSlizes(const QString & stackName);

signals:
    void ovImageStored(QString);

public slots:
    void genOverview(const QString &imageName);
    QString generateOverview(const QString & stackName);

private:
    void run();

    QMutex mutex;
    QWaitCondition condition;

    QString stackName;
    bool abort;
};

#endif // STACKOVERVIEW_H
