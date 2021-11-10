#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void ReadData();

private slots:
    void on_pushButton_find_clicked();

    void on_pushButton_connect_clicked();

    void on_pushButton_directory_clicked();

    void on_pushButton_view_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *m_serial = nullptr;
};
#endif // MAINWINDOW_H
