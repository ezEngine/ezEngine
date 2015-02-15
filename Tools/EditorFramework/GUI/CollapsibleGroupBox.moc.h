#pragma once

#include <Plugin.h>
#include <QGroupBox>
#include <QLayout>

class EZ_EDITORFRAMEWORK_DLL ezCollapsibleGroupBox : public QGroupBox
{
  Q_OBJECT

public:
  ezCollapsibleGroupBox(QWidget* pParent);
  ~ezCollapsibleGroupBox();

protected:
  void paintEvent(QPaintEvent *);

private slots:
  void on_toggled(bool bToggled);
};


