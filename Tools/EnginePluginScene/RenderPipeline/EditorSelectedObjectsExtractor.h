#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/Extractor.h>

class ezSceneContext;
class ezCameraComponent;

class ezEditorSelectedObjectsExtractor : public ezSelectedObjectsExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorSelectedObjectsExtractor, ezSelectedObjectsExtractor);

public:
  ezEditorSelectedObjectsExtractor();
  ~ezEditorSelectedObjectsExtractor();

  virtual const ezDeque<ezGameObjectHandle>* GetSelection() override;

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
                       ezExtractedRenderData& extractedRenderData) override;

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  ezSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void CreateRenderTargetTexture(const ezView& view);
  void CreateRenderTargetView(const ezView& view);
  void UpdateRenderTargetCamera(const ezCameraComponent* pCamComp);

  ezSceneContext* m_pSceneContext;
  ezViewHandle m_hRenderTargetView;
  ezRenderToTexture2DResourceHandle m_hRenderTarget;
  ezCamera m_RenderTargetCamera;
};
