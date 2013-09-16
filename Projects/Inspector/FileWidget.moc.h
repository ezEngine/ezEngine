#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_FileWidget.h>
#include <qgraphicsscene.h>

class ezFileWidget : public QDockWidget, public Ui_FileWidget
{
public:
  Q_OBJECT

public:
  ezFileWidget(QWidget* parent = 0);

  static ezFileWidget* s_pWidget;

private slots:

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  QGraphicsScene m_Scene;
};


