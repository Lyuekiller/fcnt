#include "widget.h"
#include "ui_widget.h"
#include "cfile.h"
#include "csegyfile.h"
#include "swapbyte.h"
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QProgressDialog>
#include <algorithm>
using namespace  std;
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

bool cmp(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

bool compareSegyDataTime(const QFileInfo &s1, const QFileInfo &s2)
{
    CFile cfile;
    qint64 secsTo = cfile.getSegyDateTime(s1.filePath(),1).secsTo(cfile.getSegyDateTime(s2.filePath(),1));
    if (secsTo>0) return true;
    else  return false;
}

bool compareFcntDataTime(const QFileInfo &s1, const QFileInfo &s2)
{
    CFile cfile;
    qint64 secsTo = cfile.getFcntDateTime(s1.filePath()).secsTo(cfile.getFcntDateTime(s2.filePath()));
    if (secsTo>0) return true;
    else  return false;
}

void Widget::on_openButton_clicked()
{
    QString fcntPath = QFileDialog::getOpenFileName(this,"FCNT文件",".","FCNT(*.fcnt)");
    ui->fcntLineEdit->setText(fcntPath);
}

void Widget::on_targetButton_clicked()
{
    QString targetDir = QFileDialog::getExistingDirectory(this,"生成新数据文件",QDir::homePath());
    ui->targetLineEdit->setText(targetDir);
}

void Widget::on_readButton_clicked()
{
    //把fcnt格式转化成segy格式，并在segy格式写入道序号
    //在fcnt格式中提取了分量标识，并存入list中
    //调用changeNsDt()函数，写入总道数、采样率、采样点数、分量标识

    //值得注意的是道序号是通过 unsigned int 写入的，调换了高低位(通过无符号int函数调换的)，这样可以记录更大的数字，所以在读取的时候也要按照无符号读取
    QProgressDialog *progressDialog=new QProgressDialog(this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(5);
    progressDialog->setWindowTitle("请稍等");
    progressDialog->setLabelText("格式转化中......");
    progressDialog->setCancelButtonText("Cancel");

    QString fcntPath = ui->fcntLineEdit->text();
    QFileInfo fcntInfo(fcntPath);
    qint64 fcntSize = fcntInfo.size();//文件大小
    qint64 fcntTrace = (fcntSize-288)/240340;//一个文件的总道数

    progressDialog->setRange(0,fcntTrace);

    CFile cfile;
    QList<char> componentList = cfile.checkComponent(fcntPath,fcntTrace);//查看每一道是哪一个分量

    QString fcntFileName = fcntInfo.fileName().left(fcntInfo.fileName().length()-5);
    QFile fcntFile(fcntPath);
    QFile segyFile("CR1377.sgy");
    QString newFilePath = ui->targetLineEdit->text()+"/"+fcntFileName+".sgy";
    QFile newFcntFile(newFilePath);

    char *bufferData = new char[60000*4];
    char *bufferQu = new char[3600];
    char *bufferSu = new char[240];
    char *fcntSu = new char[340];

    if(fcntFile.open(QIODevice::ReadWrite)&&segyFile.open(QIODevice::ReadWrite)&&newFcntFile.open(QIODevice::WriteOnly)){
        QDataStream newFnctIn(&newFcntFile);
        QDataStream segyIn(&segyFile);
        QDataStream fcntIn(&fcntFile);
        newFnctIn.setVersion(QDataStream::Qt_5_11);
        segyIn.setVersion(QDataStream::Qt_5_11);
        fcntIn.setVersion(QDataStream::Qt_5_11);
        segyIn.readRawData(bufferQu,3600);//读入segy的卷头
        segyIn.readRawData(bufferSu,240);//读入segy的道头
        newFnctIn.writeRawData(bufferQu,3600);
        fcntIn.skipRawData(288);
        int timeCount = 0;
        for(int i = 0; i < fcntTrace; i++){
            fcntIn.readRawData(fcntSu,340);
            char num[4] = {fcntSu[44],fcntSu[43],fcntSu[42],fcntSu[41]};
            unsigned int *num_u_int = (unsigned int *)num;
            swap_u_int_4(num_u_int);
            memcpy(&(bufferSu[0]) , num_u_int , sizeof(char)*4);//在道头的1-4的位置写入道序号


            if(timeCount%(fcntTrace/3)==0)
                timeCount = 0;
            CSegyFile csegy = cfile.getSegyInfo(fcntPath);
            short year = (short)csegy.getYear();
            swap_short_2(&year);
            memcpy(&(bufferSu[156]) , &year , sizeof(char)*2);
            QTime time = csegy.getTime();
            QDateTime dt(cfile.returnMonth(csegy.getYear()+2000,csegy.getDay()),time);
            QDateTime afterDt = dt.addSecs(120*timeCount);
            qDebug()<<afterDt.toString("yyyy-MM-dd HH:mm:dd");
            short day = (short)afterDt.date().dayOfYear();
            swap_short_2(&day);
            memcpy(&(bufferSu[158]) , &day , sizeof(char)*2);
            short hour = (short)afterDt.time().hour();
            swap_short_2(&hour);
            memcpy(&(bufferSu[160]) , &hour , sizeof(char)*2);
            short min = (short)afterDt.time().minute();
            swap_short_2(&min);
            memcpy(&(bufferSu[162]) , &min , sizeof(char)*2);
            short second = (short)afterDt.time().second();
            swap_short_2(&second);
            memcpy(&(bufferSu[164]) , &second , sizeof(char)*2);
            timeCount++;

            fcntIn.readRawData(bufferData,60000*4);
            newFnctIn.writeRawData(bufferSu,240);
            newFnctIn.writeRawData(bufferData,60000*4);
            progressDialog->setValue(i);
        }
        delete[] bufferData;
        delete[] bufferQu;
        delete[] bufferSu;
        fcntFile.close();
        segyFile.close();
        newFcntFile.close();
    }else{
        QMessageBox::information(this,"Error","文件打开失败!");
        return ;
    }
    CSegyFile sFile;
    sFile.changeNsDt(newFilePath,60000,2000,fcntTrace,componentList);
    progressDialog->setValue(fcntTrace);
    QMessageBox::information(this,"Finish","程序执行完毕!");
//    file.getSegyInfo(fcntPath);
}



void Widget::on_openFcntButton_clicked()
{
    QString segyPath = QFileDialog::getOpenFileName(this,"SEGY文件",".","SEGY(*.sgy)");
    ui->cutLineEdit->setText(segyPath);
}

void Widget::on_cutButton_clicked()
{
    QProgressDialog *progressDialog=new QProgressDialog(this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(5);
    progressDialog->setWindowTitle("请稍等");
    progressDialog->setLabelText("文件切分中......");
    progressDialog->setCancelButtonText("Cancel");

    QString segyPath = ui->cutLineEdit->text();
    CSegyFile sFile;
    if(!segyPath.isEmpty()&&!ui->targetSegyLineEdit->text().isEmpty()){
        QFile segyFile(segyPath);
        QString dirStr = ui->targetSegyLineEdit->text();
        if(segyFile.open(QIODevice::ReadOnly)){
            sFile.cut3CFile(segyPath,dirStr,progressDialog);
        }


    }else{
        QMessageBox::information(this,"Error","请输入正确的segy文件的路径!");
    }
}

void Widget::on_targetSegyButton_clicked()
{
    QString targetDir = QFileDialog::getExistingDirectory(this,"切分后文件夹的位置",QDir::homePath());
    ui->targetSegyLineEdit->setText(targetDir);
}

void Widget::on_fileButton_clicked()
{
    QString fcntPath = QFileDialog::getOpenFileName(this,"SEGY文件",".","SEGY(*.fcnt)");
    ui->readLineEdit->setText(fcntPath);
}

void Widget::on_readButton_2_clicked()
{
    QFile file(ui->readLineEdit->text());
    QFileInfo fileInfo(ui->readLineEdit->text());
    qint64 length = fileInfo.size();
    qint64 mod = length/(60000*4+340);
    char *bufferQu = new char[160+32*7];
    char *bufferSu = new char[340];
    CFile cfile;
    qDebug()<<"success";
    if(file.open(QIODevice::ReadOnly)){
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_5_11);
        in.readRawData(bufferQu,160+32*7);
        while(!in.atEnd()){
            in.readRawData(bufferSu,340);
            char num[4] = {bufferSu[44],bufferSu[43],bufferSu[42],bufferSu[41]};
            char test[4] = {bufferSu[29],bufferSu[28],bufferSu[27],0};
            char er[4]= {bufferQu[48],bufferQu[47],bufferQu[46],0};

            char component = bufferSu[40];

            char qu = bufferQu[29];

            unsigned int *shortTest = (unsigned int *)num;
            unsigned int *intTest = (unsigned int *)test;
            unsigned int *inter = (unsigned int *) er;
            qDebug()<<"the result is: "<<cfile.bcd_to_hex(qu)+0;
            in.skipRawData(60000*4);
        }
//        CSegyFile csegy = cfile.getSegyInfo(ui->readLineEdit->text());
//        QDate d = cfile.returnMonth(csegy.getYear()+2000,csegy.getDay());
//        QDateTime dt(d,csegy.getTime());
//        qDebug()<<"before: "<<dt;
//        qint64 sec = 120*mod/3;
//        qDebug()<<"trace: "<<mod<<"second: "<<sec;
//        QDateTime afterDt = dt.addSecs(sec);
//        qDebug()<<"after: "<<afterDt;
    }else{
        qDebug()<<"打开失败";
    }
//    qDebug()<<"the result is: "<<mod;
    delete [] bufferQu;
    delete [] bufferSu;
    file.close();
}

void Widget::on_openButton_2_clicked()
{
    QString mergeDir = QFileDialog::getExistingDirectory(this,"合并文件",QDir::homePath());
    ui->mergeLineEdit->setText(mergeDir);
}



void Widget::on_mergeButton_clicked()
{
    QProgressDialog *progressDialog=new QProgressDialog(this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(5);
    progressDialog->setWindowTitle("请稍等");
    progressDialog->setLabelText("格式转化中......");
    progressDialog->setCancelButtonText("Cancel");

    CFile cfile;
    QList<QDateTime> dtList;
    QString mergePath = ui->mergeLineEdit->text();
    QDir mergeDir(mergePath);
    QStringList formatList,fileNameList;
    formatList << "*.fcnt";
    fileNameList = mergeDir.entryList(formatList,QDir::Files|QDir::Readable, QDir::Name);
    progressDialog->setRange(0,fileNameList.size()*3);
    progressDialog->setValue(0);
    QString targetPath=mergePath+"/"+fileNameList.at(0).left(fileNameList.at(0).length()-9)+".fcnt";
    qDebug()<<"targePath: "<<targetPath;
    QFile *targetFile= new QFile(targetPath);
    std::sort(fileNameList.begin(),fileNameList.end(),cmp);
    for(int i = 0 ; i < 3 ; i++){
        CSegyFile segy = cfile.getSegyInfo(mergePath+"/"+fileNameList.at(0));
        QDateTime dt(cfile.returnMonth(segy.getYear()+2000,segy.getDay()),segy.getTime());
        cfile.writeFcnt(mergePath+"/"+fileNameList.at(0),targetFile,i);
        dtList<<dt;
        progressDialog->setValue(1+fileNameList.size()*i);
        for(int j = 1 ; j < fileNameList.size() ; j++){
            segy = cfile.getSegyInfo(mergePath+"/"+fileNameList.at(j));
            dt = QDateTime(cfile.returnMonth(segy.getYear()+2000,segy.getDay()),segy.getTime());
            qint64 sec = 1;
            QDateTime lastDt = dtList.at(dtList.size()-1);
            sec = lastDt.secsTo(dt);
            if((i==0&&sec>0)||i>0){
                cfile.writeFcnt(mergePath+"/"+fileNameList.at(j),targetFile,i);
                dtList<<dt;
            }else{
                QMessageBox::information(this,"Error","文件排序发送错误，或者其他原因导致无法合成!");
                return;
            }
            progressDialog->setValue(j+fileNameList.size()*i);
        }
    }
    targetFile->close();
    delete targetFile;
    delete progressDialog;
}



void Widget::on_openButton_3_clicked()
{
    QString quPath = QFileDialog::getOpenFileName(this,"FCNT文件",".","FCNT(*.fcnt)");
    ui->quLineEdit->setText(quPath);
}

void Widget::on_quButton_clicked()
{
    QProgressDialog *progressDialog=new QProgressDialog(this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(5);
    progressDialog->setWindowTitle("请稍等");
    progressDialog->setLabelText("格式转化中......");
    progressDialog->setCancelButtonText("Cancel");


    QString quPath = ui->quLineEdit->text();
    QFile file(quPath);
    QFileInfo fileInfo(file);
    QString quDir = fileInfo.path();
    qint64 size = fileInfo.size();
    qint64 mod_288 = (size-288)%(60000*4+340);
    qint64 mod_384 = (size-384)%(60000*4+340);
    qDebug()<<"mod_288:"<<mod_288<<" mod_384:"<<mod_384;
    char *bufferQu = new char[160+32*4];
    char *buffer = new char[340+60000*4];
    if(mod_288==0){
        QMessageBox::information(this,"提醒","文件道头本身就是288的长度，请确认!");
    }else{
        if(file.open(QIODevice::ReadOnly)){
            if(mod_384!=0) QMessageBox::information(this,"提醒","这个文件可能存在记录多余（或者不全的情况）!");
            QFile qufile(quDir+"/"+fileInfo.completeBaseName()+"-c."+fileInfo.suffix());
            qint64 trace = (size-384)/(60000*4+340);
            progressDialog->setRange(1,trace);
            if(qufile.open(QIODevice::ReadWrite)){
                QDataStream in(&file);
                QDataStream quIn(&qufile);
                in.setVersion(QDataStream::Qt_5_11);
                quIn.setVersion(QDataStream::Qt_5_11);
                in.readRawData(bufferQu,288);
                quIn.writeRawData(bufferQu,288);
                in.skipRawData(384-288);
                for(int i = 0; i<trace; i++){
                    in.readRawData(buffer,60000*4+340);
                    quIn.writeRawData(buffer,60000*4+340);
                    progressDialog->setValue(i+1);
                }
            }
            qufile.close();
        }
    }

}

void Widget::on_openButton_4_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,"原文件位置",QDir::homePath());
    ui->lineEdit->setText(dir);
}



void Widget::on_tarButton_clicked()
{
    QString tarDir = QFileDialog::getExistingDirectory(this,"新文件存放位置",QDir::homePath());
    ui->tarLineEdit->setText(tarDir);
}

void Widget::on_composeButton_clicked()
{
    CFile cfile;
    QFileInfoList fileInfoList = cfile.GetFileList(ui->lineEdit->text(),"*.sgy");
//    for(int i = 0; i < fileInfoList.size(); i++)
//        qDebug()<<fileInfoList.at(i).filePath();
//    QString path = fileInfoList.at(3).filePath();
    for(int i = 0; i < fileInfoList.size(); i++)
        qDebug()<<cfile.getSegyDateTime(fileInfoList.at(i).filePath(),1).toString("yyyy-MM-dd HH:mm:ss")<<"fileName"<<fileInfoList.at(i).fileName();

}

void Widget::on_oButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,"打开文件夹",QDir::homePath());
    ui->testLineEdit->setText(dir);
}


void Widget::on_testButton_clicked()
{
    QString  dirPath = ui->testLineEdit->text();
    CFile cfile;
    QFileInfoList fileInfoList = cfile.GetFileList(dirPath,"*.fcnt");
    std::sort(fileInfoList.begin(),fileInfoList.end(),compareFcntDataTime);
    for(int i = 0; i < fileInfoList.size(); i++){
        qDebug()<<cfile.getFcntDateTime(fileInfoList.at(i).filePath()).toString("yyyy-MM-dd HH:mm:ss")<<"---------"<<fileInfoList.at(i).filePath();
    }

}
