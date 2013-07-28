#pragma once

#include <QTableView>

class InputTableView : public QTableView
{
  Q_OBJECT
 
public:
  InputTableView(QWidget* parent = 0);

  virtual void keyPressEvent(QKeyEvent* event) override;
};

