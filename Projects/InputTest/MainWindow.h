#pragma once

#include <Foundation/Basics.h>
#include <QMainWindow>
#include <Projects/InputTest/ui_mainwindow.h>
#include <InputTest/InputListModel.moc.h>

class ezMainWindow : public QMainWindow, public Ui_MainWindow
{
public:
  Q_OBJECT

public:
  ezMainWindow();

  virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result) EZ_OVERRIDE;

  void paintEvent(QPaintEvent* event) EZ_OVERRIDE;

private slots:
  void on_CheckClipCursor_clicked();
  void on_CheckShowCursor_clicked();
  void on_ComboMapController1_currentIndexChanged(int index);
  void on_ComboMapController2_currentIndexChanged(int index);
  void on_ComboMapController3_currentIndexChanged(int index);
  void on_ComboMapController4_currentIndexChanged(int index);

private:
  InputListModel m_ListModel;
};


