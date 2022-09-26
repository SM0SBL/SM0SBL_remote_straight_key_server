#ifndef Q_DEBUGSTREAM_H
#define Q_DEBUGSTREAM_H

#include <iostream>
#include <streambuf>
#include <string>
#include <QString>
#include "mainwindow.h"

class Q_DebugStream : public std::basic_streambuf<char> {

public:
    Q_DebugStream(std::ostream &stream, MainWindow* obj, void (MainWindow::*dbgMsgPtr)(QString log)): m_stream(stream) {
        m_old_buf = stream.rdbuf();
        stream.rdbuf(this);
        msgObj = obj;
        msgHandler = dbgMsgPtr;
    }

    ~Q_DebugStream() {
        m_stream.rdbuf(m_old_buf);
    }

    static void registerQDebugMessageHandler() {
        qInstallMessageHandler(myQDebugMessageHandler);
    }

private:

    static void myQDebugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        QString message = msg;
        switch (type) {
          case QtDebugMsg:
            message.prepend("qDbg(): ");
            break;
          case QtWarningMsg:
            message.prepend("qWarn(): ");
            break;
          case QtCriticalMsg:
            message.prepend("qCrit(): ");
            break;
          case QtInfoMsg:
            message.prepend("qInfo(): ");
            break;
          case QtFatalMsg:
            message.prepend("qFatal(): ");
            abort();
            break;
        }
        message.append(" (" + QString::fromUtf8(context.file) + ")");
        message.append(" line: " + QString::number(context.line));
        std::cout << message.toStdString().c_str();
    }

protected:
    //This is called when a std::endl has been inserted into the stream
    virtual int_type overflow(int_type v) {
        if (v == '\n') {
            (msgObj->*msgHandler)("\n");
        }
        return v;
    }


    virtual std::streamsize xsputn(const char *p, std::streamsize n) {
        QString str(p);
        if(str.contains("\n")) {
            QStringList strSplitted = str.split("\n");
            (msgObj->*msgHandler)(strSplitted.at(0)); //Index 0 is still on the same old line
            for(int i = 1; i < strSplitted.size(); i++) {
                (msgObj->*msgHandler)("\\    " + strSplitted.at(i));
            }
        } else {
            (msgObj->*msgHandler)(str);
        }
        return n;
    }

private:
    std::ostream &m_stream;
    std::streambuf *m_old_buf;
    MainWindow* msgObj;
    void (MainWindow::*msgHandler)(QString);

};

#endif // Q_DEBUGSTREAM_H
