#include <PCH.h>
#include <EditorPluginTest/Widgets/TestObjectCreator.moc.h>
#include <EditorFramework/GUI/QtHelpers.h>
#include <QMimeData>

ezTestObjectCreatorWidget::ezTestObjectCreatorWidget(const ezDocumentObjectManagerBase* pManager, QWidget* parent) : QListWidget(parent)
{
  m_pManager = pManager;

  ezHybridArray<ezReflectedTypeHandle, 32> Types;
  m_pManager->GetCreateableTypes(Types);

  ezQtBlockSignals b(this);

  for (ezUInt32 i = 0; i < Types.GetCount(); ++i)
  {
    addItem(QString::fromUtf8(Types[i].GetType()->GetTypeName().GetData()));
  }
  setDragEnabled(true);

}

QMimeData* ezTestObjectCreatorWidget::mimeData(const QList<QListWidgetItem *> items) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  stream << items[0]->text();

  mimeData->setData("application/ezEditor.ObjectCreator", encodedData);
  return mimeData;
}

QStringList ezTestObjectCreatorWidget::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.ObjectCreator";
  return types;
}

