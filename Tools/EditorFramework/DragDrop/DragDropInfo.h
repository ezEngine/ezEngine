#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

class QMimeData;

/// \brief This type is used to provide ezDragDropHandler instances with all the important information for a drag & drop target
///
/// It is a reflected class such that one can derive and extend it, if necessary.
/// DragDrop handlers can then inspect whether it is a known extended type and cast to the type to get access to additional information.
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

  bool m_bShiftKeyDown;
  bool m_bCtrlKeyDown;
};


/// \brief After an ezDragDropHandler has been chosen to handle an operation, it is queried once to fill out an instance of this type (or an extended derived type)
/// to enable configuring how ezDragDropInfo is computed by the target.
class EZ_EDITORFRAMEWORK_DLL ezDragDropConfig : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDragDropConfig, ezReflectedClass);

public:
  ezDragDropConfig();

  /// Whether the currently selected objects (ie the dragged objects) should be considered for picking or not. Default is disabled.
  bool m_bPickSelectedObjects;
};