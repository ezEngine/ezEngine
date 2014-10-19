#pragma once

#include <Foundation/Basics.h>
#include <qlistwidget.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezTestObjectCreatorWidget : public QListWidget
{
public:
  Q_OBJECT

public:
  ezTestObjectCreatorWidget(const ezDocumentObjectManagerBase* pManager, QWidget* parent);

  virtual QMimeData * mimeData ( const QList<QListWidgetItem *> items ) const override;
  virtual QStringList mimeTypes () const override;

public slots:

private slots:

private:
  const ezDocumentObjectManagerBase* m_pManager;

};


