#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts>
#include <QtCharts/QChartView>

#include "qgridlayout.h"
#include <QGridLayout>
#include "chartview.h"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QDialog>
#include <QFileDialog>

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
    connect(ui->checkBox_auto, &QCheckBox::stateChanged, this, &MainWindow::change_mode);
    connect(ui->checkBox_step, &QCheckBox::stateChanged, this, &MainWindow::change_mode_1);

    ui->label_indicator->setStyleSheet("QLabel {color : green; }");

    QString path = QDir::currentPath() + "/results";
    ui->lineEdit_directory->setText(path);

    ui->lineEdit_start->setText("100");
    ui->lineEdit_stop->setText("300");
    ui->lineEdit_threshold->setText("4.2");

    locked_all();

    QList<QSerialPortInfo> infoList = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo info, infoList) {
        if (info.productIdentifier() != 0) ui->comboBox_ports->addItem(info.portName());

        qDebug() << info.vendorIdentifier();
        qDebug() << info.productIdentifier();
    }

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

bool recieve = 0;
bool datas = 0;
bool thresh = 0;
bool ending = 0;
bool finish = 0;
int element_num = 0;
QString word = "";
QString values[9];

void MainWindow::ReadData(){
    QByteArray data = m_serial->readAll();
    QString s(data);

    //qDebug() << s[0];
    for(int k = 0; k<s.size(); k++){
        //qDebug() << s[k];
        if (s[k] == '^') {recieve = true; qDebug() << s[k];}
        else if (recieve && !datas && !thresh && !ending && !finish) {
            if (s[k] == 'd'){
                datas = true;
                qDebug() << "datas";
            }
            else if (s[k] == 't'){thresh = true;}
            else if (s[k] == 'e'){ending = true;}
            else if (s[k] == 'f'){finish = true;}
        }
        else if (recieve && datas && s[k] != ';') {
            if (s[k] == '_'){
                qDebug() << word;
                if (word == "") continue;
                values[element_num] = word;
                element_num++;
                word = "";
            }
            else {
                word.append(s[k]);
            }
        }
        //else if (recieve && thresh && k != ';') {}
        //else if (recieve && ending && k != ';') {}
        //else if (recieve && finish && k != ';') {}
        else if (s[k] == ';') {
            recieve = false;
            if (thresh){}
            if (ending){}
            if (finish){}
            if (datas){
                qDebug() << element_num;
                values[element_num] = word;
                element_num = 0;
                word = "";
                datas = false;
                for (int l = 0; l < 8; l++){
                    mySplineSeriesList[l]->append(values[8].toDouble(), values[l].toDouble());
                }
            }
        }
    }
}

void MainWindow::change_mode(){
    ui->checkBox_step->setChecked(!ui->checkBox_auto->checkState());
}

void MainWindow::change_mode_1(){
    ui->checkBox_auto->setChecked(!ui->checkBox_step->checkState());
}

void MainWindow::locked_all(){
    ui->checkBox_auto->setDisabled(1);
    ui->checkBox_save->setDisabled(1);
    ui->checkBox_step->setDisabled(1);
    ui->lineEdit_directory->setDisabled(1);
    ui->lineEdit_start->setDisabled(1);
    ui->lineEdit_stop->setDisabled(1);
    ui->lineEdit_threshold->setDisabled(1);
    ui->pushButton_change->setDisabled(1);
    ui->pushButton_directory->setDisabled(1);
    ui->pushButton_start->setDisabled(1);
    ui->pushButton_stop->setDisabled(1);
    ui->label->setDisabled(1);
    ui->label_2->setDisabled(1);
    ui->label_3->setDisabled(1);
}

void MainWindow::unlocked_all(){
    ui->checkBox_auto->setEnabled(1);
    ui->checkBox_save->setEnabled(1);
    ui->checkBox_step->setEnabled(1);
    ui->lineEdit_directory->setEnabled(1);
    ui->lineEdit_start->setEnabled(1);
    ui->lineEdit_stop->setEnabled(1);
    ui->lineEdit_threshold->setEnabled(1);
    ui->pushButton_change->setEnabled(1);
    ui->pushButton_directory->setEnabled(1);
    ui->pushButton_start->setEnabled(1);
    ui->pushButton_stop->setEnabled(1);
    ui->label->setEnabled(1);
    ui->label_2->setEnabled(1);
    ui->label_3->setEnabled(1);
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_pushButton_find_clicked()
{
    m_serial->close();
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
         locked_all();
         m_serial->close();
         QMessageBox::warning(this, "Attention","Unable to connect");
         return;
    } else {
        qDebug() << "connect";
        unlocked_all();
    }
}


void MainWindow::on_pushButton_directory_clicked()
{
    QString path = QFileDialog::getExistingDirectory();
    ui->lineEdit_directory->setText(path);
    if (path == ""){
        path = QDir::currentPath() + "/results";
        ui->lineEdit_directory->setText(path);
    }
}


void MainWindow::on_pushButton_view_clicked()
{
    for (int i = 0; i < 8; i++){
        mySplineSeriesList.at(i)->clear();
        myChartList[i]->axes(Qt::Horizontal).first()->setRange(ui->lineEdit_start->text().toDouble(), ui->lineEdit_stop->text().toDouble());
        myChartList[i]->axes(Qt::Vertical).first()->setRange(0, 20);
    }

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open File"), ui->lineEdit_directory->text(), tr("Csv Files (*.csv)"));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.errorString();

        return;
    }

    int i = 0;
    double start, stop, max = 0;
    QStringList res_freq;
    while (!file.atEnd()) {
        QString line = file.readLine();
        i++;

        if (i == 2) {
            res_freq.append(line.split(','));
            double num = res_freq.at(7).toDouble();
            ui->freqW0->setText(res_freq.at(0));
            ui->freqW1->setText(res_freq.at(1));
            ui->freqW2->setText(res_freq.at(2));
            ui->freqW3->setText(res_freq.at(3));
            ui->freqW4->setText(res_freq.at(4));
            ui->freqW5->setText(res_freq.at(5));
            ui->freqW6->setText(res_freq.at(6));
            ui->freqW7->setText(QString::number(num));
        }
        if (i == 4) {
            QStringList wordList;
            wordList.append(line.split(','));
            double num = wordList.at(7).toDouble();
            ui->spanW0->setText(wordList.at(0));
            ui->spanW1->setText(wordList.at(1));
            ui->spanW2->setText(wordList.at(2));
            ui->spanW3->setText(wordList.at(3));
            ui->spanW4->setText(wordList.at(4));
            ui->spanW5->setText(wordList.at(5));
            ui->spanW6->setText(wordList.at(6));
            ui->spanW7->setText(QString::number(num));
        }
        if (i < 7)
            continue;
        QStringList wordList;
        wordList.append(line.split(','));
        if (i == 7) start = wordList.at(0).toDouble();
        for (int k = 0; k < 8; k++){
            mySplineSeriesList[k]->append(wordList.at(0).toDouble(), wordList.at(k+1).toDouble());
        }
        if ((wordList.at(0) == res_freq.at(0))||(wordList.at(0) == res_freq.at(1))||(wordList.at(0) == res_freq.at(2))||
                (wordList.at(0) == res_freq.at(3))||(wordList.at(0) == res_freq.at(4))||(wordList.at(0) == res_freq.at(5))||
                (wordList.at(0) == res_freq.at(6))||(wordList.at(0) == res_freq.at(7))) {
            for (int k = 1; k < 9; k++) {
                if (wordList.at(k).toDouble() > max) {
                    max = wordList.at(k).toDouble();
                }
            }
        }
        if (file.atEnd()) stop = wordList.at(0).toDouble();
    }
    if (max == 0) max = 18;
    for (int k = 0; k < 8; k++){
       myChartList[k]->axes(Qt::Vertical).first()->setRange(0, max + 2);
       myChartList[k]->axes(Qt::Horizontal).first()->setRange(start, stop);
    }
}


void MainWindow::on_pushButton_start_clicked()
{
    QByteArray ba;
    QString str = "^s_";
    str.append(ui->lineEdit_start->text());
    str.append('_');
    str.append(ui->lineEdit_stop->text());
    str.append('_');
    if(ui->checkBox_auto->isChecked()){str.append('1');}
    else str.append('0');
    str.append(';');
    ba = str.toUtf8();
    ba.toBase64();
    m_serial->write(ba);
    qDebug() << ba;

    for (int i = 0; i < 8; i++){
        mySplineSeriesList.at(i)->clear();
        myChartList[i]->axes(Qt::Horizontal).first()->setRange(ui->lineEdit_start->text().toDouble(), ui->lineEdit_stop->text().toDouble());
        myChartList[i]->axes(Qt::Vertical).first()->setRange(0, 20);
    }
    recieve = 0;
    datas = 0;
    thresh = 0;
    ending = 0;
    finish = 0;
    element_num = 0;
    QString word = "";
}


void MainWindow::on_pushButton_stop_clicked()
{
    QByteArray ba;
    QString str = "^f;";
    ba = str.toUtf8();
    ba.toBase64();
    m_serial->write(ba);
    qDebug() << ba;
}


void MainWindow::on_pushButton_change_clicked()
{
    QByteArray ba;
    QString str = "^t_";
    str.append(ui->lineEdit_threshold->text());
    str.append(';');
    ba = str.toUtf8();
    ba.toBase64();
    m_serial->write(ba);
    qDebug() << ba;
}

