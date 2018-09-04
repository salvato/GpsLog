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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QGeoPositionInfoSource>
#include <QFile>


QT_FORWARD_DECLARE_CLASS(QGridLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
#ifndef Q_OS_ANDROID
QT_FORWARD_DECLARE_CLASS(QSettings)
#endif

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
#ifndef Q_OS_ANDROID
    void         selectLogDir();
#endif
    void         createPanelElements();
    void         buildLayout();
    QGridLayout* createPanel();
#ifdef Q_OS_ANDROID
    bool         check_permission();
#endif

signals:
    void errorFound(int code);
    void entryCountChanged();

public slots:
    void error(QGeoPositionInfoSource::Error positioningError);
    void postionUpdated(QGeoPositionInfo info);
    void updateTimeout();
    void onStartButtonPushed();
    void onPauseButtonPushed();
    void onExitButtonPushed();

private:
    QGeoPositionInfoSource* pInfoSource;
    QGeoCoordinate          previousGeoCoordinate;
#ifndef Q_OS_ANDROID
    QSettings*              pSettings;
#endif
    QFile*                  pLogFile;
    QLabel*                 pAltitudeLabel;
    QLabel*                 pLatitudeLabel;
    QLabel*                 pLongitudeLabel;
    QLabel*                 pDateTimeLabel;
    QLineEdit*              pStatusEdit;
    QLineEdit*              pAltitudeEdit;
    QLineEdit*              pLatitudeEdit;
    QLineEdit*              pLongitudeEdit;
    QLineEdit*              pDateTimeEdit;
    QPushButton*            pStartButton;
    QPushButton*            pPauseButton;
    QPushButton*            pExitButton;

private:
    QString sLogDir;
    QString sLogFileName;
};

#endif // MAINWINDOW_H
