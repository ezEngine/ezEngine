#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginProceduralPlacement/EnginePluginProceduralPlacementDLL.h>

#include <ProceduralPlacementPlugin/Resources/ProceduralPlacementResource.h>

class EZ_ENGINEPLUGINPROCEDURALPLACEMENT_DLL ezProceduralPlacementContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementContext, ezEngineProcessDocumentContext);

public:
  ezProceduralPlacementContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  ezProceduralPlacementResourceHandle m_hProcGen;
};
