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
    // camera.StartGrabbing(Pylon::GrabStrategy_LatestImageOnly);
    converter.OutputPixelFormat = Pylon::PixelType_BGR8packed;
    converter.OutputBitAlignment = Pylon::OutputBitAlignment_MsbAligned;

    ui->stateComboBox->addItems(stateList);
    modelList = getModelNames(classFile);
    // modelList.sort();
    ui->modelComboBox->addItems(modelList);
    ui->modelComboBox->model()->sort(0);
    ui->marqueComboBox->addItems(marqueList);
    ui->colorLineEdit->setText("Unique");

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

int MainWindow::getMaxAreaContourId(std::vector<std::vector<cv::Point>> &contours) {
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
        std::cerr << "Cannot open classe name file!\n";
        throw std::exception();
    }
    std::string model_name;
    std::vector<std::string> modelNames;
    while (!my_file.eof()){
        if (std::getline(my_file, model_name))
            modelNames.push_back(model_name);
    }
    my_file.close();
    QStringList result;
    for (std::string& model : modelNames){
        result.append(QString::fromStdString(model));
    }
    return result;
}

void MainWindow::writeNewModelName(const QString &modelName)
{
    QFile my_file{QString::fromStdString(classFile)};
    if (!my_file.open(QIODevice::Append)){
        std::cerr << "Cannot open classe name file!\n";
        throw std::exception();
    }
    QTextStream out(&my_file);
    out << modelName;
    my_file.close();
}

void MainWindow::writeInfoModel(const QString &modelName, const QString &color, const QString &sku)
{
    QFile my_file{QString::fromStdString(infoFile)};
    if (!my_file.open(QIODevice::Append)){
        std::cerr << "Cannot open models info file!\n";
        throw std::exception();
    }
    QTextStream out(&my_file);
    out << modelName + "-" + color + "-" + sku + "\n";
    my_file.close();
}

void MainWindow::displayImagesTimer()
{
    Pylon::CGrabResultPtr grabResultPtr;
    Pylon::CPylonImage pylonImage;
    if(camera.IsGrabbing() && !pause){
        camera.RetrieveResult(5, grabResultPtr, Pylon::TimeoutHandling_ThrowException);
        if (grabResultPtr->GrabSucceeded()){
            converter.Convert(pylonImage, grabResultPtr);
            realShoeImage = cv::Mat(grabResultPtr->GetHeight(), grabResultPtr->GetWidth(), CV_8UC3, (uint8_t *)pylonImage.GetBuffer());
            QImage streamFrame(realShoeImage.data, realShoeImage.cols, realShoeImage.rows, QImage::Format_BGR888);
            subtractedShoeImage = subtractShoeFromBackground(realShoeImage, threshold);
            QImage subtractFrame(subtractedShoeImage.data, subtractedShoeImage.cols, subtractedShoeImage.rows, QImage::Format_BGR888);
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
        // Threshold mask to remove noise
        cv::Mat threshImg;
        cv::threshold(drawing, threshImg, 254, 255, cv::THRESH_BINARY );
        // Get shoe bounding box
        boundingBox = cv::boundingRect(threshImg);
        // Get only shoe from image
        cv::Mat subtracted_shoe;
        cv::bitwise_and(img, img, subtracted_shoe, threshImg);
        return subtracted_shoe;
}

void MainWindow::on_seuilSpinBox_valueChanged(int newThreshold)
{
    threshold = newThreshold;
}


void MainWindow::on_captureButton_clicked()
{
    // saveImage();
    QMessageBox msgBox;
    msgBox.setText("Image acquise.");
    msgBox.setInformativeText("Voulez vous la sauvegarder?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    pause = true;
    int ret = msgBox.exec();
    if (ret == QMessageBox::Save){
        saveResultImage();
        int prevValue = ui->viewSpinBox->value();
        ui->viewSpinBox->setValue( prevValue == 8 ? 1 : prevValue + 1 );
    }
    pause = false;
}

void MainWindow::saveResultImage()
{
    QString real_filename;
    QString sub_filename;
    auto timestamp {std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
    QString strTimestamp = QString::number(timestamp);
    const QString model_name = ui->modelComboBox->currentText().trimmed().toUpper();
    const QString sku = ui->skuLineEdit->text().trimmed();
    const QString state = (ui->stateComboBox->currentText() == "Neuve" ? "1" : "0");
    const QString color = ui->colorLineEdit->text().trimmed().toLower();
    const QString view = ui->viewSpinBox->text();
    const QString marque = (ui->marqueComboBox->currentText().toLower() == "decathlon" ? " D":" E");

    real_filename = "real_" + model_name.toLower() + "_" + color + "_" + view + "_" + state + "_" + strTimestamp + ".png";
    sub_filename = "sub_" + model_name.toLower() + "_" + color + "_" + view + "_" + state + "_" + strTimestamp + ".png";

    if (ui->modelComboBox->findText(model_name) == -1){
        bool dirCreated = imageFolder.mkdir(model_name + marque);
        if (dirCreated){
            // QString model_name_with_return = model_name + "\n"
            ui->modelComboBox->addItem(model_name);
            ui->modelComboBox->model()->sort(0);
            // ui->modelComboBox->findText(model_name);
            writeNewModelName(model_name + "\n");
            writeInfoModel(model_name, color, sku);
        }
        else{
            throw std::exception();
        }
    }

    const QDir modelFolder {imageFolder.path() + "/" + model_name + marque};

//    cv::imwrite(modelFolder.path().toStdString() + "/" + real_filename.toStdString(), realShoeImage,
//                std::vector<int>{cv::IMWRITE_PNG_COMPRESSION, 0});
    cv::imwrite(modelFolder.path().toStdString() + "/" + sub_filename.toStdString(), subtractedShoeImage(boundingBox),
                std::vector<int>{cv::IMWRITE_PNG_COMPRESSION, 0});
}


void MainWindow::on_modelComboBox_currentTextChanged(const QString &arg1)
{
    ui->viewSpinBox->setValue(1);
}

