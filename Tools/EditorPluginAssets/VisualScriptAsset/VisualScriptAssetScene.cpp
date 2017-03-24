#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetScene.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>


ezQtVisualScriptAssetScene::ezQtVisualScriptAssetScene(QObject* parent)
  : ezQtNodeScene(parent)
{

}


ezQtVisualScriptAssetScene::~ezQtVisualScriptAssetScene()
{
}

void ezQtVisualScriptAssetScene::ConnectPinsAction(const ezPin* pSourcePin, const ezPin* pTargetPin)
{
  ezStatus res = m_pManager->CanConnect(pSourcePin, pTargetPin);
  if (res.m_Result.Succeeded())
  {
    ezQtNodeScene::ConnectPinsAction(pSourcePin, pTargetPin);
    return;
  }

  if (!pTargetPin->GetConnections().IsEmpty())
  {
    // If we already have a connection at this input pin, delete it.
    ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Replace Input Pin");

    {
      const ezArrayPtr<const ezConnection* const> connections = pTargetPin->GetConnections();
      EZ_ASSERT_DEV(connections.GetCount() == 1, "A visual script should only support one input connection at a time.");
      const ezConnection* pConnection = connections[0];

      ezDisconnectNodePinsCommand cmd;
      cmd.m_ObjectSource = pConnection->GetSourcePin()->GetParent()->GetGuid();
      cmd.m_ObjectTarget = pConnection->GetTargetPin()->GetParent()->GetGuid();
      cmd.m_sSourcePin = pConnection->GetSourcePin()->GetName();
      cmd.m_sTargetPin = pConnection->GetTargetPin()->GetName();

      res = history->AddCommand(cmd);
    }
    {
      ezConnectNodePinsCommand cmd;
      cmd.m_ObjectSource = pSourcePin->GetParent()->GetGuid();
      cmd.m_ObjectTarget = pTargetPin->GetParent()->GetGuid();
      cmd.m_sSourcePin = pSourcePin->GetName();
      cmd.m_sTargetPin = pTargetPin->GetName();

      res = history->AddCommand(cmd);
    }
    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();

    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node connect failed.");
  }
}
