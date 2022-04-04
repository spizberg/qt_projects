#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <QCloseEvent>
#include "opencv2/opencv.hpp"
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <iostream>
#include <QTimer>
#include <QMainWindow>
#include <fstream>
#include <exception>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event);
    cv::Mat subtractShoeFromBackground(const cv::Mat &img, const int thresh);
    int getMaxAreaContourId(std::vector<std::vector<cv::Point>> contours);
    QStringList getModelNames(const std::string &filename);
    // void saveImage(const cv::Mat &frame);


private:
    Pylon::PylonAutoInitTerm _autoInitTerm;
    Pylon::CBaslerUniversalInstantCamera camera{Pylon::CTlFactory::GetInstance().CreateFirstDevice()};
    Pylon::CImageFormatConverter converter {Pylon::CImageFormatConverter()};
    const QStringList stateList {"Neuve", "Usée"};
    QStringList modelList;

    int threshold {15};

    QTimer * _cameraTimer = nullptr;

    Ui::MainWindow *ui;

private slots:
    void displayImagesTimer();
    void on_seuilSpinBox_valueChanged(int newThreshold);
    void on_captureButton_clicked();
};
#endif // MAINWINDOW_H
