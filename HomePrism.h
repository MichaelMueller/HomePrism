#ifndef HomePrism_H
#define HomePrism_H

#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QGridLayout>

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
    void on_snapshotFormat_currentIndexChanged (int i);
    void on_snapshotDelay_valueChanged(double);
    void on_recordingButton_toggled(bool);
    void on_snapshotTimer_timeout();

private:
    bool readCurrentImage();
    void SaveData();
    void LoadData();
    HomePrismData* d;
    int toMilliSeconds(double seconds);
};

#endif
