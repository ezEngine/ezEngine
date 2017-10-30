#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezQtParticleEffectAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtParticleViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT

public:
  ezQtParticleViewWidget(QWidget* pParent, ezQtParticleEffectAssetDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtParticleViewWidget();

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

