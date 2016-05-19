#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

class QMimeData;

class EZ_EDITORFRAMEWORK_DLL ezDragDropInfo : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDragDropInfo, ezReflectedClass);

public:
  ezDragDropInfo();

  const QMimeData* m_pMimeData;

  /// A string identifying into what context the object is dropped, e.g. "viewport" or "scenetree" etc.
  ezString m_sTargetContext;

  /// The ezDocument GUID
  ezUuid m_TargetDocument;

  /// GUID of the ezDocumentObject that is at the dropped position. May be invalid. Can be used to attach as a child, to modify the object itself or can be ignored.
  ezUuid m_TargetObject;

  /// GUID of the ezDocumentObject that is the more specific component (of m_TargetObject) that was dragged on. May be invalid.
  ezUuid m_TargetComponent;

  /// World space position where the object is dropped. May be NaN.
  ezVec3 m_vDropPosition;

  /// World space normal at the point where the object is dropped. May be NaN.
  ezVec3 m_vDropNormal;

  /// Some kind of index / ID for the object that is at the drop location. For meshes this is the material index.
  ezInt32 m_iTargetObjectSubID;

  /// If dropped on a scene tree, this may say as which child the object is supposed to be inserted. -1 if invalid (ie. append)
  ezInt32 m_iTargetObjectInsertChildIndex;
};