#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include "MainGraphicsView.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void ActionClicked(QAction* action);
    void SelectMask();
    void turnMask(bool on);
private:
    QString tiledImagePath;
    Ui::MainWindow *ui;
    MainGraphicsView *mainGraphicsView;
};

#endif // MAINWINDOW_H
