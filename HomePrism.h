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

private:
    HomePrismData* d;

    /*
    void setVisible(bool visible);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_UpdateRateSpinBoxValue_Changed( int i );
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void replyFinished(QNetworkReply*);
    void fetch();


private:
    void setIcon(QIcon& index);
    void createOptionsGroupBox();
    void createActions();
    void createTrayIcon();

    QLabel* m_StatusLabel;

    QLabel* m_UpdateRateLabel;
    QSpinBox* m_UpdateRateSpinBox;
    QGridLayout* m_OptionsGroupBoxLayout;
    QGroupBox* m_OptionsGroupBox;

    QAction* m_RestoreAction;
    QAction* m_QuitAction;

    QSystemTrayIcon* m_TrayIcon;
    QMenu* m_TrayIconMenu;
    QNetworkAccessManager* m_Manager;

    QTimer* m_UpdateTimer;

    QIcon m_RedIcon;
    QIcon m_GreenIcon;
    QIcon m_RedGreenIcon;
    QIcon m_QuestionIcon;
    int m_PreviousResult;
    QString m_Message;
    int m_MessageTime;
    QString m_Title;
*/
};

#endif
