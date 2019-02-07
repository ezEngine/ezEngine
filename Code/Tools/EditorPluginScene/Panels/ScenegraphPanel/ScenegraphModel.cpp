#include <EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Strings/TranslationLookup.h>

ezQtScenegraphModel::ezQtScenegraphModel(ezSceneDocument* pDocument)
    : ezQtGameObjectModel(pDocument)
{
  m_pSceneDocument = pDocument;
}

ezQtScenegraphModel::~ezQtScenegraphModel() {}
