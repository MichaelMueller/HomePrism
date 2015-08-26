#ifndef HomePrism_H
#define HomePrism_H

#include <cv.h>
#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QGridLayout>
#include <QDateTime>
#include <QDir>

class QAction;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;
class QSpinBox;
class QTextEdit;
class HomePrismData;

class HomePrism : public QMainWindow
{
  Q_OBJECT

public:
  HomePrism();
  virtual ~HomePrism();

  protected slots:
  void on_showVideoButton_toggled(bool);
  void on_captureTimer_timeout();
  void on_cameraNumber_valueChanged(int);
  void on_snapshotSelectButton_clicked();
  void on_snapshotCreateSubdirectories_toggled(bool);
  void on_snapshotFormat_currentIndexChanged(int i);
  void on_snapshotDelay_valueChanged(double);

  void on_recordingButton_toggled(bool);
  void on_snapshotTimer_timeout();

  void on_wipeCheckBox_toggled(bool);
  void on_wipeTime_valueChanged(int);
  void on_wipeTimeMultiplier_currentIndexChanged(int i);
  void on_wipeTimer_timeout();

  void on_wipeBySize_toggled(bool);
  void on_wipeAmount_valueChanged(int);
  void on_wipeSizeMultiplier_currentIndexChanged(int i);

  void on_movementDetection_toggled(bool);
  void on_upperLeftCornerX_valueChanged(int);
  void on_upperLeftCornerY_valueChanged(int);
  void on_rectWidth_valueChanged(int);
  void on_rectHeight_valueChanged(int);
  void on_movementDetectionDelay_valueChanged(int);
  void on_detectionSensitivity_valueChanged(int);

  void on_skipDetection_toggled(bool);
  void on_skipMo_toggled(bool);
  void on_skipTu_toggled(bool);
  void on_skipWe_toggled(bool);
  void on_skipTh_toggled(bool);
  void on_skipFr_toggled(bool);
  void on_skipSa_toggled(bool);
  void on_skipSu_toggled(bool);
  void on_skipFromTime_timeChanged(const QTime&);
  void on_skipToTime_timeChanged(const QTime&);

  void closeEvent(QCloseEvent *event);
private:
  bool getMovementDetectionRectangle(cv::Rect& rect);
  bool readCurrentImage();
  void SaveData();
  void LoadData();
  int toMilliSeconds(double seconds);
  bool detectMotion();
  void saveSnapshot();
  bool skipRecording();
  void showError(const QString& text);
  void showText(const QString& text);
  QFileInfoList getFileInfoList();
  HomePrismData* d;
};

#endif
