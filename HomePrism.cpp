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

    QString dirStr = d->ui.snapshotDirectory->text();
    if( dirStr.isEmpty() )
        dirStr = ".";
    dirStr = dirStr + QDir::separator();

    QString ext = QString(".") + d->ui.snapshotFormat->currentText();

    //get current date and time
    QDateTime dateTime = QDateTime::currentDateTime();
	QString dateTimeString = dateTime.toString("yyMMdd-hhmmss");
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
        }
        else
        {
            QString info = QString("Snapshot %1 saved!").arg(QString::fromStdString(finalFileName));
            d->ui.statusbar->showMessage(info);
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
    d->settings->setValue("snapshotFormat", d->ui.snapshotFormat->currentText() );
    d->settings->setValue("snapshotDelay", d->ui.snapshotDelay->value() );
    d->settings->sync();

    qDebug() << "settings saved: cameraNumber=" << d->settings->value("cameraNumber").toInt()
             << ", snapshotDirectory=" << d->settings->value("snapshotDirectory").toString()
             << ", snapshotFormat=" << d->settings->value("snapshotFormat").toString()
             << ", snapshotDelay=" << d->settings->value("snapshotDelay").toDouble();
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

        qDebug() << "settings loaded: cameraNumber=" << d->settings->value("cameraNumber").toInt()
             << ", snapshotDirectory=" << d->settings->value("snapshotDirectory").toString()
             << ", snapshotFormat=" << d->settings->value("snapshotFormat").toString()
             << ", snapshotDelay=" << d->settings->value("snapshotDelay").toDouble();

    // set ui
    d->ui.cameraNumber->setValue(cameraNumber);
    d->ui.snapshotDirectory->setText(snapshotDirectory);
    d->ui.snapshotFormat->setCurrentIndex(snapshotFormatIndex);
    d->ui.snapshotDelay->setValue(snapshotDelay);

    // initialize values for timers etc
    d->snapshotTimer->setInterval(this->toMilliSeconds(d->ui.snapshotDelay->value()));
}
