#pragma once

#include <Foundation/Basics.h>
#include <qlistwidget.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezTextObjectCreatorWidget : public QListWidget
{
public:
  Q_OBJECT

public:
  ezTextObjectCreatorWidget(ezDocumentObjectManagerBase* pManager, QWidget* parent);

  virtual QMimeData * mimeData ( const QList<QListWidgetItem *> items ) const override;
  virtual QStringList mimeTypes () const override;

public slots:

private slots:

private:
  ezDocumentObjectManagerBase* m_pManager;

};


