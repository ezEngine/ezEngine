#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAsset.h>

class QLabel;
class QScrollArea;

class ezSoundEventAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezSoundEventAssetDocumentWindow(ezDocument* pDocument);
  ~ezSoundEventAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "SoundEventAsset"; }

private slots:
  

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void SoundEventAssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e);

  ezSoundEventAssetDocument* m_pAssetDoc;
  QLabel* m_pLabelInfo;
};