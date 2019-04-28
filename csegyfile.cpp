#include "csegyfile.h"
#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include "swapbyte.h"
#include <QDebug>
#include <QMessageBox>
CSegyFile::CSegyFile()
{

}

void CSegyFile::setYear(int year)
{
    this->year = year;
}

void CSegyFile::setDay(int day)
{
    this->day = day;
}

void CSegyFile::setTime(QTime time)
{
    this->time = time;
}

void CSegyFile::setFileName(QString fileName)
{
    this->fileName = fileName;
}

int CSegyFile::getYear()
{
    return this->year;
}

int CSegyFile::getDay()
{
    return this->day;
}

QTime CSegyFile::getTime()
{
    return this->time;
}

QString CSegyFile::getFileName()
{
    return this->fileName;
}

void CSegyFile::changeNsDt(QString path,unsigned short ns,unsigned short dt,qint64 trace,QList<char> componentList)
{
    //写入总道数、采样率、采样点数、分量标识
    //02 – Vertical geophone
    //03 – Inline geophone
    //04 – Cross-line geophone
    QFile file(path);
    qDebug()<<path;
    char *newNs = new char[2];
    char *newDt = new char[2];
    char *newNtrpr = new char[2];
    if(file.open(QIODevice::ReadWrite)){
        short ntrpr = (short)trace;
        qDebug()<<"ntrpr "<<ntrpr<<"ns "<<ns<<"dt "<<dt;
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_5_11);
        swap_u_short_2(&ns);
        swap_u_short_2(&dt);
        swap_short_2(&ntrpr);
        memcpy(&(newNs[0]) , &ns , sizeof(char)*2);
        memcpy(&(newDt[0]) , &dt , sizeof(char)*2);
        memcpy(&(newNtrpr[0]) , &ntrpr , sizeof(char)*2);
        in.skipRawData(3212);
        in.writeRawData(newNtrpr,2);
        in.skipRawData(2);
        in.writeRawData(newDt,2);
        in.writeRawData(newDt,2);
        in.writeRawData(newNs,2);//3222
        in.writeRawData(newNs,2);//3224
        in.skipRawData(376);
        if(componentList.size()==trace) {
            for(int i = 0;i < trace; i++){
                short component = (short)componentList.at(i);
                swap_short_2(&component);
                char *newComonpent = new char[2];
                memcpy(&(newComonpent[0]) , &component , sizeof(char)*2);
                in.skipRawData(28);
                in.writeRawData(newComonpent,2);//30
                in.skipRawData(84);
                in.writeRawData(newNs,2);//116
                in.writeRawData(newDt,2);//118
                in.skipRawData(122);
                in.skipRawData(60000*4);
                delete [] newComonpent;
            }
        }else{
            qDebug()<<"list和trace不匹配!";
        }
    }
    delete []newNs;
    delete []newDt;
    delete []newNtrpr;
    file.close();
}

short CSegyFile::getComponentNo(char bufferSu[]){
    char newComonpent[2] = {bufferSu[28],bufferSu[29]};
    short *s_comonpent = (short *)newComonpent;
    swap_short_2(s_comonpent);
    return *s_comonpent;
}

void CSegyFile::cut3CFile(QString segyPath,QString targetDir,QProgressDialog *progressDialog)
{
    QFile segyFile(segyPath);
    if(segyFile.open(QIODevice::ReadOnly)){
        QFileInfo segyFileInfo(segyFile);
        QString segyName = segyFileInfo.fileName();
        qint64 trace= (segyFileInfo.size()-3600)/(240+60000*4);
        progressDialog->setRange(0,trace-1);

//        qDebug()<<"cutTrace"<<trace<<"segyName: "<<segyName;
        QFile verticalFile(targetDir+"/"+segyName.left(segyName.length()-4)+"-Vertical.sgy");
        QFile inlineFile(targetDir+"/"+segyName.left(segyName.length()-4)+"-Inline.sgy");
        QFile crossLineFile(targetDir+"/"+segyName.left(segyName.length()-4)+"-Crossline.sgy");
        qDebug()<<targetDir+"/"+segyName.left(segyName.length()-4)+"-Vertical.sgy";
        if(verticalFile.open(QIODevice::WriteOnly)&&inlineFile.open(QIODevice::WriteOnly)&&crossLineFile.open(QIODevice::WriteOnly)){
            QDataStream verticalIn(&verticalFile);
            QDataStream inlineIn(&inlineFile);
            QDataStream crossLineIn(&crossLineFile);
            QDataStream in(&segyFile);
            verticalIn.setVersion(QDataStream::Qt_5_11);
            inlineIn.setVersion(QDataStream::Qt_5_11);
            crossLineIn.setVersion(QDataStream::Qt_5_11);
            in.setVersion(QDataStream::Qt_5_11);
            char *bufferQu = new char[3600];
            char *bufferSu = new char[240];
            char *data = new char[60000*4];
            in.readRawData(bufferQu,3600);//读取卷头
            verticalIn.writeRawData(bufferQu,3600);
            inlineIn.writeRawData(bufferQu,3600);
            crossLineIn.writeRawData(bufferQu,3600);
            for(int i = 0; i < trace; i++){
                in.readRawData(bufferSu,240);
                in.readRawData(data,60000*4);
                short c = getComponentNo(bufferSu);
                switch (c) {
                case 3:
                    inlineIn.writeRawData(bufferSu,240);
                    inlineIn.writeRawData(data,60000*4);
                    break;
                case 4:
                    crossLineIn.writeRawData(bufferSu,240);
                    crossLineIn.writeRawData(data,60000*4);
                    break;
                case 2:
                    verticalIn.writeRawData(bufferSu,240);
                    verticalIn.writeRawData(data,60000*4);
                    break;
                default:
                    qDebug()<<"switch语句出现问题!";
                    break;
                }
                progressDialog->setValue(i);
            }
            delete []bufferQu;
            delete []bufferSu;
            delete []data;
        }else{
            qDebug()<<"文件打开失败!";
        }
        verticalFile.close();
        inlineFile.close();
        crossLineFile.close();
        segyFile.close();
    }
}
