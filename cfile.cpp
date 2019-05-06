#include "cfile.h"
#include <QDir>
#include <QDebug>
#include <QList>
#include "csegyfile.h"
#include "swapbyte.h"
CFile::CFile()
{

}

QStringList CFile::getAllFileName(QString path)
{
    QDir dir(path);
    QStringList nameFilters;
    nameFilters << "*.fcnt";
    QStringList fileNameList = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    qDebug()<<fileNameList.size();
    return fileNameList;
}

CSegyFile CFile::getSegyInfo(QString path)
{
    char *buffersu = new char[32];
    CSegyFile segyFile;
    //segyFile.setFileName(fileNameList.at(i));
    qDebug()<<path;
    //开始读取文件的year+day+time
    QFile file(path);
    if(file.open(QIODevice::ReadOnly)){
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_5_11);
//            in.skipRawData(32);
        in.readRawData(buffersu,32);
        char year = {buffersu[10]};
        int year_int = bcd_to_hex(year);
        char d = {buffersu[11]};
        int d_int = bcd_to_hex(d)%10;
        char day = {buffersu[12]};
        int day_int = bcd_to_hex(day)+d_int*100;
        char hour = {buffersu[13]};
        int hour_int = bcd_to_hex(hour);
        char min = {buffersu[14]};
        int min_int = bcd_to_hex(min);
        char sec = {buffersu[15]};
        int sec_int = bcd_to_hex(sec);
        QTime time(hour_int,min_int,sec_int);
        segyFile.setYear(year_int);
        segyFile.setDay(day_int);
        segyFile.setTime(time);
    }
        file.close();
        delete [] buffersu;
    return segyFile;
}

unsigned char CFile::bcd_to_hex(char data){
    unsigned char temp;
    temp = ((data>>4)*10 + (data&0x0f));
    return temp;
}

char CFile::swapBinary(char *tni4)
{
    char tmp;
    tmp=*tni4;
    *tni4=*(tni4+3);
    *(tni4+3)=tmp;
    tmp=*(tni4+1);
    *(tni4+1)=*(tni4+2);
    *(tni4+2)=tmp;
    return *tni4;
}

bool CFile::isRunyear(int year)
{
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0){
        return true;
    }else{
        return false;
    }
}

bool compare(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

int CFile::sort(QStringList qstrList)
{
    qSort(qstrList.begin(), qstrList.end(), compare);
}

void CFile::writeFcnt(QString filePath, QFile *targetFile, int componentNo)
{
    QFile file(filePath);
    qint64 trace_3C = (file.size()-288)/(60000*4+340);
    qint64 trace_1C = trace_3C/3;
    char *buffer = new char[60000*4+340];
    char *bufferQu = new char[288];
    qDebug()<<"文件大小开始为： "<<targetFile->size();
    if(file.open(QIODevice::ReadOnly)&&targetFile->open(QIODevice::ReadWrite|QIODevice::Append)){
        QDataStream in(&file);
        QDataStream targetIn(targetFile);
//        targetFile->seek(targetFile->size());
        in.setVersion(QDataStream::Qt_5_11);
        targetIn.setVersion(QDataStream::Qt_5_11);
        if(targetFile->size()==0){
            qDebug()<<"这是129.0.0的第1次写入";
            in.readRawData(bufferQu , 288);
            targetIn.writeRawData(bufferQu , 288);
            for(int i = 0 ; i < trace_1C ; i++){
                in.readRawData(buffer , 60000*4+340);
                targetIn.writeRawData(buffer , 60000*4+340);
            }
        }else{
            qDebug()<<"这是"<<filePath<<"的第"<<componentNo+1<<"次写入"<<"指针的位置在："<<file.pos();
            in.skipRawData(288);
            for(int i = 0; i<componentNo; i++){
                for(int j = 0; j<trace_1C; j++)
                    in.skipRawData(60000*4+340);
            }
            for(int i = 0 ; i < trace_1C&&!in.atEnd() ; i++){
                in.readRawData(buffer , 60000*4+340);
                targetIn.writeRawData(buffer , 60000*4+340);
            }
        }
    }
    qDebug()<<"文件大小最终为： "<<targetFile->size()/(1024*1024);
    delete [] buffer;
    delete [] bufferQu;
    targetFile->close();
    file.close();

}

QFileInfoList CFile::GetFileList(QString path)
{
    QDir dir(path);
    QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(int i = 0; i != folder_list.size(); i++){
        QString name = folder_list.at(i).absoluteFilePath();
        QFileInfoList child_file_list = GetFileList(name);
        file_list.append(child_file_list);
    }
    return file_list;
}

QDateTime CFile::getSegyDateTime(QString path, qint64 trace)
{
    QFile file(path);
    char *bufferSu = new char[240];
    if(file.open(QIODevice::ReadOnly)){
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_5_11);
        in.skipRawData(3600);
        for(qint64 i = 0; i < trace-1; i++){
            in.skipRawData(60000*4+240);
        }
        in.readRawData(bufferSu,240);
        char year[2] = {bufferSu[156],bufferSu[157]};
        short *year_s = (short *)year;
        swap_short_2(year_s);

        char day[2] = {bufferSu[158],bufferSu[159]};
        short *day_s = (short *)day;
        swap_short_2(day_s);

        char hour[2] = {bufferSu[160],bufferSu[161]};
        short *hour_s = (short *)hour;
        swap_short_2(hour_s);

        char min[2] = {bufferSu[162],bufferSu[163]};
        short *min_s = (short *)min;
        swap_short_2(min_s);

        char sec[2] = {bufferSu[164],bufferSu[165]};
        short *sec_s = (short *)sec;
        swap_short_2(sec_s);

        QDateTime dt(returnMonth((*year_s)+2000,*day_s),QTime (*hour_s,*min_s,*sec_s));
        return dt;
    }
}

QDate CFile::returnMonth(int year , int day)
{
    bool t;
    t = isRunyear(year);
    int iArr[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int last = 0;
    int next = 0;
    int month=0;
    int d = 0;
    if(t==true){
        //表明是闰年
        iArr[1] = 29;
        for(int i = 0; i < 12; i++){
            last = next;
            next += iArr[i];
            if(day > last && day <= next){
                month = i+1;
                d = day-last;
                break;
            }
        }
    }else{
        //表明是平年
        for(int i = 0; i < 12; i++){
            last = next;
            next += iArr[i];
            if(day > last && day <= next){
                month = i+1;
                d = day-last;
                break;
            }
        }
    }
    QDate date(year,month,d);
    return date;
}

QList<char> CFile::checkComponent(QString path, qint64 trace)
{
    QFile file(path);
    QList<char> componentList;
    if(file.open(QIODevice::ReadOnly)){
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_5_11);
        in.skipRawData(288);
        for(qint64 i = 0; i < trace; i++){
            char com;
            in.skipRawData(20);
            in.skipRawData(20);
            in.readRawData(&com,1);
            componentList<<com;
            in.skipRawData(299);
            in.skipRawData(60000*4);
        }
    }
    file.close();
    return componentList;
}




