// COPYRIGHT AND PERMISSION NOTICE

// Copyright (c) 2020 - 2021, Björn Langels, <sm0sbl@langelspost.se>
// All rights reserved.

// Permission to use, copy, modify, and distribute this software for any purpose
// with or without fee is hereby granted, provided that the above copyright
// notice and this permission notice appear in all copies.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

// Except as contained in this notice, the name of a copyright holder shall not
// be used in advertising or otherwise to promote the sale, use or other dealings
// in this Software without prior written authorization of the copyright holder.

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
//#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QSerialPort>
#include <QBuffer>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QStandardPaths>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


#define MIN(A,B) ((A)<(B)?(A):(B))
#define MAX(A,B) ((A)>(B)?(A):(B))

#define QT_NO_DEBUG_OUTPUT
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void logSlot(QString);

private slots:

    // Buttons and fields
    void on_ComPortName_textChanged(const QString &arg1);
    void on_ConnectToComPort_stateChanged(int comConnect);
    void on_comPortInvert_stateChanged(int arg1);
    void on_ShowComPortList_clicked();
    void on_radioPort_editingFinished();

    // Functions
    void msEvent();
    void readyReadKeyTcp();
    //void readyReadSerial();

    void on_SaveSettings_clicked();
    void on_LoadSettings_clicked();
    void on_comPortDevice_currentIndexChanged(const QString &arg1);
    void on_tcpKeyConnected();
    void on_tcpKeyDisconnected();
    void onKeySocketStateChanged(QAbstractSocket::SocketState socketState);

private:
    void dbgMsg(QString);
    QTextEdit *logView;
    Ui::MainWindow *ui;

    void loadSettings();
    void saveSettings();
    void KeyUp();
    void KeyDown();
    void updateComPortList();

    QString SettingsPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString SettingsFile = "remotecwserver.ini";
    QTcpSocket *tcpKeySocket;
    QTcpServer *tcpKeyServer;
    bool tcpKeyConnected = false;
    QSerialPort *serialPort;
    QByteArray comPort;
    QByteArray hostAddress;
    bool comPortStatus;
    quint32 packetDelay = 0;
    quint32 SetKeyDelayCnt = 0;
    bool ComPortInverted = false;
    // keyctrl is the circular buffer representing milliseconds in the future.
    // Every time a KeyDown or KeyUp event is received the timestamp is modulo
    // the size of this memory buffer (which is hardcoded at the moment).
    // this mean it is not possible to have a larger jitter buffer than ~1 second
    // but if that would be necessary the keyer is kind of useless anyway.
    // My experience is that around 130-180ms is normal when using a quite lousy
    // 4G/LTE connection at the radio side.
    char keyctrl[1024];
    QHostAddress senderIp;
    quint16 senderPort;
    quint32 DownTimer = 0;
    QByteArray rxbuffer;

signals:
    void signal_readyReadTcp();
    void logSignal(QString);
};
#endif // MAINWINDOW_H
