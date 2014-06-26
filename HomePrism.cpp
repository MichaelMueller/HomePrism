#include <QtGui>
#include <QTimer>
#include <QSettings>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

#include <iostream>

#include <highgui.h>

#include "HomePrism.h"
#include <HomePrismConfig.h>
#include "ui_HomePrism.h"

class HomePrismData
{
public:
    Ui::HomePrismMainWindow ui;
    cv::VideoCapture capture;
    QTimer* captureTimer;
    cv::Mat currentImage;
    QSettings* settings;
    QTimer* snapshotTimer;
    QTimer* wipeTimer;
};

HomePrism::HomePrism()
    : d(new HomePrismData)
{
    d->captureTimer = new QTimer(this);
    d->captureTimer->setInterval(40);
    d->captureTimer->setObjectName("captureTimer");
    d->settings = new QSettings(QSettings::IniFormat,
          QSettings::UserScope,
          QCoreApplication::organizationName(),
          QCoreApplication::applicationName(), this );

    d->snapshotTimer = new QTimer(this);
    d->snapshotTimer->setObjectName("snapshotTimer");

    d->wipeTimer = new QTimer(this);
    d->wipeTimer->setInterval(1000*60);
    d->wipeTimer->setObjectName("wipeTimer");

    d->ui.setupUi(this);
    setWindowTitle( HomePrism_TITLE );
    this->LoadData();
}

HomePrism::~HomePrism()
{
    delete d;
    d = 0;
}

void HomePrism::on_showVideoButton_toggled(bool checked)
{
    //d->ui.statusbar->showMessage( "on_showVideoButton_Toggled" );
    if( checked )
    {
        d->captureTimer->start();
    }
    else
    {
        d->captureTimer->stop();
    }
}

void HomePrism::on_captureTimer_timeout()
{
    if( !this->readCurrentImage() )
    {
        d->ui.showVideoButton->click();
        return;
    }

    d->ui.cvImageWidget->showImage( d->currentImage );
}

void HomePrism::on_cameraNumber_valueChanged(int)
{
    if( d->ui.showVideoButton->isChecked() )
    {
        d->ui.showVideoButton->click();
        d->capture.release();
    }
    this->SaveData();
}

void HomePrism::on_snapshotSelectButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory( this, tr(HomePrism_TITLE) );
    if( !dir.isEmpty() )
    {
        d->ui.snapshotDirectory->setText(dir);
    }
    this->SaveData();
}

void HomePrism::on_snapshotCreateSubdirectories_toggled(bool)
{
    this->SaveData();
}

void HomePrism::on_snapshotFormat_currentIndexChanged(int i)
{
    //std::cout<< "on_snapshotFormat_currentChanged" << std::endl;
    this->SaveData();
}

void HomePrism::on_snapshotDelay_valueChanged(double delay)
{
    d->snapshotTimer->setInterval(this->toMilliSeconds(delay));
    this->SaveData();
}

int HomePrism::toMilliSeconds(double seconds)
{
    return static_cast<int>(seconds*1000.0);
}

void HomePrism::on_recordingButton_toggled(bool checked)
{
    if( checked )
    {
        d->snapshotTimer->start();
    }
    else
    {
        d->snapshotTimer->stop();
    }
}

void HomePrism::on_snapshotTimer_timeout()
{

    //get current date and time
    QDateTime dateTime = QDateTime::currentDateTime();
	QString dateTimeString = dateTime.toString("yyMMdd-hhmmss");

    // dirs
    QString dirStr = d->ui.snapshotDirectory->text();
    if( dirStr.isEmpty() )
        dirStr = ".";
    dirStr = dirStr + QDir::separator();
    bool createSubDir = d->ui.snapshotCreateSubdirectories->isChecked();
    if( createSubDir )
    {
        dirStr = dirStr + dateTime.toString("yyMMdd-hh") + QDir::separator();
        QDir newDir(dirStr);
        if( !newDir.exists() )
        {
            if(!newDir.mkdir(dirStr))
            {
                QString err = QString("Error: Cannot create directory %1!").arg(dirStr);
                d->ui.statusbar->showMessage(err);
                return;
            }
        }
    }

    // ext
    QString ext = QString(".") + d->ui.snapshotFormat->currentText();

    // create filename
    QString baseFileName = dateTimeString;
    int i = 1;
    QFileInfo f(dirStr+baseFileName+ext);
    while(f.exists())
    {
        f = QFileInfo(dirStr+baseFileName+QString::number(i)+ext);
        ++i;
    }

    if(!this->readCurrentImage())
    {
        d->ui.recordingButton->click();
        return;
    }
    else
    {
        std::string finalFileName = f.absoluteFilePath().toStdString();
        if(!cv::imwrite(finalFileName, d->currentImage))
        {
            QString err = QString("Error: Cannot write snapshot %1 (please check filename, permissions)!").arg(QString::fromStdString(finalFileName));
            d->ui.statusbar->showMessage(err);
            return;
        }
        /*
        else
        {
            QString info = QString("Snapshot %1 saved!").arg(QString::fromStdString(finalFileName));
            d->ui.statusbar->showMessage(info);
        }
        */
    }
}

void HomePrism::on_wipeCheckBox_toggled(bool checked)
{
    this->SaveData();
    if( checked )
        d->wipeTimer->start();
    else
        d->wipeTimer->stop();
}

void HomePrism::on_wipeTime_valueChanged(int)
{
    this->SaveData();

}

void HomePrism::on_wipeTimeMultiplier_currentIndexChanged(int i)
{
    this->SaveData();

}

void HomePrism::on_wipeTimer_timeout()
{
    QDir dir(d->ui.snapshotDirectory->text());
    QDateTime currentDateTime = QDateTime::currentDateTime();
    qint64 currentTime = currentDateTime.toMSecsSinceEpoch();
    // minutes
    qint64 multiplier = 1000*60;
    QString wipeTimeMultiplierString = d->ui.wipeTimeMultiplier->currentText();
    if(wipeTimeMultiplierString == "hours")
        multiplier = multiplier*60;
    else if(wipeTimeMultiplierString == "days")
        multiplier = multiplier*60*24;

    qint64 wipeTime = d->ui.wipeTime->value();
    qint64 allowedAge = wipeTime*multiplier;

    QStringList filters;
    for( int i=0; i<d->ui.snapshotFormat->count(); ++i )
        filters << QString( QString("*.") + d->ui.snapshotFormat->itemText(i) );
    //dir.setNameFilters(filters);

    QFileInfoList fileList = dir.entryInfoList(filters);
    for( int i=0; i<fileList.size(); ++i )
    {
        qint64 modTime = fileList.at(i).lastModified().toMSecsSinceEpoch();
        qint64 age = currentTime - modTime;
        if( age > allowedAge )
        {
            QFile file(fileList.at(i).absoluteFilePath());
            if( !file.remove() )
            {
                QString err = QString("File %1 could not be wiped!").arg( fileList.at(i).absoluteFilePath() );
                d->ui.statusbar->showMessage(err);
            }
        }
    }
}

bool HomePrism::readCurrentImage()
{

    if( !d->capture.isOpened() )
    {
        d->capture.open( d->ui.cameraNumber->value() );
    }

    cv::Mat image;
    bool readImage = d->capture.read(image);
    if( !readImage )
    {
        d->ui.statusbar->showMessage("Error: Cannot read images from capture device (Wrong capture number?)!");
        return false;
    }
    d->currentImage = image;
    return true;
}

void HomePrism::SaveData()
{
    qDebug() << "saving settings";
    d->settings->setValue("cameraNumber", d->ui.cameraNumber->value() );
    d->settings->setValue("snapshotDirectory", d->ui.snapshotDirectory->text() );
    d->settings->setValue("snapshotCreateSubdirectories", d->ui.snapshotCreateSubdirectories->isChecked() );
    d->settings->setValue("snapshotFormat", d->ui.snapshotFormat->currentText() );
    d->settings->setValue("snapshotDelay", d->ui.snapshotDelay->value() );

    d->settings->setValue("wipeCheckBox", d->ui.wipeCheckBox->isChecked() );
    d->settings->setValue("wipeTime", d->ui.wipeTime->value() );
    d->settings->setValue("wipeTimeMultiplier", d->ui.wipeTimeMultiplier->currentText() );

    d->settings->sync();

    qDebug() << "settings saved: "
         << "cameraNumber=" << d->settings->value("cameraNumber").toInt()
         << ", snapshotDirectory=" << d->settings->value("snapshotDirectory").toString()
         << ", snapshotCreateSubdirectories=" << d->settings->value("snapshotCreateSubdirectories").toBool()
         << ", snapshotFormat=" << d->settings->value("snapshotFormat").toString()
         << ", snapshotDelay=" << d->settings->value("snapshotDelay").toDouble()
         << ", wipeCheckBox=" << d->settings->value("wipeCheckBox").toBool()
         << ", wipeTime=" << d->settings->value("wipeTime").toInt()
         << ", wipeTimeMultiplier=" << d->settings->value("wipeTimeMultiplier").toString();
}

void HomePrism::LoadData()
{
    qDebug() << "loading settings";

    d->settings->sync();

    // cameraNumber
    int cameraNumber = 0;
    if( d->settings->contains("cameraNumber") )
        cameraNumber = d->settings->value("cameraNumber").toInt();

    // snapshotDirectory
    QString snapshotDirectory = "";
    if( d->settings->contains("snapshotDirectory") )
        snapshotDirectory = d->settings->value("snapshotDirectory").toString();
    if(snapshotDirectory.isEmpty())
        snapshotDirectory = QDir::currentPath();

    // snapshotCreateSubdirectories
    bool snapshotCreateSubdirectories = false;
    if( d->settings->contains("snapshotCreateSubdirectories") )
        snapshotCreateSubdirectories = d->settings->value("snapshotCreateSubdirectories").toBool();

    // snapshotFormat
    int snapshotFormatIndex = 0;
    if( d->settings->contains("snapshotFormat") )
    {
        int index = d->ui.snapshotFormat->findText( d->settings->value("snapshotFormat").toString() );
        if( index >= 0)
            snapshotFormatIndex = index;
    }

    // snapshotDelay
    double snapshotDelay = 1;
    if( d->settings->contains("snapshotDelay") )
        snapshotDelay = d->settings->value("snapshotDelay").toDouble();

    // wipeCheckBox
    bool wipeCheckBox = false;
    if( d->settings->contains("wipeCheckBox") )
        wipeCheckBox = d->settings->value("wipeCheckBox").toBool();

    // cameraNumber
    int wipeTime = 1;
    if( d->settings->contains("wipeTime") )
        wipeTime = d->settings->value("wipeTime").toInt();

    // wipeTimeMultiplier
    int wipeTimeMultiplierIndex = 0;
    if( d->settings->contains("wipeTimeMultiplier") )
    {
        int index = d->ui.wipeTimeMultiplier->findText( d->settings->value("wipeTimeMultiplier").toString() );
        if( index >= 0)
            wipeTimeMultiplierIndex = index;
    }


    qDebug() << "settings loaded: "
         << "cameraNumber=" << d->settings->value("cameraNumber").toInt()
         << ", snapshotDirectory=" << d->settings->value("snapshotDirectory").toString()
         << ", snapshotCreateSubdirectories=" << d->settings->value("snapshotCreateSubdirectories").toBool()
         << ", snapshotFormat=" << d->settings->value("snapshotFormat").toString()
         << ", snapshotDelay=" << d->settings->value("snapshotDelay").toDouble()
         << ", wipeCheckBox=" << d->settings->value("wipeCheckBox").toBool()
         << ", wipeTime=" << d->settings->value("wipeTime").toInt()
         << ", wipeTimeMultiplier=" << d->settings->value("wipeTimeMultiplier").toString();

    // set ui
    d->ui.cameraNumber->setValue(cameraNumber);
    d->ui.snapshotDirectory->setText(snapshotDirectory);
    d->ui.snapshotCreateSubdirectories->setChecked(snapshotCreateSubdirectories);
    d->ui.snapshotFormat->setCurrentIndex(snapshotFormatIndex);
    d->ui.snapshotDelay->setValue(snapshotDelay);
    d->ui.wipeCheckBox->setChecked(wipeCheckBox);
    d->ui.wipeTime->setValue(wipeTime);
    d->ui.wipeTimeMultiplier->setCurrentIndex(wipeTimeMultiplierIndex);

    // initialize values for timers etc
    d->snapshotTimer->setInterval(this->toMilliSeconds(d->ui.snapshotDelay->value()));
}
