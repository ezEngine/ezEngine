#include <PCH.h>
#include <EditorFramework/PropertyGrid/QtAssetLineEdit.moc.h>
#include <qevent.h>
#include <QMimeData>

ezQtAssetLineEdit::ezQtAssetLineEdit(QWidget* parent /*= nullptr*/) : QLineEdit(parent)
{

}

void ezQtAssetLineEdit::dragMoveEvent(QDragMoveEvent *e)
{
  if (e->mimeData()->hasUrls())
  {
    e->acceptProposedAction();
  }
  else
  {
    QLineEdit::dragMoveEvent(e);
  }
}

void ezQtAssetLineEdit::dragEnterEvent(QDragEnterEvent * e)
{
  if (e->mimeData()->hasUrls())
  {
    e->acceptProposedAction();
  }
  else
  {
    QLineEdit::dragEnterEvent(e);
  }
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
    setText(str);
    return;
  }


  if (e->mimeData()->hasText())
  {
    QString str = e->mimeData()->text();
    setText(str);
    return;
  }
}
