#ifndef CFILE_H
#define CFILE_H
#include <QList>
#include "csegyfile.h"
#include <QFileInfoList>
class CFile
{
public:
    CFile();
    QStringList getAllFileName(QString path);
    CSegyFile getSegyInfo(QString path);
    unsigned char bcd_to_hex(char data);
    QList<char> checkComponent(QString path, qint64 trace);
    char swapBinary(char *tni4);
    QDate returnMonth(int year , int day);
    bool isRunyear(int year);
    int sort(QStringList qstrList);
    void writeFcnt(QString filePath , QFile *targetFile , int componentNo);
    QFileInfoList GetFileList(QString path);
    QDateTime getSegyDateTime(QString path, qint64 trace);
};

#endif // CFILE_H
