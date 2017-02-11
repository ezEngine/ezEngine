#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>

class ezQtParticleEffectAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtParticleViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT

public:
  ezQtParticleViewWidget(QWidget* pParent, ezQtParticleEffectAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtParticleViewWidget();

private:
  ezOrbitCameraContext* m_pOrbitCameraContext;
};

