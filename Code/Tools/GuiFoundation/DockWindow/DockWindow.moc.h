#pragma once

#include <GuiFoundation/Basics.h>
#include <QDockWidget>

class EZ_GUIFOUNDATION_DLL ezDockWindow : public QDockWidget
{
public:
  Q_OBJECT

public:
  ezDockWindow(QWidget* parent);
  ~ezDockWindow();


};


