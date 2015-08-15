#include <PCH.h>
#include <EditorFramework/GUI/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <QStyleOptionToolButton>
#include <QStyle>
#include <QPainter>
#include <QMouseEvent>
#include <QScrollArea>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

const ezInt32 s_GroupBoxHeight = 17;
ezCollapsibleGroupBox::ezCollapsibleGroupBox(QWidget* pParent, bool bShowElementButtons) : QWidget(pParent)
{
  setupUi(this);

  if (!bShowElementButtons)
  {
    MoveUp->setVisible(false);
    MoveDown->setVisible(false);
    Delete->setVisible(false);
  }

  Icon->installEventFilter(this);
  Caption->installEventFilter(this);
}

void ezCollapsibleGroupBox::setTitle(QString sTitle)
{
  Caption->setText(sTitle);
}

bool ezCollapsibleGroupBox::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Type::MouseButtonPress || event->type() == QEvent::Type::MouseButtonDblClick)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

    if (mouseEvent->button() == Qt::MouseButton::LeftButton)
    {
      QtScopedUpdatesDisabled sud(this);

      Content->setVisible(!Content->isVisible());

      QWidget* pCur = this;
      while (pCur != nullptr && qobject_cast<QScrollArea*>(pCur) == nullptr)
      {
        pCur->updateGeometry();
        pCur = pCur->parentWidget();
      }

      Icon->setPixmap(QPixmap(QLatin1String(Content->isVisible() ? ":/GuiFoundation/Icons/GroupOpen.png" : ":/GuiFoundation/Icons/GroupClosed.png")));
      return true;
    }
  }

  return false;
}

ezElementGroupBox::ezElementGroupBox(QWidget* pParent) : ezCollapsibleGroupBox(pParent, true)
{

}

void ezElementGroupBox::Move(ezInt32 iMove)
{
  ezCommandHistory* history = m_Items[0].m_pObject->GetDocumentObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezMoveObjectCommand cmd;

  bool bDidAny = false;

  ezStatus res(EZ_SUCCESS);
  for (auto& item : m_Items)
  {
    ezInt32 iCurIndex = item.m_pObject->GetPropertyIndex().ConvertTo<ezInt32>() + iMove;
    if (iCurIndex < 0 || iCurIndex > item.m_pObject->GetParentAccessor().GetCount(item.m_pObject->GetParentProperty()))
      continue;

    if (!bDidAny)
    {
      setParent(nullptr);
      setVisible(false);
    }

    bDidAny = true;

    cmd.m_NewParent = item.m_pObject->GetParent()->GetGuid();
    cmd.m_Object = item.m_pObject->GetGuid();
    cmd.m_sParentProperty = item.m_pObject->GetParentProperty();
    cmd.m_Index = iCurIndex;

    res = history->AddCommand(cmd);
    if (res.m_Result.Failed())
      break;
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezUIServices::GetInstance()->MessageBoxStatus(res, "Moving sub-element failed.");

  if (bDidAny)
    deleteLater();
}

void ezElementGroupBox::on_MoveUp_clicked()
{
  Move(-1);
}

void ezElementGroupBox::on_MoveDown_clicked()
{
  Move(+2);
}

void ezElementGroupBox::on_Delete_clicked()
{
  setParent(nullptr);
  setVisible(false);

  ezCommandHistory* history = m_Items[0].m_pObject->GetDocumentObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezRemoveObjectCommand cmd;

  ezStatus res;
  for (auto& item : m_Items)
  {
    cmd.m_Object = item.m_pObject->GetGuid();
    res = history->AddCommand(cmd);
    if (res.m_Result.Failed())
      break;
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezUIServices::GetInstance()->MessageBoxStatus(res, "Removing sub-element from the property failed.");

  deleteLater();
}

