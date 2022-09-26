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

#include <QDateTime>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QSettings>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QThread>
#include "q_debugstream.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this, SIGNAL(logSignal(QString)),
              this, SLOT(logSlot(QString)));

    emit logSignal("Now call Q_DebugStream");

    //Redirect //qDebug() output to dbgMsg(QString)
    new Q_DebugStream(std::cout, this, &MainWindow::dbgMsg);
    Q_DebugStream::registerQDebugMessageHandler();

    //QStringList s;
    comPortStatus = false;

    // Key TCP socket
    tcpKeyServer = new QTcpServer(this);
    if ( !tcpKeyServer->listen(QHostAddress::Any, ui->radioPort->value()) )
    {
        qDebug() << "tcpKeyServer could not start";
    }
    else
    {
        qDebug() << "tcpKeyServer started! Listen on port:" << ui->radioPort->value();
        connect(tcpKeyServer, SIGNAL(newConnection()),
                this,      SLOT(on_tcpKeyConnected()));
    }

    // Serial port
    serialPort = new QSerialPort(this);

    // millisecond timer used to check keyer memory
    QTimer* timer = new QTimer(this);
    timer->connect(timer,
                   SIGNAL(timeout()),
                   this,
                   SLOT(msEvent()));
    timer->start(1); //, Qt::PreciseTimer, timeout());

    ui->CwPadDisplay->setStyleSheet("background-color: white; border:  none;");
    for (int i=0; i<1024; i++)
        keyctrl[i] = 0;

    updateComPortList();



    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
    tcpKeyServer->close();
    serialPort->close();
    delete ui;
}

void MainWindow::logSlot(QString log) {
  ui->dbgTextOut->append(log);
}

void MainWindow::dbgMsg(QString log) {
  emit logSignal(log);
}


void MainWindow::on_tcpKeyDisconnected()
{
    tcpKeyConnected = false;
    //qDebug() << "on_tcpKeyDisconnected";
    for (int i=0; i<1024; i++)
        keyctrl[i] = 0;
    KeyUp();
}


void MainWindow::onKeySocketStateChanged(QAbstractSocket::SocketState socketState)
{
    //qDebug() << "onCmdrSocketStateChanged" << socketState;
}


void MainWindow::on_tcpKeyConnected()
{
    tcpKeyConnected = true;
    //qDebug() << "on_tcpKeyConnected called";
    //QObject().thread()->usleep(1000*1000*2);
    // need to grab the socket
    tcpKeySocket = tcpKeyServer->nextPendingConnection();
    tcpKeySocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    //qDebug() << "DBG: tcpKeySocket->isOpen():" << tcpKeySocket->isOpen();
    //QObject().thread()->usleep(1000*1000*2);
    connect(tcpKeySocket, SIGNAL(readyRead()), this, SLOT(readyReadKeyTcp()));
    connect(tcpKeySocket, SIGNAL(disconnected()),
            this,      SLOT(on_tcpKeyDisconnected()));
    connect(tcpKeySocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(onKeySocketStateChanged(QAbstractSocket::SocketState)));

    tcpKeyConnected = tcpKeySocket->isOpen();
    if ( tcpKeySocket->isOpen() /*&& tcpCmdrSocket->isOpen()*/ ) {
        ui->CwPadDisplay->setStyleSheet("background-color: green; border:  none;");
    }
}

void MainWindow::updateComPortList()
{
    //qDebug() << "DBG: updateComPortList";
    QList<QSerialPortInfo> comDevices = QSerialPortInfo::availablePorts();
    //qDebug() << "on_comPortDevice_highlighted";
    ui->comPortDevice->clear();
    this->ui->comPortDevice->addItem("Select COM port");
    foreach (QSerialPortInfo i, comDevices) {
        this->ui->comPortDevice->addItem(i.portName()+" "+i.description());
        //qDebug() << "comDevice:" << i.portName();
    }
}

void MainWindow::loadSettings() {

    //qDebug() << "loadSettings";
    QSettings settings(SettingsPath + "/" + SettingsFile, QSettings::IniFormat);
    settings.beginGroup("MAIN");
    quint32 val = settings.value("LocalPort", "").toUInt();
    ui->radioPort->setValue(val);
    QString str = settings.value("ComPort", "").toString();
    ui->ComPortName->setText(str);
    str = settings.value("KeyOutput", "").toString();
    if ( !QString::compare(str, "RTS") ) {
        ui->KeyWithRTS->setChecked(true);
        ui->KeyWithDTR->setChecked(false);
    } else if ( !QString::compare(str, "DTR") ) {
        ui->KeyWithRTS->setChecked(false);
        ui->KeyWithDTR->setChecked(true);
    }
    str = settings.value("KeyInvert", "").toString();
    if ( !QString::compare(str, "Inverted") ) {
        ui->comPortInvert->setChecked(true);
    } else {
        ui->comPortInvert->setChecked(false);
    }
    settings.endGroup();
}

void MainWindow::saveSettings()
{
    //qDebug() << "saveSettings";
    QSettings settings(SettingsPath + "/" + SettingsFile, QSettings::IniFormat);
    settings.beginGroup("MAIN");
    settings.setValue("LocalPort",  ui->radioPort->value());
    settings.setValue("ComPort",  ui->ComPortName->text().toLatin1());
    if ( ui->KeyWithRTS->isChecked() ) {
        settings.setValue("KeyOutput",  "RTS");
    } else {
        settings.setValue("KeyOutput",  "DTR");
    }
    if ( ui->comPortInvert->isChecked() ) {
        settings.setValue("KeyInvert",  "Inverted");
    } else {
        settings.setValue("KeyInvert",  "NotInverted");
    }
    settings.endGroup();
}

void MainWindow::msEvent()
{
    static quint32 keyctrlTime = (QDateTime::currentMSecsSinceEpoch() % 4294967295);
    quint32 now = (QDateTime::currentMSecsSinceEpoch() % 4294967295);
    QByteArray tmpbuf;

    // This is a timeout to protect from infinite KeyDown.
    // After 5 seconds with the key down the key is automatically
    // released and a new KeyDown command must arrive from the client
    if ( (DownTimer != 0) && ((now - DownTimer) > 5000) ) {
        KeyUp();
        DownTimer = 0;
    }

    // Loop from last position in circular memory to now.
    // Need to loop as it might have passed more than one
    // ms due to OS overhead
    for ( ; keyctrlTime <= now; keyctrlTime++) {
        if ( keyctrl[keyctrlTime%1024] == 'D' ) {
            keyctrl[keyctrlTime%1024] = 0;
            KeyDown();
        } else if ( keyctrl[keyctrlTime%1024] == 'U' ) {
            keyctrl[keyctrlTime%1024] = 0;
            KeyUp();
        }
    }
}

void MainWindow::KeyUp()
{
    //qDebug() << "KeyUp";
    if ( tcpKeyConnected )
        ui->CwPadDisplay->setStyleSheet("background-color: green; border:  none;");
    else
        ui->CwPadDisplay->setStyleSheet("background-color: white; border:  none;");
    if ( comPortStatus )
    {
        // Send key up to radio port DTR or RTS depending of settings
        if ( ui->KeyWithDTR->isChecked() ) {
            serialPort->QSerialPort::setDataTerminalReady(!ComPortInverted);
        } else if ( ui->KeyWithRTS->isChecked() ) {
            serialPort->QSerialPort::setRequestToSend(!ComPortInverted);
        }
    }

}

void MainWindow::KeyDown()
{
    //qDebug() << "KeyDown";
    ui->CwPadDisplay->setStyleSheet("background-color: red; border:  none;");
    if ( comPortStatus )
    {
        // Send key down to radio port DTR or RTS depending of settings
        if ( ui->KeyWithDTR->isChecked() ) {
            serialPort->QSerialPort::setDataTerminalReady(ComPortInverted);
        } else if ( ui->KeyWithRTS->isChecked() ) {
            serialPort->QSerialPort::setRequestToSend(ComPortInverted);
        }
    }

}


void MainWindow::readyReadKeyTcp()
{
    quint32 timeOrigin, timeSent, timeToSend, now;
    QByteArray txbuffer;
    char str[100];
    //qDebug() << "readyReadKeyTcp called";

    // when data comes in
    qint64 len = tcpKeySocket->bytesAvailable();
    rxbuffer.resize(len);
    tcpKeySocket->read(rxbuffer.data(), len);

    now = (QDateTime::currentMSecsSinceEpoch() % 4294967295);

    //qDebug() << "Received buffer:" << rxbuffer.data();
    sscanf(rxbuffer.data(), "%s %u %u", str, &timeOrigin, &timeToSend);
    if ( !strcmp(str, "CC") ) {
    } else if ( !strcmp(str, "P") ) {
        txbuffer.append("PP ");
    } else if ( !strcmp(str, "KD") ) {
        // This is the key down processing
        DownTimer = now;
        // set the KeyDown command in the circular buffer
        keyctrl[(timeToSend%1024)] = 'D';
        // return an ACK of KeyDown command received
        txbuffer.append("KD ");
    } else if ( !strcmp(str, "KU") ) {
        // this is the key up processing
        DownTimer = 0;
        // set the KeyUp command in the circular buffer
        keyctrl[(timeToSend%1024)] = 'U';
        // return an ACK of KeyUp command received
        txbuffer.append("KU ");
    } else {
        //unknown command
        txbuffer.append("Unknown subcommand received: ");
        txbuffer.append(rxbuffer.data());
    }
    timeSent = (QDateTime::currentMSecsSinceEpoch() % 4294967295);
    sprintf(str, "%u %u %u", timeOrigin, timeSent, timeToSend);
    txbuffer.append(str);
    // Send the ACK
    tcpKeySocket->write(txbuffer.data());
    tcpKeySocket->waitForBytesWritten(1);
}


void MainWindow::on_ComPortName_textChanged(const QString &arg1)
{
    //qDebug() << "DBG: on_ComPortName_textChanged, comPortStatus:"<<comPortStatus;
    ui->ConnectToComPort->setChecked(false);
    comPort = ui->ComPortName->text().toLatin1();
}

void MainWindow::on_ConnectToComPort_stateChanged(int comConnect)
{
    //qDebug() << "DBG: on_ConnectToComPort_stateChanged, comPortStatus:"<<comPortStatus;
    if ( comPortStatus ) {
        serialPort->close();
        comPortStatus = false;
    }
    if ( comConnect ) { // checkbox checked -> connect to com port if possible
        //qDebug()<< "Setting up com port"<< ui->ComPortName->text();
        serialPort->setPortName(ui->ComPortName->text());
        serialPort->setBaudRate(QSerialPort::Baud115200);
        serialPort->setParity(QSerialPort::Parity::NoParity);
        serialPort->setDataBits(QSerialPort::DataBits::Data8);
        serialPort->setStopBits(QSerialPort::StopBits::OneStop);
        serialPort->setFlowControl(QSerialPort::FlowControl::UnknownFlowControl);
        comPortStatus = serialPort->open(QSerialPort::OpenModeFlag::ReadWrite);
        //qDebug() << "com port"<<ui->ComPortName->text()<<"connection result:"<<comPortStatus;
        if ( comPortStatus )
        {
            if ( ui->KeyWithDTR->isChecked() ) {
                serialPort->QSerialPort::setDataTerminalReady(!ComPortInverted);
            } else if ( ui->KeyWithRTS->isChecked() ) {
                serialPort->QSerialPort::setRequestToSend(!ComPortInverted);
            }
        }
        //qDebug()<<"Leaving on_ConnectToComPort_stateChanged";
    }
}

void MainWindow::on_ShowComPortList_clicked()
{
    QStringList ports;
    //qDebug() << "DBG: on_ShowComPortList_clicked";


    for (QSerialPortInfo port : QSerialPortInfo::availablePorts())
    {
        //Their is some sorting to do for just list the port I want, with vendor Id & product Id
        ////qDebug() << port.portName() << Qt::hex << port.vendorIdentifier() << Qt::hex << port.productIdentifier()
        ////qDebug() << port.hasProductIdentifier() << port.hasVendorIdentifier() << port.isBusy();
        ports += port.portName();
    }
    //qDebug() << ports;


}


void MainWindow::on_comPortInvert_stateChanged(int arg1)
{
    //qDebug() << "DBG: on_comPOrtInvert_stateChanged";
    ComPortInverted = (arg1 != 0);
    KeyUp();
    //qDebug() << "on_comPortInvert_stateChanged("<<arg1<<")";
}


void MainWindow::on_radioPort_editingFinished()
{
    //qDebug() << "DBG: on_radioPort_editingFinished";
    //if ( tcpKeySocket->isOpen()) {
        ////qDebug() << "socket is open";
        //tcpKeySocket->waitForBytesWritten(3000);
        ////qDebug() << "Waited, to be closed";
        //tcpKeySocket->close();
   //
    tcpKeyServer->close();
    //qDebug() << "Starting new server";
    if ( !tcpKeyServer->listen(QHostAddress::Any, ui->radioPort->value()) )
    {
        //qDebug() << "tcpKeyServer could not start";
    }
    else
    {
        //qDebug() << "tcpKeyServer started! Listen on port:" << ui->radioPort->value();
    }
}

void MainWindow::on_SaveSettings_clicked()
{
    //qDebug() << "DBG: on_SaveSettings";
    saveSettings();
}

void MainWindow::on_LoadSettings_clicked()
{
    //qDebug() << "DBG: on_LoadSettings_clicked";
    loadSettings();
}

void MainWindow::on_comPortDevice_currentIndexChanged(const QString &arg1)
{
    //qDebug() << "DBG: on_comPOrtDevice_currentIndexChanged";
    QString qstr;
    QStringList list = arg1.split(" ");
    qstr = list[0];
    //qDebug() << "on_comPortDevice_currentIndexChanged to:" << arg1;
    //qDebug() << qstr;
    ui->ComPortName->setText(qstr);
}
