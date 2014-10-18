#pragma once

#include <Foundation/Basics.h>
#include <QMainWindow>
#include <Tools/EditorPluginTest/ui_TestWindow.h>

class ezTestWindow : public QMainWindow, public Ui_TestWindow
{
public:
  Q_OBJECT

public:
  ezTestWindow(QWidget* parent);
  ~ezTestWindow();

public slots:

private slots:

private:
};


