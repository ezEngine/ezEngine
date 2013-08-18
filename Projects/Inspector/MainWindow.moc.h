#pragma once

#include <Foundation/Basics.h>
#include <QMainWindow>
#include <Projects/Inspector/ui_mainwindow.h>

class ezMainWindow : public QMainWindow, public Ui_MainWindow
{
public:
  Q_OBJECT

public:
  ezMainWindow();

  void paintEvent(QPaintEvent* event) EZ_OVERRIDE;

private slots:
  virtual void on_ButtonClearLog_clicked();
  virtual void on_ButtonConnect_clicked();

private:

};


