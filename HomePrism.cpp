#include <QtGui>
#include <QTimer>
#include <iostream>

#include "HomePrism.h"
#include <HomePrismConfig.h>
#include "ui_HomePrism.h"

class HomePrismData
{
public:
    Ui::HomePrismMainWindow ui;
    cv::VideoCapture capture;
    QTimer* m_CaptureTimer;
    cv::Mat currentImage;
};

HomePrism::HomePrism()
    : d(new HomePrismData)
{
    d->m_CaptureTimer = new QTimer(this);
    d->m_CaptureTimer->setInterval(40);
    d->m_CaptureTimer->setObjectName("captureTimer");


    d->ui.setupUi(this);
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
        d->m_CaptureTimer->start();
    }
    else
    {
        d->m_CaptureTimer->stop();
    }
}

void HomePrism::on_captureTimer_timeout()
{
    //d->ui.statusbar->showMessage( "on_captureTimer_timeout" );
    if( !d->capture.isOpened() )
    {
        d->capture.open( d->ui.cameraNumber->value() );
    }

    cv::Mat image;
    bool readImage = d->capture.read(image);
    if( !readImage )
    {
        d->ui.statusbar->showMessage("Error: Cannot read images from capture device (Wrong capture number?)!");
        d->ui.showVideoButton->click();
    }

    d->currentImage = image;
    d->ui.cvImageWidget->showImage( d->currentImage );
}

void HomePrism::on_cameraNumber_valueChanged(int)
{
    if( d->ui.showVideoButton->isChecked() )
    {
        d->ui.showVideoButton->click();
        d->capture.release();
    }
}

/*
  HomePrism::HomePrism()
   : m_Manager( new QNetworkAccessManager(this) ),
     m_UpdateTimer(new QTimer(this)),
     m_RedIcon(":/HomePrism/red-dot.png"),
     m_GreenIcon(":/HomePrism/green-dot.png"),
     m_RedGreenIcon(":/HomePrism/red-green-dot.png"),
     m_QuestionIcon(":/HomePrism/question.png"),
     m_PreviousResult(-1),
     m_Message("The Dashboard status is unknown"),
     m_MessageTime(6000),
     m_Title(QString("HomePrism v%1.%2.%3").arg(HomePrism_MAJOR_VERSION).arg(HomePrism_MINOR_VERSION).arg(HomePrism_PATCH_VERSION))
  {
    // setup internals
    m_UpdateTimer = new QTimer(this);
    m_UpdateTimer->setInterval(2000*60);

    // setup UI
    createOptionsGroupBox();
    createActions();
    createTrayIcon();

    m_StatusLabel = new QLabel;
    m_StatusLabel->setText(m_Message);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_StatusLabel);
    mainLayout->addWidget(m_OptionsGroupBox);
    setLayout(mainLayout);

    setWindowTitle( m_Title );
    this->setIcon( m_QuestionIcon );

    // connect
    connect(m_UpdateRateSpinBox, SIGNAL( valueChanged (int) ),
            this, SLOT( on_UpdateRateSpinBoxValue_Changed(int) ));

    connect(m_TrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    connect(m_Manager, SIGNAL(finished(QNetworkReply*)),
      this, SLOT(replyFinished(QNetworkReply*)));

    connect(m_UpdateTimer, SIGNAL(timeout()),
            this, SLOT(fetch()));

    // show/start
    m_TrayIcon->show();
    m_UpdateTimer->start();
    this->fetch();
  }
  void HomePrism::iconActivated(QSystemTrayIcon::ActivationReason reason)
  {
      switch (reason) {
      case QSystemTrayIcon::DoubleClick:
        this->setVisible(!this->isVisible());
          break;click()
      default:
          ;
      }
  }
 void HomePrism::setVisible(bool visible)
 {
     m_RestoreAction->setEnabled(isMaximized() || !visible);
     QWidget::setVisible(visible);
 }

 void HomePrism::closeEvent(QCloseEvent *event)
 {
     if (m_TrayIcon->isVisible())
     {
         hide();
         event->ignore();
     }
 }

 void HomePrism::setIcon(QIcon& icon)
 {
     m_TrayIcon->setIcon(icon);
     this->setWindowIcon(icon);
 }

 void HomePrism::fetch()
 {
     m_Manager->get(QNetworkRequest(QUrl("http://mbits/git-status/getRepoStatus.php")));
 }

 void HomePrism::replyFinished(QNetworkReply* pReply)
 {
   QNetworkReply::NetworkError error =
     pReply->error();

   if( error == QNetworkReply::NoError )
   {

     QByteArray data=pReply->readAll();
     QString str(data);

     int result = str.toInt();
     if( m_PreviousResult != result )
     {
       if( result == 0 )
       {
         m_Message = "MITK and MITK-MBI Dashboard are green";
         this->setIcon( m_GreenIcon );
       }
       else if( result == 1 )
       {
         m_Message = "MITK-MBI Dashboard is red";
         this->setIcon( m_RedGreenIcon );
       }
       else if( result == 2 )
       {
         m_Message = "MITK and MITK-MBI Dashboard are red";
         this->setIcon( m_RedIcon );
       }
       m_PreviousResult = result;
     }
   }
   else
   {
     this->setIcon( m_QuestionIcon );
   }

   m_TrayIcon->showMessage(QString(m_Title),
                           m_Message,
                           QSystemTrayIcon::Information, m_MessageTime);
   m_TrayIcon->setToolTip(m_Message);
   m_StatusLabel->setText(m_Message);
 }

 void HomePrism::createOptionsGroupBox()
 {

   m_UpdateRateLabel = new QLabel(tr("Update Rate:"));

   m_UpdateRateSpinBox = new QSpinBox;
   m_UpdateRateSpinBox->setRange(1, 3600);
   m_UpdateRateSpinBox->setSuffix(" minutes");
   m_UpdateRateSpinBox->setValue(2);

   m_OptionsGroupBoxLayout = new QGridLayout;
   m_OptionsGroupBoxLayout->addWidget(m_UpdateRateLabel, 0, 0);
   m_OptionsGroupBoxLayout->addWidget(m_UpdateRateSpinBox, 0, 1);

   m_OptionsGroupBox = new QGroupBox(tr("Options"));
   m_OptionsGroupBox->setLayout(m_OptionsGroupBoxLayout);
 }

 void HomePrism::createActions()
 {
    m_RestoreAction = new QAction(tr("&Restore"), this);
    connect(m_RestoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    m_QuitAction = new QAction(tr("&Quit"), this);
    connect(m_QuitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
 }

 void HomePrism::createTrayIcon()
 {
     m_TrayIconMenu = new QMenu(this);
     m_TrayIconMenu->addAction(m_RestoreAction);
     m_TrayIconMenu->addAction(m_QuitAction);

     m_TrayIcon = new QSystemTrayIcon(this);
     m_TrayIcon->setContextMenu(m_TrayIconMenu);

 }

 void HomePrism::on_UpdateRateSpinBoxValue_Changed( int i )
 {
   m_UpdateTimer->setInterval( i * 60 * 1000 );
 }
 */
