#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/IO/MemoryStream.h>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

////////////////////////////////////////////////////////////////////////
// ezEditActions
////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezEditActions::s_hEditCategory;
ezActionDescriptorHandle ezEditActions::s_hCopy;
ezActionDescriptorHandle ezEditActions::s_hPaste;
ezActionDescriptorHandle ezEditActions::s_hDelete;

void ezEditActions::RegisterActions()
{
  s_hEditCategory = EZ_REGISTER_CATEGORY("EditCategory");
  s_hCopy = EZ_REGISTER_ACTION_1("Copy", "Copy", ezActionScope::Document, "Document", "Ctrl+C", ezEditAction, ezEditAction::ButtonType::Copy);
  s_hPaste = EZ_REGISTER_ACTION_1("Paste", "Paste", ezActionScope::Document, "Document", "Ctrl+V", ezEditAction, ezEditAction::ButtonType::Paste);
  s_hDelete = EZ_REGISTER_ACTION_1("Delete", "Delete", ezActionScope::Document, "Document", "", ezEditAction, ezEditAction::ButtonType::Delete);
}

void ezEditActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hEditCategory);
  ezActionManager::UnregisterAction(s_hCopy);
  ezActionManager::UnregisterAction(s_hPaste);
  ezActionManager::UnregisterAction(s_hDelete);
}

void ezEditActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the edit actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/EditCategory");

  pMap->MapAction(s_hEditCategory, szPath, 3.5f);

  pMap->MapAction(s_hCopy, sSubPath, 1.0f);
  pMap->MapAction(s_hPaste, sSubPath, 2.0f);
  pMap->MapAction(s_hDelete, sSubPath, 3.0f);
}

////////////////////////////////////////////////////////////////////////
// ezEditAction
////////////////////////////////////////////////////////////////////////

ezEditAction::ezEditAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
  case ezEditAction::ButtonType::Copy:
    SetIconPath(":/GuiFoundation/Icons/Copy16.png");
    break;
  case ezEditAction::ButtonType::Paste:
    SetIconPath(":/GuiFoundation/Icons/Paste16.png");
    break;
  case ezEditAction::ButtonType::Delete:
    SetIconPath(":/GuiFoundation/Icons/Delete16.png");
    break;
  }

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezEditAction::SelectionEventHandler, this));

  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}

ezEditAction::~ezEditAction()
{
  if (m_Context.m_pDocument)
  {
    m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezEditAction::SelectionEventHandler, this));
  }
}

void ezEditAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
  case ezEditAction::ButtonType::Copy:
    {
      ezAbstractObjectGraph graph;
      m_Context.m_pDocument->Copy(graph);

      // Serialize to string
      ezMemoryStreamStorage streamStorage;
      ezMemoryStreamWriter memoryWriter(&streamStorage);
      ezAbstractGraphJsonSerializer::Write(memoryWriter, &graph, ezJSONWriter::WhitespaceMode::LessIndentation);
      memoryWriter.WriteBytes("\0", 1); // null terminate

      // Write to clipboard
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize());
      mimeData->setData(m_Context.m_pDocument->GetDocumentTypeDescriptor().m_sDocumentTypeName.GetData(), encodedData);
      mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
      clipboard->setMimeData(mimeData);
    }
    break;

  case ezEditAction::ButtonType::Paste:
    {
      // Check for clipboard data of the correct type.
      const ezString& sDocumentTypeName = m_Context.m_pDocument->GetDocumentTypeDescriptor().m_sDocumentTypeName;
      QClipboard* clipboard = QApplication::clipboard();
      auto mimedata = clipboard->mimeData();
      if (!mimedata->hasFormat(sDocumentTypeName.GetData()))
        return;

      // Paste at current selected object.
      ezPasteObjectsCommand cmd;
      QByteArray ba = mimedata->data(sDocumentTypeName.GetData());
      cmd.m_sJsonGraph = ba.data();

      //if (!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty())
        //cmd.m_Parent = m_Context.m_pDocument->GetSelectionManager()->GetSelection().PeekBack()->GetGuid();

      auto history = m_Context.m_pDocument->GetCommandHistory();

      history->StartTransaction();

      if (history->AddCommand(cmd).m_Result.Failed())
        history->CancelTransaction();
      else
        history->FinishTransaction();
    }
    break;

  case ezEditAction::ButtonType::Delete:
    {
      m_Context.m_pDocument->DeleteSelectedObjects();
    }
    break;
  }
}

void ezEditAction::SelectionEventHandler(const ezSelectionManager::Event& e)
{
  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}

