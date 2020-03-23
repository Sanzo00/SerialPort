#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

QByteArray dataText;
QByteArray dataText2;
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

    ui->hexRecv->setChecked(true);
    ui->hexSend->setChecked(true);

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
       dataText2.clear();
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
    if (ui->hexRecv->isChecked()) { // show hex
        QString str = dataText.toHex(' ').toUpper();
        ui->recvData->appendPlainText(str);
    }else { // show char
        ui->recvData->appendPlainText(QString(dataText));
    }
    ui->recvData_2->clear();
    ui->recvData_2->appendPlainText(QString(dataText2));
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
    QString ret = dataProcess(msg.toHex().toUpper());
    qDebug() << ret;
    dataText.append(msg);
    dataText2.append(ret + "\n");
    flushText();
}

QString MainWindow::dataProcess(QString str)
{
    int len = str.length();
    qDebug() << len;
    qDebug() << str;
    if (len < 28 || str.mid(0,4) != "EECC") {
        qDebug() << "数据有误!";
        return "数据有误!";
    }
    QString ret;
    int sensorType = str.mid(4, 2).toInt(NULL, 16);
    qDebug() << str.mid(4, 2) << " " << sensorType;
    QString data = str.mid(10, 12);
    int HH, HL, TH, TL;
    double humidity, temperature;
    int XH, YH, ZH, XL, YL, ZL;
    double X, Y, Z;

    int num = data.mid(10, 2).toInt();
    switch (sensorType) {
    case 8:
        // 三轴加速度: XH, XL, YH, YL, ZH, ZL
        ret += "三轴加速度传感器: ";
        XH = data.mid(0, 2).toInt();
        XL= data.mid(0, 2).toInt();
        YH = data.mid(0, 2).toInt();
        YL = data.mid(0, 2).toInt();
        ZH = data.mid(0, 2).toInt();
        ZL = data.mid(0, 2).toInt();
        X = (XH * 256 + XL) * 0.0039;
        Y = (YH * 256 + YL) * 0.0039;
        Z = (ZH * 256 + ZL) * 0.0039;
        ret += QString("{X:%1, Y:%2, Z:%3}").arg(X).arg(Y).arg(Z);
        break;
    case 10:
        // 温湿度: 00, 00, HH, HL, TH, TL
        ret += "温湿度传感器: ";
        HH = data.mid(4, 2).toInt();
        HL = data.mid(6, 2).toInt();
        TH = data.mid(8, 2).toInt();
        TL = data.mid(10, 2).toInt();

        humidity = (HH * 256 + HL) / 10.0;
        temperature = (TH * 256 + TL) / 10.0;
        ret += QString("{湿度:%1%, %2°C}").arg(humidity).arg(temperature);

        break;
    case 2:
        // 光照
        ret += "光照传感器: ";
        if (num == 0) ret += "无光照";
        else if (num == 1) ret += "有光照";
        else ret += "数据格式错误";

        break;
    case 7:
        // 人体检测
        ret += "人体检测传感器: ";
        if (num == 0) ret += "无人";
        else if (num == 1) ret += "有人";
        else ret += "数据格式错误";

        break;
    case 14:
        // 声光
        ret += "声光传感器: ";
        if (num == 0) ret += "关闭";
        else if (num == 1) ret += "打开";
        else ret += "数据格式错误";

        break;
    case 16: // 步进电机
        ret += "步进电机传感器: ";
        if (num == 0) ret += "关闭";
        else if (num == 1) ret += "打开";
        else ret += "数据格式错误";

        break;
    case 15: // 继电器
        ret += "继电器传感器: ";
        if (num == 0) ret += "关闭";
        else if (num == 1) ret += "打开";
        else ret += "数据格式错误";

        break;
    default:
        ret += "未知类型的传感器";
        break;
    }
    if (len > 28) { // 判断是否包含多个指令
        int pos = 28;
        while (pos + 3 < len) { // 找到下个指令的开始位置
            if (str.mid(pos, 4) == "EECC") {
                break;
            }
            pos++;
        }
        if (pos < len) {
            return ret + "\n" + dataProcess(str.mid(pos)); // 递归调用dataProcess
        }
    }
    return ret;
}
