#ifndef WIDGET_H
#define WIDGET_H


#include <QWidget>
#include <QByteArray>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QList>
#include <QMessageBox>
#include <QDateTime>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void findPorts();
    bool initSerialPort();
    void sendMsg(const QString &msg);
    void flushText();


public slots:
    void recvMsg();

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;
};
#endif // WIDGET_H
