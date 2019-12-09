#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

QByteArray dataText;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->github, &QAction::triggered, [=](){
        QDesktopServices::openUrl(QUrl("https://github.com/Sanzona"));
    });
    connect(ui->readMe, &QAction::triggered, [=](){
        QDesktopServices::openUrl(QUrl("https://github.com/Sanzona/SerialPort/blob/master/README.md"));
    });

    ui->charRecv->setChecked(true);
    ui->charSend->setChecked(true);

    serialPort = new QSerialPort;
    findPorts();

    // open port
    connect(ui->openPort, &QCheckBox::toggled, [=](bool checked){
       if (checked == true) {
           qDebug() << "Open";
           initSerialPort();
           ui->btnSend->setEnabled(true);
       }else {
           this->serialPort->close();
           ui->btnSend->setEnabled(false);
       }
    });

    // recv
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(recvMsg()));

    // send
    connect(ui->btnSend, &QPushButton::clicked, [=](){
       sendMsg(ui->message->toPlainText());
    });

    // clear recv data
    connect(ui->btnClear, &QPushButton::clicked, [=](){
       dataText.clear();
       flushText();
    });

    // hex <-> char
    connect(ui->hexRecv, &QRadioButton::toggled, [=](){
        flushText();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::findPorts()
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    int cnt = 0;
    for (int i = 0; i < ports.size(); ++i) {
        if (ports.at(i).isBusy()) continue;
        cnt++;
        ui->portName->addItem(ports.at(i).portName());
    }
    if (cnt == 0) {
        QMessageBox::warning(NULL, "错误", "没有空闲端口！");
    }
    return;
}

bool MainWindow::initSerialPort()
{
    int bits;
    // port
    serialPort->setPortName(ui->portName->currentText());
    if (serialPort->open(QIODevice::ReadWrite) == false) {
        QMessageBox::warning(NULL, "错误", "打开失败");
        return false;
    }

    // baudRate
    serialPort->setBaudRate(ui->baudRate->currentText().toInt());

    // dataBits
    bits = ui->dataBits->currentText().toInt();
    if (bits == 8) serialPort->setDataBits(QSerialPort::Data8);
    else if (bits == 7) serialPort->setDataBits(QSerialPort::Data7);
    else if (bits == 6) serialPort->setDataBits(QSerialPort::Data6);
    else if (bits == 5) serialPort->setDataBits(QSerialPort::Data5);

    // stopBits
    bits = ui->stopBits->currentText().toInt();
    if (bits == 1) serialPort->setStopBits(QSerialPort::OneStop);
    else if (bits == 2) serialPort->setStopBits(QSerialPort::TwoStop);

    // parityBits
    QString str = ui->parityBits->currentText();
    if (str == "无") serialPort->setParity(QSerialPort::NoParity);
    else if (str == "偶校验") serialPort->setParity(QSerialPort::EvenParity);
    else if (str == "奇校验") serialPort->setParity(QSerialPort::OddParity);
    return true;
}

// data <-> char
void MainWindow::flushText()
{
    ui->recvData->clear();
    qDebug() << QString(dataText);
    if (ui->hexRecv->isChecked()) { // show hex
        QString str = dataText.toHex(' ').toUpper();
        qDebug() << "hex: " << str;
        ui->recvData->appendPlainText(str);
    }else { // show char
        ui->recvData->appendPlainText(QString(dataText));
    }
}

void MainWindow::sendMsg(const QString &msg)
{
    if (ui->charSend->isChecked()) { // send hex
        serialPort->write(msg.toLatin1());
    }else { // send char
        serialPort->write(QByteArray::fromHex(msg.toLatin1()));
    }
}

void MainWindow::recvMsg()
{
    QByteArray msg = this->serialPort->readAll();
    qDebug() << "msg: " <<  msg;
    if (msg.isEmpty()) return;
    dataText.append(msg);
    flushText();
}
