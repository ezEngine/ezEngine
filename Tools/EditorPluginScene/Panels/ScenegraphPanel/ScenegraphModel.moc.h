#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

class ezSceneDocument;

class ezQtScenegraphModel : public ezQtDocumentTreeModel
{
  Q_OBJECT

public:

  ezQtScenegraphModel(ezSceneDocument* pDocument);
  ~ezQtScenegraphModel();

  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;

protected:
  virtual void TreeEventHandler(const ezDocumentObjectStructureEvent& e) override;

private:
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);
  void SceneObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezSceneObjectMetaData>::EventData& e);
  void DetermineNodeName(const ezDocumentObject* pObject, const ezUuid& prefabGuid, ezStringBuilder& out_Result, QIcon& icon) const;

  ezSceneDocument* m_pSceneDocument;
};
