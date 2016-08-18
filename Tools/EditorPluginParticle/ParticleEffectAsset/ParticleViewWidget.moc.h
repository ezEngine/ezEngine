#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

class ezParticleEffectAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtParticleViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtParticleViewWidget(QWidget* pParent, ezParticleEffectAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtParticleViewWidget();

private:
  ezOrbitCameraContext* m_pOrbitCameraContext;
};
