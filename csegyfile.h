#ifndef CSEGYFILE_H
#define CSEGYFILE_H
#include <QTime>
#include <QFile>
#include <QProgressDialog>
class CSegyFile
{
public:
    CSegyFile();
    void setYear(int year);
    void setDay(int day);
    void setTime(QTime time);
    void setFileName(QString fileName);
    int getYear();
    int getDay();
    QTime getTime();
    QString getFileName();
    void changeNsDt(QString path,unsigned short ns,unsigned short dt,qint64 trace,QList<char> componentList);
    void cut3CFile(QString segyPath,QString targetDir,QProgressDialog *progressDialog);
    short getComponentNo(char *bufferSu);

private:
    int year;
    int day;
    QTime time;
    QString fileName;
};

#endif // CSEGYFILE_H
