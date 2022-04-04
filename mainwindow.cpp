#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    camera.Open();
    camera.ExposureTime.SetValue(10000);
    camera.StartGrabbing(Pylon::GrabStrategy_LatestImages);
    converter.OutputPixelFormat = Pylon::PixelType_BGR8packed;
    converter.OutputBitAlignment = Pylon::OutputBitAlignment_MsbAligned;

    ui->stateComboBox->addItems(stateList);
    modelList = getModelNames("/home/nathan/QtProjects/CellulePhoto/classes.txt");
    ui->modelComboBox->addItems(modelList);

    _cameraTimer = new QTimer();
    _cameraTimer->setInterval(10);
    _cameraTimer->start();
    connect(_cameraTimer, SIGNAL(timeout()), this, SLOT(displayImagesTimer()));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{

}

int MainWindow::getMaxAreaContourId(std::vector<std::vector<cv::Point>> contours) {
    double maxArea = 0;
    int maxAreaContourId = -1;
    for (int j = 0; j < contours.size(); j++) {
        double newArea = cv::contourArea(contours.at(j));
        if (newArea > maxArea) {
            maxArea = newArea;
            maxAreaContourId = j;
        }
    }
    return maxAreaContourId;
}

QStringList MainWindow::getModelNames(const std::string &filename)
{
    std::ifstream my_file{filename};
    if (!my_file.is_open()){
        std::cerr << "Cannot open classe name file!";
        throw std::exception();
    }
    std::string model_name;
    std::vector<std::string> modelNames;
    while (my_file >> model_name)
        modelNames.push_back(model_name);
    QStringList result;
    for (std::string& model : modelNames){
        result.append(QString::fromStdString(model));
    }
    return result;
}

void MainWindow::displayImagesTimer()
{
    Pylon::CGrabResultPtr grabResultPtr;
    cv::Mat opencvImage;
    Pylon::CPylonImage pylonImage;
    if(camera.IsGrabbing()){
        camera.RetrieveResult(5, grabResultPtr, Pylon::TimeoutHandling_ThrowException);
        if (grabResultPtr->GrabSucceeded()){
            converter.Convert(pylonImage, grabResultPtr);
            opencvImage = cv::Mat(grabResultPtr->GetHeight(), grabResultPtr->GetWidth(), CV_8UC3, (uint8_t *)pylonImage.GetBuffer());
            const QImage streamFrame(opencvImage.data,opencvImage.cols, opencvImage.rows, QImage::Format_BGR888);
            const cv::Mat subtractedFrame{subtractShoeFromBackground(opencvImage, threshold)};
            const QImage subtractFrame(subtractedFrame.data, subtractedFrame.cols, subtractedFrame.rows, QImage::Format_BGR888);
            ui->streamLabel->setPixmap(QPixmap::fromImage(streamFrame.scaledToWidth(880)));
            ui->subObjectLabel->setPixmap(QPixmap::fromImage(subtractFrame.scaledToWidth(880)));
        }
        grabResultPtr.Release();
    }
}

cv::Mat MainWindow::subtractShoeFromBackground(const cv::Mat &img, const int thresh){
    // Convert image to grayscale
        cv::Mat gray_img;
        cv::cvtColor(img, gray_img, cv::COLOR_BGR2GRAY);
        // Blurred the image
        cv::blur(gray_img, gray_img, cv::Size(3, 3));
        // Apply canny on image
        cv::Mat canny_img;
        cv::Canny(gray_img, canny_img, thresh, thresh * 2);
        // Dilate canny image to have continuous edges
        cv::Mat kernel {cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5))};
        cv::Mat dilated;
        cv::dilate(canny_img, dilated, kernel);
        // Get contours of dilated image
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(dilated, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
        // Get contour with max area and save it in vector
        std::vector<std::vector<cv::Point>> best_contours {contours.at(getMaxAreaContourId(contours))};
        // Create shoe's mask with contour
        cv::Mat drawing(gray_img.rows, gray_img.cols, CV_8U);
        cv::drawContours(drawing, best_contours, 0, cv::Scalar(255, 255, 255), -1);
        // Get only shoe from image
        cv::Mat subtracted_shoe;
        cv::bitwise_and(img, img, subtracted_shoe, drawing);

        return subtracted_shoe;
}


void MainWindow::on_seuilSpinBox_valueChanged(int newThreshold)
{
    threshold = newThreshold;
}


void MainWindow::on_captureButton_clicked()
{
    int prevValue = ui->viewSpinBox->value();
    ui->viewSpinBox->setValue( prevValue == 8 ? 1 : prevValue + 1 );
}

