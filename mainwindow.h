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
#include <chrono>
#include <QDir>
#include <QMessageBox>
#include <QFile>


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
    int getMaxAreaContourId(std::vector<std::vector<cv::Point>> &contours);
    QStringList getModelNames(const std::string &filename);
    void writeNewModelName(const QString &modelName);
    void writeInfoModel(const QString &modelName, const QString &color, const QString &sku);

private:
    Pylon::PylonAutoInitTerm _autoInitTerm;
    Pylon::CBaslerUniversalInstantCamera camera{Pylon::CTlFactory::GetInstance().CreateFirstDevice()};
    Pylon::CImageFormatConverter converter {Pylon::CImageFormatConverter()};
    cv::Mat realShoeImage;
    cv::Mat subtractedShoeImage;
    cv::Rect boundingBox;
    const QStringList stateList {"Neuve", "Usée"};
    const QStringList marqueList {"DECATHLON", "ERAM"};
    QStringList modelList;
    const QDir imageFolder {"/home/nathan/Code/ID_SHOES_PICTURES/IDSHOES_2/PHOTO_CELL/WHITE_DATA_FOLDER/real_images/"};
    const std::string classFile {"/home/nathan/QtProjects/CellulePhoto/classes.txt"};
    const std::string infoFile {"/home/nathan/QtProjects/CellulePhoto/models_info.txt"};
    bool pause{false};

    int threshold {15};

    QTimer * _cameraTimer = nullptr;

    Ui::MainWindow *ui;

private slots:
    void displayImagesTimer();
    void on_seuilSpinBox_valueChanged(int newThreshold);
    void on_captureButton_clicked();
    void saveResultImage();

    void on_modelComboBox_currentTextChanged(const QString &arg1);
};
#endif // MAINWINDOW_H
