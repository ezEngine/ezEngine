#pragma once

#include <EditorPluginFmod/SoundEventAsset/SoundEventAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;

class ezSoundEventAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezSoundEventAssetDocumentWindow(ezDocument* pDocument);
  ~ezSoundEventAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "SoundEventAsset"; }
  virtual const char* GetWindowLayoutGroupName() const { return "SoundEventAsset"; }

private Q_SLOTS:


private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  ezSoundEventAssetDocument* m_pAssetDoc = nullptr;
  QLabel* m_pLabelInfo = nullptr;
};
