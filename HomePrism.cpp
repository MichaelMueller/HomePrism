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

using namespace cv;

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
  std::vector<cv::Mat> previousImages;
  QLabel *statusLabel;
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
    QCoreApplication::applicationName(), this);

  d->snapshotTimer = new QTimer(this);
  d->snapshotTimer->setObjectName("snapshotTimer");

  d->wipeTimer = new QTimer(this);
  d->wipeTimer->setInterval(1000 * 60);
  d->wipeTimer->setObjectName("wipeTimer");

  d->ui.setupUi(this);
  setWindowTitle(HomePrism_TITLE);
  d->statusLabel = new QLabel(this);
  d->ui.statusbar->addPermanentWidget(d->statusLabel, 1);
  this->LoadData();
}

HomePrism::~HomePrism()
{
  delete d;
  d = 0;
}

void HomePrism::on_showVideoButton_toggled(bool checked)
{
  if (checked)
  {
    d->captureTimer->start();
  }
  else
  {
    d->captureTimer->stop();
  }
  this->SaveData();
}

void HomePrism::on_captureTimer_timeout()
{
  if (!this->readCurrentImage())
  {
    d->ui.showVideoButton->click();
    return;
  }

  if (d->ui.movementDetection->isChecked())
  {
    cv::Rect rect;
    if (this->getMovementDetectionRectangle(rect))
    {
      cv::Scalar colour(0, 0, 255);
      int thickness = 3;
      int linetype = 4;
      cv::rectangle(d->currentImage, rect.tl(), rect.br(), colour, thickness, linetype);
    }
  }

  d->ui.cvImageWidget->showImage(d->currentImage);
}

bool HomePrism::getMovementDetectionRectangle(cv::Rect& rect)
{
  rect.x = d->ui.upperLeftCornerX->value();
  rect.y = d->ui.upperLeftCornerY->value();
  rect.width = d->ui.rectWidth->value();
  rect.height = d->ui.rectHeight->value();

  if (d->ui.showVideoButton->isChecked() || d->ui.recordingButton->isChecked())
  {
    if (rect.x < d->currentImage.cols
      && rect.y < d->currentImage.rows
      && rect.x + rect.width <= d->currentImage.cols
      && rect.y + rect.height <= d->currentImage.rows
      && rect.width > 2
      && rect.height > 2)
    {
      return true;
    }
    else
    {
      this->showError("Invalid values for rectangle (out of bounds)");
      return false;
    }
  }
}

void HomePrism::on_cameraNumber_valueChanged(int)
{
  if (d->ui.showVideoButton->isChecked())
  {
    d->ui.showVideoButton->click();
    d->capture.release();
  }
  this->SaveData();
}

void HomePrism::on_snapshotSelectButton_clicked()
{
  QString dir = QFileDialog::getExistingDirectory(this, tr(HomePrism_TITLE));
  if (!dir.isEmpty())
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
  if (checked)
  {
    if (d->ui.movementDetection->isChecked())
    {
      d->snapshotTimer->setInterval(d->ui.movementDetectionDelay->value());
    }
    else
    {
      d->snapshotTimer->setInterval(this->toMilliSeconds(d->ui.snapshotDelay->value()));
    }
    d->snapshotTimer->start();
  }
  else
  {
    d->snapshotTimer->stop();
  }
}
// Check if there is motion in the result matrix
// count the number of changes and return.
int detectMotion(const Mat & motion, Mat & result, Mat & result_cropped,
  int x_start, int x_stop, int y_start, int y_stop,
  int max_deviation,
  Scalar & color)
{
  // calculate the standard deviation
  Scalar mean, stddev;
  meanStdDev(motion, mean, stddev);
  // if not to much changes then the motion is real (neglect agressive snow, temporary sunlight)
  if (stddev[0] < max_deviation)
  {
    int number_of_changes = 0;
    int min_x = motion.cols, max_x = 0;
    int min_y = motion.rows, max_y = 0;
    // loop over image and detect changes
    for (int j = y_start; j < y_stop; j += 2){ // height
      for (int i = x_start; i < x_stop; i += 2){ // width
        // check if at pixel (j,i) intensity is equal to 255
        // this means that the pixel is different in the sequence
        // of images (prev_frame, current_frame, next_frame)
        if (static_cast<int>(motion.at<unsigned char>(cv::Point(j, i))) == 255)
        {
          number_of_changes++;
          if (min_x>i) min_x = i;
          if (max_x < i) max_x = i;
          if (min_y > j) min_y = j;
          if (max_y < j) max_y = j;
        }
      }
    }
    if (number_of_changes){
      //check if not out of bounds
      if (min_x - 10 > 0) min_x -= 10;
      if (min_y - 10 > 0) min_y -= 10;
      if (max_x + 10 < result.cols - 1) max_x += 10;
      if (max_y + 10 < result.rows - 1) max_y += 10;
      // draw rectangle round the changed pixel
      Point x(min_x, min_y);
      Point y(max_x, max_y);
      Rect rect(x, y);
      Mat cropped = result(rect);
      cropped.copyTo(result_cropped);
      rectangle(result, rect, color, 1);
    }
    return number_of_changes;
  }
  return 0;
}

bool HomePrism::detectMotion()
{
  d->previousImages.push_back(d->currentImage);
  int maxSize = 3;
  if (d->previousImages.size() > maxSize)
  {
    std::vector<Mat> newVec;
    for (int i = 1; i < d->previousImages.size(); i++)
    {
      newVec.push_back(d->previousImages[i]);
    }
    d->previousImages = newVec;
  }

  if (d->previousImages.size() < 3)
    return false;

  // d1 and d2 for calculating the differences
  // result, the result of and operation, calculated on d1 and d2
  // number_of_changes, the amount of changes in the result matrix.
  // color, the color for drawing the rectangle when something has changed.
  Mat d1, d2, motion;
  int number_of_changes, number_of_sequence = 0;
  Scalar mean_, color(0, 255, 255); // yellow

  // Detect motion in window
  cv::Rect rect;
  if (!this->getMovementDetectionRectangle(rect))
  {
    return false;
  }
  int x_start = rect.tl().x, x_stop = rect.br().x;
  int y_start = rect.tl().y, y_stop = rect.br().y;

  // If more than 'there_is_motion' pixels are changed, we say there is motion
  // and store an image on disk
  int there_is_motion = d->ui.detectionSensitivity->value();

  // Maximum deviation of the image, the higher the value, the more motion is allowed
  int max_deviation = 20;

  // Erode kernel
  Mat kernel_ero = getStructuringElement(MORPH_RECT, Size(2, 2));

  // Take images and convert them to gray
  Mat prev_frame = d->previousImages[0];
  Mat current_frame = d->previousImages[1];
  Mat next_frame = d->previousImages[2];
  Mat result = next_frame;
  Mat result_cropped;
  cvtColor(current_frame, current_frame, CV_RGB2GRAY);
  cvtColor(prev_frame, prev_frame, CV_RGB2GRAY);
  cvtColor(next_frame, next_frame, CV_RGB2GRAY);

  // Calc differences between the images and do AND-operation
  // threshold image, low differences are ignored (ex. contrast change due to sunlight)
  absdiff(prev_frame, next_frame, d1);
  absdiff(next_frame, current_frame, d2);
  bitwise_and(d1, d2, motion);
  threshold(motion, motion, 35, 255, CV_THRESH_BINARY);
  erode(motion, motion, kernel_ero);
#ifdef _DEBUG
  imshow("test", motion);
  waitKey(5);
#endif
  number_of_changes = ::detectMotion(motion, result, result_cropped, x_start, x_stop, y_start, y_stop, max_deviation, color);

  return (number_of_changes >= there_is_motion);
}

void HomePrism::showError(const QString& text)
{
  QString newText;
  QDateTime dateTime = QDateTime::currentDateTime();
  newText = dateTime.toString() + QString(": ") + text;

  QListWidgetItem* pItem = new QListWidgetItem(newText);
  pItem->setForeground(Qt::red); // sets red text
  //pItem->setBackground(Qt::green); // sets green background
  d->ui.log->addItem(pItem);
  //d->statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold;}");
  //d->statusLabel->setText(text);
}
void HomePrism::showText(const QString& text)
{
  QString newText;
  QDateTime dateTime = QDateTime::currentDateTime();
  newText = dateTime.toString() + QString(": ") + text;

  //d->statusLabel->setStyleSheet("QLabel { color: black; font-weight: bold;}");
  //d->statusLabel->setText(text);
  QListWidgetItem* pItem = new QListWidgetItem(newText);
  //pItem->setBackground(Qt::green); // sets green background
  d->ui.log->addItem(pItem);
  d->ui.log->scrollToBottom();
}

bool HomePrism::skipRecording()
{
  if (d->ui.skipDetection->isChecked())
  {
    QDateTime dateTime = QDateTime::currentDateTime();

    int day = dateTime.date().dayOfWeek();
    bool dayMatches = (day == 1 && d->ui.skipMo->isChecked())
      || (day == 2 && d->ui.skipTu->isChecked())
      || (day == 3 && d->ui.skipWe->isChecked())
      || (day == 4 && d->ui.skipTh->isChecked())
      || (day == 5 && d->ui.skipFr->isChecked())
      || (day == 6 && d->ui.skipSa->isChecked())
      || (day == 7 && d->ui.skipSu->isChecked());

    if (dayMatches)
    {
      QTime current = dateTime.time();
      QTime from = d->ui.skipFromTime->time();
      QTime to = d->ui.skipToTime->time();

      if (from <= to)
      {
        return current > from && current < to;
      }
      else
      {
        QTime oneMinuteBeforeMidnight = QTime::fromString("23:59", "HH:mm");
        QTime midnight = QTime::fromString("00:00", "HH:mm");
        bool todayInTime = (current > from && current <= oneMinuteBeforeMidnight);
        bool tomorrowInTime = (current >= midnight && current < to);
        return todayInTime || tomorrowInTime;
      }
    }
  }
  return false;
}

void HomePrism::on_snapshotTimer_timeout()
{
  if (this->skipRecording())
    return;

  if (!this->readCurrentImage())
  {
    d->ui.recordingButton->click();
    return;
  }
  else
  {
    if (d->ui.movementDetection->isChecked())
    {
      if (!this->detectMotion())
      {
        return;
      }
      else
      {
        this->showText("!!! Motion detected !!!");
      }
    }

    this->saveSnapshot();
  }
}

void HomePrism::saveSnapshot()
{
  //get current date and time
  QDateTime dateTime = QDateTime::currentDateTime();
  QString dateTimeString = dateTime.toString("yyMMdd-hhmmss");

  // dirs
  QString dirStr = d->ui.snapshotDirectory->text();
  if (dirStr.isEmpty())
    dirStr = ".";
  dirStr = dirStr + QDir::separator();
  bool createSubDir = d->ui.snapshotCreateSubdirectories->isChecked();
  if (createSubDir)
  {
    dirStr = dirStr + dateTime.toString("yyMMdd-hh") + QDir::separator();
    QDir newDir(dirStr);
    if (!newDir.exists())
    {
      if (!newDir.mkdir(dirStr))
      {
        QString err = QString("Error: Cannot create directory %1!").arg(dirStr);
        this->showError(err);
        return;
      }
    }
  }

  // ext
  QString ext = QString(".") + d->ui.snapshotFormat->currentText();

  // create filename
  QString baseFileName = dateTimeString;
  int i = 1;
  QFileInfo f(dirStr + baseFileName + ext);
  while (f.exists())
  {
    f = QFileInfo(dirStr + baseFileName + QString::number(i) + ext);
    ++i;
  }

  std::string finalFileName = f.absoluteFilePath().toStdString();
  if (!cv::imwrite(finalFileName, d->currentImage))
  {
    QString err = QString("Error: Cannot write snapshot %1 (please check filename, permissions)!").arg(QString::fromStdString(finalFileName));
    this->showError(err);
    return;
  }
  /*
  else
  {
  QString info = QString("Snapshot %1 saved!").arg(QString::fromStdString(finalFileName));
  this->showError(info);
  }
  */
  if (d->ui.wipeBySize->isChecked())
  {
    QFileInfoList fileList = this->getFileInfoList();
    qint64 kbSize = 0;
    for (int i = 0; i < fileList.size(); ++i)
    {
      const QFileInfo& info = fileList.at(i);
      kbSize = kbSize + (info.size() / 1000);
    }

    qint64 allowedSize = d->ui.wipeAmount->value();
    if (d->ui.wipeSizeMultiplier->currentIndex() == 1)
      allowedSize = allowedSize * 1000;
    else if (d->ui.wipeSizeMultiplier->currentIndex() == 2)
      allowedSize = allowedSize * 1000 * 1000;

    if (kbSize > allowedSize)
    {
      while (kbSize > allowedSize && fileList.size() > 0)
      {
        QFileInfo info = fileList.at(0);
        kbSize = kbSize - (info.size() / 1000);
        QFile file(info.absoluteFilePath());
        if (!file.remove())
        {
          QString err = QString("File %1 could not be wiped!").arg(info.absoluteFilePath());
          this->showError(err);
        }
        else
        {
          QString message = QString("File %1 wiped!").arg(info.absoluteFilePath());
          this->showText(message);
        }
        fileList.removeAt(0);
      }
    }
  }
}
void HomePrism::on_wipeCheckBox_toggled(bool checked)
{
  this->SaveData();
  if (checked)
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
void HomePrism::on_wipeBySize_toggled(bool checked)
{
  this->SaveData();
  if (checked)
    d->wipeTimer->start();
  else
    d->wipeTimer->stop();
}

void HomePrism::on_wipeAmount_valueChanged(int)
{
  this->SaveData();
}

void HomePrism::on_wipeSizeMultiplier_currentIndexChanged(int i)
{
  this->SaveData();
}

void HomePrism::on_movementDetection_toggled(bool b)
{
  d->ui.snapshotDelayLabel->setEnabled(!b);
  d->ui.snapshotDelay->setEnabled(!b);
  d->previousImages.clear();
  this->SaveData();

  d->ui.recordingButton->click();
  d->ui.recordingButton->click();
}

void HomePrism::on_upperLeftCornerX_valueChanged(int)
{
  this->SaveData();
}

void HomePrism::on_upperLeftCornerY_valueChanged(int)
{
  this->SaveData();
}

void HomePrism::on_rectWidth_valueChanged(int)
{
  this->SaveData();
}

void HomePrism::on_rectHeight_valueChanged(int)
{
  this->SaveData();
}

void HomePrism::on_movementDetectionDelay_valueChanged(int delay)
{
  d->snapshotTimer->setInterval(delay);
  this->SaveData();
}

void HomePrism::on_detectionSensitivity_valueChanged(int)
{
  this->SaveData();
}

void HomePrism::on_skipDetection_toggled(bool)
{
  this->SaveData();
}

void HomePrism::on_skipMo_toggled(bool)
{
  this->SaveData();
}

void HomePrism::on_skipTu_toggled(bool)
{
  this->SaveData();
}

void HomePrism::on_skipWe_toggled(bool)
{
  this->SaveData();
}

void HomePrism::on_skipTh_toggled(bool)
{
  this->SaveData();
}

void HomePrism::on_skipFr_toggled(bool)
{
  this->SaveData();
}

void HomePrism::on_skipSa_toggled(bool)
{
  this->SaveData();
}

void HomePrism::on_skipSu_toggled(bool)
{
  this->SaveData();
}

void HomePrism::on_skipFromTime_timeChanged(const QTime&)
{
  this->SaveData();
}

void HomePrism::on_skipToTime_timeChanged(const QTime&)
{
  this->SaveData();
}

QFileInfoList HomePrism::getFileInfoList()
{
  QDir dir(d->ui.snapshotDirectory->text());

  QStringList filters;
  for (int i = 0; i < d->ui.snapshotFormat->count(); ++i)
    filters << QString(QString("*.") + d->ui.snapshotFormat->itemText(i));
  //dir.setNameFilters(filters);

  QFileInfoList fileList = dir.entryInfoList(filters);
  return fileList;
}

void HomePrism::on_wipeTimer_timeout()
{
  QDateTime currentDateTime = QDateTime::currentDateTime();
  qint64 currentTime = currentDateTime.toMSecsSinceEpoch();
  // minutes
  qint64 multiplier = 1000 * 60;
  QString wipeTimeMultiplierString = d->ui.wipeTimeMultiplier->currentText();
  if (wipeTimeMultiplierString == "hours")
    multiplier = multiplier * 60;
  else if (wipeTimeMultiplierString == "days")
    multiplier = multiplier * 60 * 24;

  qint64 wipeTime = d->ui.wipeTime->value();
  qint64 allowedAge = wipeTime*multiplier;

  QFileInfoList fileList = this->getFileInfoList();
  for (int i = 0; i < fileList.size(); ++i)
  {
    qint64 modTime = fileList.at(i).lastModified().toMSecsSinceEpoch();
    qint64 age = currentTime - modTime;
    if (age > allowedAge)
    {
      QFile file(fileList.at(i).absoluteFilePath());
      if (!file.remove())
      {
        QString err = QString("File %1 could not be wiped!").arg(fileList.at(i).absoluteFilePath());
        this->showError(err);
      }
    }
  }
}

bool HomePrism::readCurrentImage()
{
  if (!d->capture.isOpened())
  {
    d->capture.open(d->ui.cameraNumber->value());
  }

  cv::Mat image;
  bool readImage = d->capture.read(image);
  if (!readImage)
  {
    this->showError("Error: Cannot read images from capture device (Wrong capture number?)!");
    return false;
  }
  d->currentImage = image;
  return true;
}

void HomePrism::SaveData()
{
  qDebug() << "saving settings";
  d->settings->setValue("cameraNumber", d->ui.cameraNumber->value());
  d->settings->setValue("snapshotDirectory", d->ui.snapshotDirectory->text());
  d->settings->setValue("snapshotCreateSubdirectories", d->ui.snapshotCreateSubdirectories->isChecked());
  d->settings->setValue("snapshotFormat", d->ui.snapshotFormat->currentText());
  d->settings->setValue("snapshotDelay", d->ui.snapshotDelay->value());

  d->settings->setValue("wipeCheckBox", d->ui.wipeCheckBox->isChecked());
  d->settings->setValue("wipeTime", d->ui.wipeTime->value());
  d->settings->setValue("wipeTimeMultiplier", d->ui.wipeTimeMultiplier->currentText());

  d->settings->setValue("wipeBySize", d->ui.wipeBySize->isChecked());
  d->settings->setValue("wipeAmount", d->ui.wipeAmount->value());
  d->settings->setValue("wipeSizeMultiplier", d->ui.wipeSizeMultiplier->currentText());

  d->settings->setValue("movementDetection", d->ui.movementDetection->isChecked());
  d->settings->setValue("upperLeftCornerX", d->ui.upperLeftCornerX->value());
  d->settings->setValue("upperLeftCornerY", d->ui.upperLeftCornerY->value());
  d->settings->setValue("rectWidth", d->ui.rectWidth->value());
  d->settings->setValue("rectHeight", d->ui.rectHeight->value());
  d->settings->setValue("movementDetectionDelay", d->ui.movementDetectionDelay->value());
  d->settings->setValue("detectionSensitivity", d->ui.detectionSensitivity->value());

  d->settings->setValue("geometry", saveGeometry());
  d->settings->setValue("windowState", saveState());
  d->settings->setValue("showVideo", d->ui.showVideoButton->isChecked());
  d->settings->setValue("skipDetection", d->ui.skipDetection->isChecked());
  d->settings->setValue("skipMo", d->ui.skipMo->isChecked());
  d->settings->setValue("skipTu", d->ui.skipTu->isChecked());
  d->settings->setValue("skipWe", d->ui.skipWe->isChecked());
  d->settings->setValue("skipTh", d->ui.skipTh->isChecked());
  d->settings->setValue("skipFr", d->ui.skipFr->isChecked());
  d->settings->setValue("skipSa", d->ui.skipSa->isChecked());
  d->settings->setValue("skipSu", d->ui.skipSu->isChecked());
  d->settings->setValue("skipFromTime", d->ui.skipFromTime->time());
  d->settings->setValue("skipToTime", d->ui.skipToTime->time());
  d->settings->sync();

  qDebug() << "settings saved: "
    << "cameraNumber=" << d->settings->value("cameraNumber").toInt()
    << ", snapshotDirectory=" << d->settings->value("snapshotDirectory").toString()
    << ", snapshotCreateSubdirectories=" << d->settings->value("snapshotCreateSubdirectories").toBool()
    << ", snapshotFormat=" << d->settings->value("snapshotFormat").toString()
    << ", snapshotDelay=" << d->settings->value("snapshotDelay").toDouble()
    << ", wipeCheckBox=" << d->settings->value("wipeCheckBox").toBool()
    << ", wipeTime=" << d->settings->value("wipeTime").toInt()
    << ", wipeTimeMultiplier=" << d->settings->value("wipeTimeMultiplier").toString()
    << ", wipeBySize=" << d->settings->value("wipeBySize").toBool()
    << ", wipeAmount=" << d->settings->value("wipeAmount").toInt()
    << ", wipeSizeMultiplier=" << d->settings->value("wipeSizeMultiplier").toString()
    << ", movementDetection=" << d->settings->value("movementDetection").toInt()
    << ", upperLeftCornerX=" << d->settings->value("upperLeftCornerX").toInt()
    << ", upperLeftCornerY=" << d->settings->value("upperLeftCornerY").toInt()
    << ", rectWidth=" << d->settings->value("rectWidth").toInt()
    << ", rectHeight=" << d->settings->value("rectHeight").toInt()
    << ", movementDetectionDelay=" << d->settings->value("movementDetectionDelay").toInt()
    << ", detectionSensitivity=" << d->settings->value("detectionSensitivity").toInt()
    << ", skipDetection=" << d->settings->value("skipDetection").toInt()
    << ", skipMo=" << d->settings->value("skipMo").toInt()
    << ", skipTu=" << d->settings->value("skipTu").toInt()
    << ", skipWe=" << d->settings->value("skipWe").toInt()
    << ", skipTh=" << d->settings->value("skipTh").toInt()
    << ", skipFr=" << d->settings->value("skipFr").toInt()
    << ", skipSa=" << d->settings->value("skipSa").toInt()
    << ", skipSu=" << d->settings->value("skipSu").toInt()
    << ", skipFromTime=" << d->settings->value("skipFromTime").toString()
    << ", skipToTime=" << d->settings->value("skipToTime").toString();
}
void HomePrism::closeEvent(QCloseEvent *event)
{
  this->SaveData();
}
void HomePrism::LoadData()
{
  qDebug() << "loading settings";

  d->settings->sync();

  // cameraNumber
  int cameraNumber = 0;
  if (d->settings->contains("cameraNumber"))
    cameraNumber = d->settings->value("cameraNumber").toInt();

  // snapshotDirectory
  QString snapshotDirectory = "";
  if (d->settings->contains("snapshotDirectory"))
    snapshotDirectory = d->settings->value("snapshotDirectory").toString();
  if (snapshotDirectory.isEmpty())
    snapshotDirectory = QDir::currentPath();

  // snapshotCreateSubdirectories
  bool snapshotCreateSubdirectories = false;
  if (d->settings->contains("snapshotCreateSubdirectories"))
    snapshotCreateSubdirectories = d->settings->value("snapshotCreateSubdirectories").toBool();

  // snapshotFormat
  int snapshotFormatIndex = 0;
  if (d->settings->contains("snapshotFormat"))
  {
    int index = d->ui.snapshotFormat->findText(d->settings->value("snapshotFormat").toString());
    if (index >= 0)
      snapshotFormatIndex = index;
  }

  // snapshotDelay
  double snapshotDelay = 1;
  if (d->settings->contains("snapshotDelay"))
    snapshotDelay = d->settings->value("snapshotDelay").toDouble();

  // wipeCheckBox
  bool wipeCheckBox = false;
  if (d->settings->contains("wipeCheckBox"))
    wipeCheckBox = d->settings->value("wipeCheckBox").toBool();

  // cameraNumber
  int wipeTime = 1;
  if (d->settings->contains("wipeTime"))
    wipeTime = d->settings->value("wipeTime").toInt();

  // wipeTimeMultiplier
  int wipeTimeMultiplierIndex = 0;
  if (d->settings->contains("wipeTimeMultiplier"))
  {
    int index = d->ui.wipeTimeMultiplier->findText(d->settings->value("wipeTimeMultiplier").toString());
    if (index >= 0)
      wipeTimeMultiplierIndex = index;
  }

  // wipeBySize
  bool wipeBySize = false;
  if (d->settings->contains("wipeBySize"))
    wipeBySize = d->settings->value("wipeBySize").toBool();

  // wipeAmount
  int wipeAmount = 1;
  if (d->settings->contains("wipeAmount"))
    wipeAmount = d->settings->value("wipeAmount").toInt();

  // wipeSizeMultiplier
  int wipeSizeMultiplierIndex = 0;
  if (d->settings->contains("wipeSizeMultiplier"))
  {
    int index = d->ui.wipeSizeMultiplier->findText(d->settings->value("wipeSizeMultiplier").toString());
    if (index >= 0)
      wipeSizeMultiplierIndex = index;
  }

  // movementDetection
  bool movementDetection = false;
  if (d->settings->contains("movementDetection"))
    movementDetection = d->settings->value("movementDetection").toBool();

  // upperLeftCornerX
  int upperLeftCornerX = 0;
  if (d->settings->contains("upperLeftCornerX"))
    upperLeftCornerX = d->settings->value("upperLeftCornerX").toInt();

  // upperLeftCornerY
  int upperLeftCornerY = 0;
  if (d->settings->contains("upperLeftCornerY"))
    upperLeftCornerY = d->settings->value("upperLeftCornerY").toInt();

  // rectWidth
  int rectWidth = 0;
  if (d->settings->contains("rectWidth"))
    rectWidth = d->settings->value("rectWidth").toInt();

  // rectHeight
  int rectHeight = 0;
  if (d->settings->contains("rectHeight"))
    rectHeight = d->settings->value("rectHeight").toInt();

  // movementDetectionDelay
  int movementDetectionDelay = 0;
  if (d->settings->contains("movementDetectionDelay"))
    movementDetectionDelay = d->settings->value("movementDetectionDelay").toInt();

  // detectionSensitivity
  int detectionSensitivity = 0;
  if (d->settings->contains("detectionSensitivity"))
    detectionSensitivity = d->settings->value("detectionSensitivity").toInt();

  // geometry
  if (d->settings->contains("geometry"))
    restoreGeometry(d->settings->value("geometry").toByteArray());

  // windowState
  if (d->settings->contains("windowState"))
    restoreState(d->settings->value("myWidget/windowState").toByteArray());

  // showVideo
  bool showVideo = false;
  if (d->settings->contains("showVideo"))
    showVideo = d->settings->value("showVideo").toBool();

  // skipDetection
  bool skipDetection = false;
  if (d->settings->contains("skipDetection"))
    skipDetection = d->settings->value("skipDetection").toBool();

  // skipMo
  bool skipMo = false;
  if (d->settings->contains("skipMo"))
    skipMo = d->settings->value("skipMo").toBool();

  // skipTu
  bool skipTu = false;
  if (d->settings->contains("skipTu"))
    skipTu = d->settings->value("skipTu").toBool();

  // skipWe
  bool skipWe = false;
  if (d->settings->contains("skipWe"))
    skipWe = d->settings->value("skipWe").toBool();

  // skipTh
  bool skipTh = false;
  if (d->settings->contains("skipTh"))
    skipTh = d->settings->value("skipTh").toBool();

  // skipFr
  bool skipFr = false;
  if (d->settings->contains("skipFr"))
    skipFr = d->settings->value("skipFr").toBool();

  // skipSa
  bool skipSa = false;
  if (d->settings->contains("skipSa"))
    skipSa = d->settings->value("skipSa").toBool();

  // skipSu
  bool skipSu = false;
  if (d->settings->contains("skipSu"))
    skipSu = d->settings->value("skipSu").toBool();

  // skipFromTime
  QTime skipFromTime;
  if (d->settings->contains("skipFromTime"))
    skipFromTime = d->settings->value("skipFromTime").toTime();

  // skipToTime
  QTime skipToTime;
  if (d->settings->contains("skipToTime"))
    skipToTime = d->settings->value("skipToTime").toTime();

  qDebug() << "settings loaded: "
    << "cameraNumber=" << d->settings->value("cameraNumber").toInt()
    << ", snapshotDirectory=" << d->settings->value("snapshotDirectory").toString()
    << ", snapshotCreateSubdirectories=" << d->settings->value("snapshotCreateSubdirectories").toBool()
    << ", snapshotFormat=" << d->settings->value("snapshotFormat").toString()
    << ", snapshotDelay=" << d->settings->value("snapshotDelay").toDouble()
    << ", wipeCheckBox=" << d->settings->value("wipeCheckBox").toBool()
    << ", wipeTime=" << d->settings->value("wipeTime").toInt()
    << ", wipeTimeMultiplier=" << d->settings->value("wipeTimeMultiplier").toString()
    << ", wipeBySize=" << d->settings->value("wipeBySize").toBool()
    << ", wipeAmount=" << d->settings->value("wipeAmount").toInt()
    << ", wipeSizeMultiplier=" << d->settings->value("wipeSizeMultiplier").toString()
    << ", movementDetection=" << d->settings->value("movementDetection").toBool()
    << ", upperLeftCornerX=" << d->settings->value("upperLeftCornerX").toInt()
    << ", upperLeftCornerY=" << d->settings->value("upperLeftCornerY").toInt()
    << ", rectWidth=" << d->settings->value("rectWidth").toInt()
    << ", rectHeight=" << d->settings->value("rectHeight").toInt()
    << ", movementDetectionDelay=" << d->settings->value("movementDetectionDelay").toInt()
    << ", detectionSensitivity=" << d->settings->value("detectionSensitivity").toInt()
    << ", skipDetection=" << d->settings->value("skipDetection").toBool()
    << ", skipMo=" << d->settings->value("skipMo").toBool()
    << ", skipTu=" << d->settings->value("skipTu").toBool()
    << ", skipWe=" << d->settings->value("skipWe").toBool()
    << ", skipTh=" << d->settings->value("skipTh").toBool()
    << ", skipFr=" << d->settings->value("skipFr").toBool()
    << ", skipSa=" << d->settings->value("skipSa").toBool()
    << ", skipSu=" << d->settings->value("skipSu").toBool()
    << ", skipFromTime=" << d->settings->value("skipFromTime").toString()
    << ", skipToTime=" << d->settings->value("skipToTime").toString();

  // set ui
  d->ui.cameraNumber->setValue(cameraNumber);
  d->ui.snapshotDirectory->setText(snapshotDirectory);
  d->ui.snapshotCreateSubdirectories->setChecked(snapshotCreateSubdirectories);
  d->ui.snapshotFormat->setCurrentIndex(snapshotFormatIndex);
  d->ui.snapshotDelay->setValue(snapshotDelay);
  d->ui.wipeCheckBox->setChecked(wipeCheckBox);
  d->ui.wipeTime->setValue(wipeTime);
  d->ui.wipeTimeMultiplier->setCurrentIndex(wipeTimeMultiplierIndex);
  d->ui.wipeBySize->setChecked(wipeBySize);
  d->ui.wipeAmount->setValue(wipeAmount);
  d->ui.wipeSizeMultiplier->setCurrentIndex(wipeSizeMultiplierIndex);

  // initialize values for timers etc
  d->snapshotTimer->setInterval(this->toMilliSeconds(d->ui.snapshotDelay->value()));
  d->ui.movementDetection->setChecked(movementDetection);
  d->ui.upperLeftCornerX->setValue(upperLeftCornerX);
  d->ui.upperLeftCornerY->setValue(upperLeftCornerY);
  d->ui.rectWidth->setValue(rectWidth);
  d->ui.rectHeight->setValue(rectHeight);
  d->ui.movementDetectionDelay->setValue(movementDetectionDelay);
  d->ui.detectionSensitivity->setValue(detectionSensitivity);

  d->ui.skipDetection->setChecked(skipDetection);
  d->ui.skipDetection->setChecked(skipDetection);
  d->ui.skipMo->setChecked(skipMo);

  d->ui.skipDetection->setChecked(skipDetection);
  d->ui.skipMo->setChecked(skipMo);
  d->ui.skipTu->setChecked(skipTu);
  d->ui.skipWe->setChecked(skipWe);
  d->ui.skipTh->setChecked(skipTh);
  d->ui.skipFr->setChecked(skipFr);
  d->ui.skipSa->setChecked(skipSa);
  d->ui.skipSu->setChecked(skipSu);
  d->ui.skipFromTime->setTime(skipFromTime);
  d->ui.skipToTime->setTime(skipToTime);

  if (showVideo)
    d->ui.showVideoButton->click();
}