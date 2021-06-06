#pragma once

#include <EditorPluginScene/Scene/SceneDocument.h>

struct ezScene2LayerEvent
{
  enum class Type
  {
    LayerAdded,
    LayerRemoved,
    ActiveLayerChanged,
  };

  Type m_Type;
  ezSceneDocument* m_pDocument = nullptr;
};

class ezScene2Document : public ezSceneDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScene2Document, ezSceneDocument);

public:
  ezScene2Document(const char* szDocumentPath);
  ~ezScene2Document();

public:
  mutable ezEvent<const ezScene2LayerEvent&> m_LayerEvents;
};
