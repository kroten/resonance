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

#include "windows.h"

QList<QChart*> myChartList;
QList<ChartView*> myChartViewList;
QList<QSplineSeries*> mySplineSeriesList;

QFile mfile;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      m_serial(new QSerialPort(this))
{
    ui->setupUi(this);

    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::ReadData);
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::PortClosed);
    //connect(m_serial, &QSerialPort::errorOccurred, this, qDebug() << m_serial->errorString());
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
float max = 18;
QString word = "";
QString values[9];
QString freqs[8];
QString spans[8];

bool above_threshold[8];
double start_res[8];
double start_res_temp[8];
double end_res[8];
double maxi_res[8];
//double spans[8];
bool find_max[8];



void MainWindow::ReadData(){
    QByteArray data = m_serial->readAll();
    QString s(data);

    //qDebug() << s[0];
    for(int k = 0; k<s.size(); k++){
        qDebug() << s[k];
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
                if (word.toDouble() > max) {
                    max = word.toDouble();
                    for (int i = 0; i < 8; i++){
                        myChartList[i]->axes(Qt::Vertical).first()->setRange(0, max + 2);
                    }
                }
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
            qDebug() << "here";
            recieve = false;
            if (thresh){
                QMessageBox::information(this, "", "successfull");
                thresh = false;
            }
            if (ending){
                ending = false;
                ui->label_frequency->setText("0");
                qDebug() << "in_ending";

                if (ui->checkBox_save->isChecked()){
                    mfile.close();
                    mfile.open(QIODevice::ReadWrite);
                    QByteArray ba;
                    QString str = "max:\n";
                    ba = str.toUtf8();
                    ba.toBase64();
                    mfile.write(ba);
                    str = freqs[0];
                    for (int l = 1; l < 8; l++){
                        str.append("," + freqs[l]);
                    }
                    str.append("\n");
                    ba = str.toUtf8();
                    ba.toBase64();
                    mfile.write(ba);
                    str = "span:\n";
                    ba = str.toUtf8();
                    ba.toBase64();
                    mfile.write(ba);
                    str = spans[0];
                    for (int l = 1; l < 8; l++){
                        str.append("," + spans[l]);
                    }
                    str.append("\n");
                    ba = str.toUtf8();
                    ba.toBase64();
                    mfile.write(ba);

                    mfile.close();
                }
            }
            if (finish){
                unlocked_all();
                finish = false;
                ui->label_indicator->setText("OFF");
                ui->label_indicator->setStyleSheet("QLabel {color : green; }");
                ui->pushButton_connect->setEnabled(1);
                ui->pushButton_find->setEnabled(1);
                ui->pushButton_view->setEnabled(1);
                ui->pushButton_stop->setDisabled(1);
                ui->comboBox_ports->setEnabled(1);
                max = 18;
                qDebug() << "in_finish"; 
            }
            if (datas){
                qDebug() << element_num;
                values[element_num] = word;
                element_num = 0;
                word = "";
                datas = false;
                ui->label_frequency->setText(values[8]);
                for (int l = 0; l < 8; l++){
                    mySplineSeriesList[l]->append(values[8].toDouble(), values[l].toDouble());
                    if (values[l].toDouble() > ui->lineEdit_threshold->text().toDouble()){ // ВОТ ТУТ ПОШЛА ЖАРА СПАН И ФРЕК
                        above_threshold[l] = true;
                        start_res_temp[l] = values[8].toDouble();
                    } else {
                        above_threshold[l] = false;
                        if (find_max[l]) {
                            find_max[l] = false;
                            end_res[l] = values[8].toDouble();
                            //freqs[l] = QString::number(maxi_res[l]);
                            spans[l] = QString::number(end_res[l] - start_res[l]);
                            switch (l){
                            case 0:
                                ui->freqW0->setText(freqs[l]);
                                ui->spanW0->setText(spans[l]);
                                break;
                            case 1:
                                ui->freqW1->setText(freqs[l]);
                                ui->spanW1->setText(spans[l]);
                                break;
                            case 2:
                                ui->freqW2->setText(freqs[l]);
                                ui->spanW2->setText(spans[l]);
                                break;
                            case 3:
                                ui->freqW3->setText(freqs[l]);
                                ui->spanW3->setText(spans[l]);
                                break;
                            case 4:
                                ui->freqW4->setText(freqs[l]);
                                ui->spanW4->setText(spans[l]);
                                break;
                            case 5:
                                ui->freqW5->setText(freqs[l]);
                                ui->spanW5->setText(spans[l]);
                                break;
                            case 6:
                                ui->freqW6->setText(freqs[l]);
                                ui->spanW6->setText(spans[l]);
                                break;
                            case 7:
                                ui->freqW7->setText(freqs[l]);
                                ui->spanW7->setText(spans[l]);
                                break;
                            }
                        }
                    }
                    if (above_threshold[l]){
                        if (values[l].toDouble() > maxi_res[l]){
                            start_res[l] = start_res_temp[l];
                            find_max[l] = true;
                            maxi_res[l] = values[l].toDouble();
                            freqs[l] = values[8];
                        }
                    } // ВОТ ТУТ ЖАРА ЗАКАНЧИВАЕТСЯ
                }
                if (ui->checkBox_save->isChecked()){
                    QString str = values[8];
                    for (int l = 0; l < 8; l++){
                        str.append("," + values[l]);
                    }
                    str.append("\n");
                    QByteArray ba;
                    ba = str.toUtf8();
                    ba.toBase64();
                    mfile.write(ba);
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
    ui->label->setDisabled(1);
    ui->label_2->setDisabled(1);
    ui->label_3->setDisabled(1);
    ui->pushButton_stop->setDisabled(1);
}

void MainWindow::clear_SpanFreq() {
    ui->freqW0->setText("0.00");
    ui->freqW1->setText("0.00");
    ui->freqW2->setText("0.00");
    ui->freqW3->setText("0.00");
    ui->freqW4->setText("0.00");
    ui->freqW5->setText("0.00");
    ui->freqW6->setText("0.00");
    ui->freqW7->setText("0.00");

    ui->spanW0->setText("0.00");
    ui->spanW1->setText("0.00");
    ui->spanW2->setText("0.00");
    ui->spanW3->setText("0.00");
    ui->spanW4->setText("0.00");
    ui->spanW5->setText("0.00");
    ui->spanW6->setText("0.00");
    ui->spanW7->setText("0.00");
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
    ui->label->setEnabled(1);
    ui->label_2->setEnabled(1);
    ui->label_3->setEnabled(1);
}

void MainWindow::PortClosed(){
    if (m_serial->error() == QSerialPort::PermissionError) {
        qDebug() << m_serial->error();
        locked_all();
//        clear_SpanFreq();
//        for (int i = 0; i < 8; i++){
//            mySplineSeriesList.at(i)->clear();
//            myChartList[i]->axes(Qt::Horizontal).first()->setRange(ui->lineEdit_start->text().toDouble(), ui->lineEdit_stop->text().toDouble());
//            myChartList[i]->axes(Qt::Vertical).first()->setRange(0, 20);
//        }

        if(m_serial->isOpen()){m_serial->close();}
        ui->comboBox_ports->clear();
        QList<QSerialPortInfo> infoList = QSerialPortInfo::availablePorts();
        foreach(QSerialPortInfo info, infoList) {
            if (info.productIdentifier() != 0) ui->comboBox_ports->addItem(info.portName());
        }

        ui->label_indicator->setText("OFF");
        ui->label_indicator->setStyleSheet("QLabel {color : green; }");
        ui->label_frequency->setText("0");
        ui->lineEdit_threshold->setText("4.2");

        ui->pushButton_find->setEnabled(1);
        ui->pushButton_connect->setEnabled(1);
        ui->pushButton_view->setEnabled(1);
        ui->comboBox_ports->setEnabled(1);

        QMessageBox::warning(this, "Attention","SerialPort closed. Reload the plate");

    } else if (m_serial->error() == QSerialPort::WriteError) {QMessageBox::warning(this, "Attention","Something wrong. Reload all");}
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_pushButton_find_clicked()
{
    if (m_serial->isOpen()){m_serial->close();}
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
    if (m_serial->isOpen()) {
        locked_all();
        m_serial->close();}
    m_serial->setPortName(ui->comboBox_ports->currentText());
    m_serial->setBaudRate(QSerialPort::Baud9600);
    //m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    // указали скорость
    //if (!m_serial->open(QIODevice::ReadWrite)){}



    // пробуем подключится
    if (!m_serial->open(QIODevice::ReadWrite)) {
        // если подключится не получится, то покажем сообщение с ошибкой
         qDebug() << "Ошибка, не удалось подключится к порту";
         locked_all();
         m_serial->close();
         QMessageBox::warning(this, "Attention","Unable to connect");
         ui->comboBox_ports->clear();
         QList<QSerialPortInfo> infoList = QSerialPortInfo::availablePorts();
         foreach(QSerialPortInfo info, infoList) {
             if (info.productIdentifier() != 0) ui->comboBox_ports->addItem(info.portName());

             qDebug() << info.vendorIdentifier();
             qDebug() << info.productIdentifier();
         }
         return;
    } else {
//        m_serial->setBaudRate(QSerialPort::Baud9600);
//        m_serial->setDataBits(QSerialPort::Data8);
//        m_serial->setFlowControl(QSerialPort::NoFlowControl);

        qDebug() << "connect";
        Sleep(2000);
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

    clear_SpanFreq();

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
    if(ui->checkBox_save->isChecked()){
        mfile.setFileName("");
        bool bOk;
        QString str = QInputDialog::getText( 0,
                                             "Set file name",
                                             "File name:",
                                             QLineEdit::Normal,
                                             "",
                                             &bOk
                                            );
        if (!bOk) {
            // Была нажата кнопка Cancel
            ui->checkBox_save->setChecked(0);
        } else {
            QString name = ui->lineEdit_directory->text();
            QDir dir(name);
            if (dir.exists()){
                qDebug() << "директория на месте";
            } else {
                 dir.mkdir(name);
            }
            name.append("/" + str + ".csv");
            mfile.setFileName(name);
            mfile.open(QIODevice::WriteOnly);

            QByteArray ba;
            QString str = "                        ";
            ba = str.toUtf8();
            ba.toBase64();
            for (int i = 0; i < 5; i++){
                mfile.write(ba);
            }
            str = "\nfreq, v0, v1, v2, v3, v4, v5, v6, v7\n";
            ba = str.toUtf8();
            ba.toBase64();
            mfile.write(ba);
        }
    }
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

    max = 18;

    for (int i = 0; i < 8; i++){
        mySplineSeriesList.at(i)->clear();
        myChartList[i]->axes(Qt::Horizontal).first()->setRange(ui->lineEdit_start->text().toDouble(), ui->lineEdit_stop->text().toDouble());
        myChartList[i]->axes(Qt::Vertical).first()->setRange(0, 20);
        freqs[i] = "0.00";
        spans[i] = "0.00";
        above_threshold[i] = false;
        start_res[i] = 0;
        start_res_temp[i] = 0;
        end_res[i] = 0;
        maxi_res[i] = 0;
        find_max[i] = false;
    }

    clear_SpanFreq();

    recieve = 0;
    datas = 0;
    thresh = 0;
    ending = 0;
    finish = 0;
    element_num = 0;
    QString word = "";
    locked_all();
    clear_SpanFreq();
    ui->pushButton_stop->setEnabled(1);
    ui->pushButton_connect->setDisabled(1);
    ui->pushButton_find->setDisabled(1);
    ui->pushButton_view->setDisabled(1);
    ui->comboBox_ports->setDisabled(1);
    ui->pushButton_stop->setEnabled(1);
    ui->label_indicator->setText("ON");
    ui->label_indicator->setStyleSheet("QLabel {color : red; }");
}


void MainWindow::on_pushButton_stop_clicked()
{
    QByteArray ba;
    QString str = "^f;";
    ba = str.toUtf8();
    ba.toBase64();
    m_serial->write(ba);
    qDebug() << ba;
    ui->pushButton_stop->setDisabled(1);
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

