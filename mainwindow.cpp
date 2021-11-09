#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts>
#include <QtCharts/QChartView>

#include "qgridlayout.h"
#include <QGridLayout>
#include "chartview.h"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

QList<QChart*> myChartList;
QList<ChartView*> myChartViewList;
QList<QSplineSeries*> mySplineSeriesList;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      m_serial(new QSerialPort(this))
{
    ui->setupUi(this);

    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::ReadData);

    ui->label_indicator->setStyleSheet("QLabel {color : green; }");

    for (int i = 0; i < 8; i++){
        QSplineSeries *series = new QSplineSeries();
        series->setName("spline");

        QChart *chart1 = new QChart();

        chart1->legend()->hide();
        chart1->addSeries(series);
        chart1->setTitle("Wire №" + QString::number(i+1));
        chart1->createDefaultAxes();
        chart1->axes(Qt::Vertical).first()->setRange(0, 20);
        chart1->axes(Qt::Horizontal).first()->setRange(100, 300);

        ChartView *chartView = new ChartView(chart1);

        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setMinimumSize(300, 150);
        chartView->setMaximumSize(1000, 1000);

        ui->gridLayout->addWidget(chartView, int(i/2), int(i%2));

        mySplineSeriesList.append(series);
        myChartList.append(chart1);
        myChartViewList.append(chartView);
    }
}

void MainWindow::ReadData(){
    QByteArray data = m_serial->readAll();
    QString s(data);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_find_clicked()
{
    ui->comboBox_ports->clear();
    QList<QSerialPortInfo> infoList = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo info, infoList) {
        if (info.productIdentifier() != 0) ui->comboBox_ports->addItem(info.portName());

        qDebug() << info.vendorIdentifier();
        qDebug() << info.productIdentifier();
    }
}


void MainWindow::on_pushButton_connect_clicked()
{
    m_serial->setPortName(ui->comboBox_ports->currentText());
    // указали скорость
    m_serial->setBaudRate(QSerialPort::Baud9600);

    m_serial->setDataBits(QSerialPort::Data8);

    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    // пробуем подключится
    if (!m_serial->open(QIODevice::ReadWrite)) {
        // если подключится не получится, то покажем сообщение с ошибкой
         qDebug() << "Ошибка, не удалось подключится к порту";
        return;
    } else {qDebug() << "connect";}
}

