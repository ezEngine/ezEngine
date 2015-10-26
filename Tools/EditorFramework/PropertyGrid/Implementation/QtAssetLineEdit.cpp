#include <PCH.h>
#include <EditorFramework/PropertyGrid/QtAssetLineEdit.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <qevent.h>
#include <QMimeData>
#include "EditorApp/EditorApp.moc.h"

ezQtAssetLineEdit::ezQtAssetLineEdit(QWidget* parent /*= nullptr*/) : QLineEdit(parent)
{

}

void ezQtAssetLineEdit::dragMoveEvent(QDragMoveEvent *e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidAssetType(str.toUtf8().data()))
      e->acceptProposedAction();
   
    return;
  }

  QLineEdit::dragMoveEvent(e);
}

void ezQtAssetLineEdit::dragEnterEvent(QDragEnterEvent * e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidAssetType(str.toUtf8().data()))
      e->acceptProposedAction();
    
    return;
  }

  QLineEdit::dragEnterEvent(e);
}

void ezQtAssetLineEdit::dropEvent(QDropEvent* e)
{
  if (e->source() == this)
  {
    QLineEdit::dropEvent(e);
    return;
  }

  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    ezString sPath = str.toUtf8().data();
    if (ezQtEditorApp::GetInstance()->MakePathDataDirectoryRelative(sPath))
    {
      setText(QString::fromUtf8(sPath.GetData()));
    }
    else
      setText(QString());

    return;
  }


  if (e->mimeData()->hasText())
  {
    QString str = e->mimeData()->text();

    ezString sPath = str.toUtf8().data();
    if (ezQtEditorApp::GetInstance()->MakePathDataDirectoryRelative(sPath))
    {
      setText(QString::fromUtf8(sPath.GetData()));
    }
    else
      setText(QString());

    return;
  }
}
