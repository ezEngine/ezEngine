#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <CoreUtils/DataStructures/ObjectMetaData.h>
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

private:
  void ObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezSceneObjectMetaData>::EventData& e);
  void DetermineNodeName(const ezDocumentObject* pObject, ezStringBuilder& out_Result) const;

  ezSceneDocument* m_pSceneDocument;
};