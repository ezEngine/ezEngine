#include <GuiFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// ezEditActions
////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezEditActions::s_hEditCategory;
ezActionDescriptorHandle ezEditActions::s_hCopy;
ezActionDescriptorHandle ezEditActions::s_hPaste;
ezActionDescriptorHandle ezEditActions::s_hPasteAsChild;
ezActionDescriptorHandle ezEditActions::s_hDelete;

void ezEditActions::RegisterActions()
{
  s_hEditCategory = EZ_REGISTER_CATEGORY("EditCategory");
  s_hCopy =
      EZ_REGISTER_ACTION_1("Selection.Copy", ezActionScope::Document, "Document", "Ctrl+C", ezEditAction, ezEditAction::ButtonType::Copy);
  s_hPaste =
      EZ_REGISTER_ACTION_1("Selection.Paste", ezActionScope::Document, "Document", "Ctrl+V", ezEditAction, ezEditAction::ButtonType::Paste);
  s_hPasteAsChild = EZ_REGISTER_ACTION_1("Selection.PasteAsChild", ezActionScope::Document, "Document", "", ezEditAction,
                                         ezEditAction::ButtonType::PasteAsChild);
  s_hDelete =
      EZ_REGISTER_ACTION_1("Selection.Delete", ezActionScope::Document, "Document", "", ezEditAction, ezEditAction::ButtonType::Delete);
}

void ezEditActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hEditCategory);
  ezActionManager::UnregisterAction(s_hCopy);
  ezActionManager::UnregisterAction(s_hPaste);
  ezActionManager::UnregisterAction(s_hPasteAsChild);
  ezActionManager::UnregisterAction(s_hDelete);
}

void ezEditActions::MapActions(const char* szMapping, const char* szPath, bool bDeleteAction, bool bPasteAsChildAction)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/EditCategory");

  pMap->MapAction(s_hEditCategory, szPath, 3.5f);

  pMap->MapAction(s_hCopy, sSubPath, 1.0f);
  pMap->MapAction(s_hPaste, sSubPath, 2.0f);

  if (bPasteAsChildAction)
    pMap->MapAction(s_hPasteAsChild, sSubPath, 2.5f);

  if (bDeleteAction)
    pMap->MapAction(s_hDelete, sSubPath, 3.0f);
}


void ezEditActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/EditCategory");

  pMap->MapAction(s_hEditCategory, szPath, 10.0f);

  pMap->MapAction(s_hCopy, sSubPath, 1.0f);
  pMap->MapAction(s_hPasteAsChild, sSubPath, 2.0f);
  pMap->MapAction(s_hDelete, sSubPath, 3.0f);
}


void ezEditActions::MapViewContextMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/EditCategory");

  pMap->MapAction(s_hEditCategory, szPath, 10.0f);

  pMap->MapAction(s_hCopy, sSubPath, 1.0f);
  pMap->MapAction(s_hPasteAsChild, sSubPath, 2.0f);
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
    case ezEditAction::ButtonType::PasteAsChild:
      SetIconPath(":/GuiFoundation/Icons/Paste16.png"); /// \todo Icon
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
      ezStringBuilder sMimeType;

      ezAbstractObjectGraph graph;
      if (!m_Context.m_pDocument->CopySelectedObjects(graph, sMimeType))
        break;

      // Serialize to string
      ezMemoryStreamStorage streamStorage;
      ezMemoryStreamWriter memoryWriter(&streamStorage);
      ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph, nullptr, false);
      memoryWriter.WriteBytes("\0", 1); // null terminate

      // Write to clipboard
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize());

      mimeData->setData(sMimeType.GetData(), encodedData);
      mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
      clipboard->setMimeData(mimeData);
    }
    break;

    case ezEditAction::ButtonType::Paste:
    case ezEditAction::ButtonType::PasteAsChild:
    {
      // Check for clipboard data of the correct type.
      QClipboard* clipboard = QApplication::clipboard();
      auto mimedata = clipboard->mimeData();

      ezHybridArray<ezString, 4> MimeTypes;
      m_Context.m_pDocument->GetSupportedMimeTypesForPasting(MimeTypes);

      ezInt32 iFormat = -1;
      {
        for (ezUInt32 i = 0; i < MimeTypes.GetCount(); ++i)
        {
          if (mimedata->hasFormat(MimeTypes[i].GetData()))
          {
            iFormat = i;
            break;
          }
        }

        if (iFormat < 0)
          break;
      }

      // Paste at current selected object.
      ezPasteObjectsCommand cmd;
      cmd.m_sMimeType = MimeTypes[iFormat];

      QByteArray ba = mimedata->data(MimeTypes[iFormat].GetData());
      cmd.m_sGraphTextFormat = ba.data();

      if (m_ButtonType == ButtonType::PasteAsChild)
      {
        if (!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty())
          cmd.m_Parent = m_Context.m_pDocument->GetSelectionManager()->GetSelection().PeekBack()->GetGuid();
      }

      auto history = m_Context.m_pDocument->GetCommandHistory();

      history->StartTransaction("Paste");

      if (history->AddCommand(cmd).Failed())
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

void ezEditAction::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
