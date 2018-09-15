/*
 * MIT License

Copyright (c) 2018 Gabriele Salvato

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "mainwindow.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStringList>
#ifdef Q_OS_ANDROID
    #include <QtAndroid>
#endif
#ifndef Q_OS_ANDROID
    #include <QSettings>
#endif

typedef QMap<QString, QString> AndroidDirCache;


MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    // Pointers
    , pInfoSource(Q_NULLPTR)
    , pLogFile(Q_NULLPTR)
    // Variables
    , sLogFileName("percorso.txt")
{
    // Instantiate a Position Source
    pInfoSource = QGeoPositionInfoSource::createDefaultSource(this);

    // Do we have a Position Source available ?
    if(pInfoSource == Q_NULLPTR) {
        QMessageBox::critical(Q_NULLPTR, Q_FUNC_INFO,
                                 QString("No Geo Info Source Available"));
        exit(EXIT_FAILURE);
    }

#ifdef Q_OS_ANDROID
    // Do we have permissions to write Log File ?
    if(!check_permission()) {
        QMessageBox::critical(Q_NULLPTR, Q_FUNC_INFO,
                                 QString("No Permission to Write Log File"));
        exit(EXIT_FAILURE);
    }
    sLogDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if(!sLogDir.endsWith(QString("/"))) sLogDir+= QString("/");
    sLogDir += QString("gpsData/");
#else
    pSettings = new QSettings("GabrieleSalvato", "GpsTest");

    // Logged messages will be written in the following folder
    sLogDir = pSettings->value("directories/log",
                               QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).toString();
    // Give a chance to select a different directory
    selectLogDir();
    // Save the choice
    pSettings->setValue("directories/log", sLogDir);
#endif
    if(!sLogDir.endsWith(QString("/"))) sLogDir+= QString("/");
//    QMessageBox::information(Q_NULLPTR, Q_FUNC_INFO,
//                             QString("Writing on %1: ")
//                             .arg(sLogDir));
    QDir dir(sLogDir);
    if(!dir.exists()) {
        if(!dir.mkpath(sLogDir)) {
            QMessageBox::critical(Q_NULLPTR, Q_FUNC_INFO,
                                     QString("Unable to create %1")
                                  .arg(sLogDir));
            exit(EXIT_FAILURE);
        }
    }
    // Rotate 5 previous logs, removing the oldest, to avoid data loss
    sLogFileName = sLogDir + sLogFileName;
    QFileInfo checkFile(sLogFileName);
    if(checkFile.exists() && checkFile.isFile()) {
        QDir renamed;
        renamed.remove(sLogFileName+QString(".4.txt"));
        for(int i=3; i>=0; i--) {
            renamed.rename(sLogFileName+QString(".%1.txt").arg(i),
                           sLogFileName+QString(".%1.txt").arg(i+1));
        }
        renamed.rename(sLogFileName,
                       sLogFileName+QString(".0.txt"));
    }
    // Open the new log file
    pLogFile = new QFile(sLogFileName);
    if (!pLogFile->open(QIODevice::WriteOnly)) {
        QMessageBox::information(Q_NULLPTR, Q_FUNC_INFO,
                                 QString("Impossibile aprire il file %1: %2.")
                                 .arg(sLogFileName).arg(pLogFile->errorString()));
        delete pLogFile;
        pLogFile = Q_NULLPTR;
        exit(EXIT_FAILURE);
    }

    pLogFile->write("Date, latitude, longitude, position error, altitude\n");

    // Initialize the previous point as non existing
    previousGeoCoordinate.setLatitude(0.0);
    previousGeoCoordinate.setLongitude(0.0);
    previousGeoCoordinate.setAltitude(0.0);

    int minimumInterval = pInfoSource->minimumUpdateInterval();
    pInfoSource->setUpdateInterval(qMax(3000, minimumInterval));

    createPanelElements();
    setLayout(createPanel());

    // Set the event handlers
    connect(pInfoSource, SIGNAL(positionUpdated(QGeoPositionInfo)),
            this, SLOT(postionUpdated(QGeoPositionInfo)));
    connect(pInfoSource, SIGNAL(updateTimeout()),
            this, SLOT(updateTimeout()));
    connect(pInfoSource, SIGNAL(error(QGeoPositionInfoSource::Error)),
            this, SLOT(error(QGeoPositionInfoSource::Error)));
    // UI events
    connect(pStartButton, SIGNAL(clicked()),
            this, SLOT(onStartButtonPushed()));
    connect(pPauseButton, SIGNAL(clicked()),
            this, SLOT(onPauseButtonPushed()));
    connect(pExitButton, SIGNAL(clicked()),
            this, SLOT(onExitButtonPushed()));
}


#ifdef Q_OS_ANDROID

// Taken from https://bugreports.qt.io/browse/QTBUG-50759
bool
MainWindow::check_permission() {
    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
    if(r == QtAndroid::PermissionResult::Denied) {
        QtAndroid::requestPermissionsSync( QStringList() <<
                                           "android.permission.WRITE_EXTERNAL_STORAGE" <<
                                           "android.permission.READ_EXTERNAL_STORAGE" <<
                                           "android.permission.WRITE_SETTINGS");
        r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
        if(r == QtAndroid::PermissionResult::Denied) {
            QMessageBox::critical(Q_NULLPTR, Q_FUNC_INFO,
                                     QString("No Permission to WRITE_EXTERNAL_STORAGE"));
            return false;
        }
        r = QtAndroid::checkPermission("android.permission.READ_EXTERNAL_STORAGE");
        if(r == QtAndroid::PermissionResult::Denied) {
            QMessageBox::critical(Q_NULLPTR, Q_FUNC_INFO,
                                     QString("No Permission to READ_EXTERNAL_STORAGE"));
            return false;
        }
        r = QtAndroid::checkPermission("android.permission.WRITE_SETTINGS");
        if(r == QtAndroid::PermissionResult::Denied) {
            QMessageBox::critical(Q_NULLPTR, Q_FUNC_INFO,
                                     QString("No Permission to WRITE_SETTINGS"));
            return false;
        }
    }
    return true;
}

#else

void
MainWindow::selectLogDir() {
    QFileDialog* pGetDirDlg;
    QDir logDir = QDir(sLogDir);
    if(logDir.exists()) {
        pGetDirDlg = new QFileDialog(this, tr("Log Dir"),
                                     sLogDir);
    }
    else {
        pGetDirDlg = new QFileDialog(this, tr("Log Dir"),
                                     QStandardPaths::displayName(QStandardPaths::DocumentsLocation));
    }
    pGetDirDlg->setOptions(QFileDialog::ShowDirsOnly);
    pGetDirDlg->setFileMode(QFileDialog::Directory);
    pGetDirDlg->setViewMode(QFileDialog::List);
#ifdef Q_OS_ANDROID
    pGetDirDlg->setWindowFlags(Qt::Window);
#endif
    if(pGetDirDlg->exec() == QDialog::Accepted)
        sLogDir = pGetDirDlg->directory().absolutePath();
    pGetDirDlg->deleteLater();
    if(!sLogDir.endsWith(QString("/"))) sLogDir+= QString("/");
    logDir.setPath(sLogDir);
}

#endif


MainWindow::~MainWindow() {
    if(pLogFile) {
        if(pLogFile->isOpen()) {
            pLogFile->close();
        }
        delete pLogFile;
    }
    delete pInfoSource;

    delete pAltitudeLabel;
    delete pLatitudeLabel;
    delete pLongitudeLabel;
    delete pDateTimeLabel;
    delete pAltitudeEdit;
    delete pLatitudeEdit;
    delete pLongitudeEdit;
    delete pDateTimeEdit;
    delete pStartButton;
    delete pPauseButton;
    delete pExitButton;
    delete pStatusEdit;
}


QGridLayout*
MainWindow::createPanel() {
    auto *layout = new QGridLayout();
    layout->addWidget(pAltitudeLabel,  0, 0, 1, 1);
    layout->addWidget(pLatitudeLabel,  1, 0, 1, 1);
    layout->addWidget(pLongitudeLabel, 2, 0, 1, 1);

    layout->addWidget(pAltitudeEdit,  0, 1, 1, 2);
    layout->addWidget(pLatitudeEdit,  1, 1, 1, 2);
    layout->addWidget(pLongitudeEdit, 2, 1, 1, 2);

    layout->addWidget(pAltitudeErrorLabel,  0, 3, 1, 1);
    layout->addWidget(pLatitudeErrorLabel,  1, 3, 1, 1);
    layout->addWidget(pLongitudeErrorLabel, 2, 3, 1, 1);

    layout->addWidget(pAltitudeErrorEdit,  0, 4, 1, 1);
    layout->addWidget(pLatitudeErrorEdit,  1, 4, 1, 1);
    layout->addWidget(pLongitudeErrorEdit, 2, 4, 1, 1);

    layout->addWidget(pDateTimeLabel,  3, 0, 1, 1);
    layout->addWidget(pDateTimeEdit,   3, 1, 1, 4);


    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(pStartButton);
    buttonLayout->addWidget(pPauseButton);
    buttonLayout->addWidget(pExitButton);

    layout->addLayout(buttonLayout, 4, 0, 1, 5);

    layout->addWidget(pStatusEdit,  5, 0, 1, 5);

    return layout;
}


void
MainWindow::createPanelElements() {
    pAltitudeLabel       = new QLabel(tr("Altitudine"));
    pAltitudeErrorLabel  = new QLabel(tr("+/- [m]"));
    pLatitudeLabel       = new QLabel(tr("Latitudine"));
    pLatitudeErrorLabel  = new QLabel(tr("+/- [m]"));
    pLongitudeLabel      = new QLabel(tr("Longitudine"));
    pLongitudeErrorLabel = new QLabel(tr("+/- [m]"));

    pDateTimeLabel       = new QLabel(tr("Data"));

    pAltitudeEdit       = new QLineEdit("------------");
    pAltitudeErrorEdit  = new QLineEdit("------------");
    pLatitudeEdit       = new QLineEdit("------------");
    pLatitudeErrorEdit  = new QLineEdit("------------");
    pLongitudeEdit      = new QLineEdit("------------");
    pLongitudeErrorEdit = new QLineEdit("------------");

    pDateTimeEdit       = new QLineEdit("------------");

    pAltitudeEdit->setReadOnly(true);
    pAltitudeErrorEdit->setReadOnly(true);
    pLatitudeEdit->setReadOnly(true);
    pLatitudeErrorEdit->setReadOnly(true);
    pLongitudeEdit->setReadOnly(true);
    pLongitudeErrorEdit->setReadOnly(true);

    pDateTimeEdit->setReadOnly(true);

    pStartButton   = new QPushButton("Start");
    pPauseButton   = new QPushButton("Pause");
    pExitButton    = new QPushButton("Exit");

    pStartButton->setEnabled(true);
    pPauseButton->setDisabled(true);

    pStatusEdit = new QLineEdit(tr(""));
    pStatusEdit->setReadOnly(true);
}


void
MainWindow::postionUpdated(QGeoPositionInfo info) {
    pStatusEdit->setText("...Running...");
    QGeoCoordinate newGeoCoordinate = info.coordinate();
    // To reduce the amount of written information we log only positions
    // that differ from the previous more than 1.0 meter
    if(newGeoCoordinate.distanceTo(previousGeoCoordinate) > 1.0) {
        previousGeoCoordinate = newGeoCoordinate;
        QDateTime dateTime = info.timestamp();
        double altitude      = -9999.99;
        double altitudeError = -1.0;
        if(newGeoCoordinate.type() == QGeoCoordinate::Coordinate3D) {
            altitude = newGeoCoordinate.altitude();
            if(info.hasAttribute(QGeoPositionInfo::VerticalAccuracy))
                altitudeError = info.attribute(QGeoPositionInfo::VerticalAccuracy);
        }
        double latitude = newGeoCoordinate.latitude();
        double positionError = -1.0;
        if(info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
           positionError = info.attribute(QGeoPositionInfo::HorizontalAccuracy);
        double longitude = newGeoCoordinate.longitude();
        if(pLogFile) {
            if(pLogFile->isOpen()) {
                pLogFile->write(dateTime.toString().toUtf8().data());
                pLogFile->write(",");
                pLogFile->write(QString("%1").arg(latitude, 12, 'g', 10).toUtf8().data());
                pLogFile->write(",");
                pLogFile->write(QString("%1").arg(longitude, 12, 'g', 10).toUtf8().data());
                pLogFile->write(",");
                pLogFile->write(QString("%1").arg(positionError, 6, 'g', 10).toUtf8().data());
                pLogFile->write(",");
                pLogFile->write(QString("%1").arg(altitude, 12, 'g', 10).toUtf8().data());
                pLogFile->write("\n");
                pLogFile->flush();
            }
        }
        // Updates UI
        pAltitudeEdit->setText(QString("%1").arg(altitude,   12, 'g', 10));
        pAltitudeErrorEdit->setText(QString("%1").arg(altitudeError,   6, 'g', 10));
        pLatitudeEdit->setText(QString("%1").arg(latitude,   6, 'g', 10));
        pLatitudeErrorEdit->setText(QString("%1").arg(positionError,   6, 'g', 10));
        pLongitudeEdit->setText(QString("%1").arg(longitude, 12, 'g', 10));
        pLongitudeErrorEdit->setText(QString("%1").arg(positionError, 6, 'g', 10));
        pDateTimeEdit->setText(dateTime.toString());
    }
}


void
MainWindow::updateTimeout() {
    pStatusEdit->setText(tr("Update Timeout"));
}


void
MainWindow::error(QGeoPositionInfoSource::Error positioningError) {
    QString sError;
    if(positioningError == QGeoPositionInfoSource::AccessError)
        sError = QString("The application lack the required privileges");
    else if(positioningError == QGeoPositionInfoSource::ClosedError)
        sError = QString("The remote positioning backend closed the connection");
    else if(positioningError == QGeoPositionInfoSource::UnknownSourceError)
        sError = QString("An unidentified error occurred");
    pStatusEdit->setText(sError);
    emit errorFound(int(positioningError));
}


void
MainWindow::onStartButtonPushed() {
    // Start updating
    pStatusEdit->setText("...Starting...");
    pStartButton->setDisabled(true);
    pPauseButton->setEnabled(true);
    pInfoSource->startUpdates();
}


void
MainWindow::onPauseButtonPushed() {
    pStatusEdit->setText("...Paused...");
    pPauseButton->setDisabled(true);
    pStartButton->setEnabled(true);
    pInfoSource->stopUpdates();
}


void
MainWindow::onExitButtonPushed() {
    exit(EXIT_SUCCESS);
}
