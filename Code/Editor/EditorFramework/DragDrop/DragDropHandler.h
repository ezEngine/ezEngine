#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezDragDropInfo;
class ezDragDropConfig;

class EZ_EDITORFRAMEWORK_DLL ezDragDropHandler : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDragDropHandler, ezReflectedClass);

public:

  /// \brief Returns whether the last call to BeginDragDropOperation() was successful and a handler is now in effect.
  static bool IsHandlerActive() { return s_pActiveDnD != nullptr; }

  /// \brief Call this when a drag enter event occurs. Return value indicates whether a ezDragDropHandler was found to handle the operation. If not, subsequent drag & drop updates are ignored.
  static bool BeginDragDropOperation(const ezDragDropInfo* pInfo, ezDragDropConfig* pConfigToFillOut = nullptr);

  /// \brief Call this when a drag event occurs. Ignored if BeginDragDropOperation() was not successful.
  static void UpdateDragDropOperation(const ezDragDropInfo* pInfo);

  /// \brief Call this when a drop event occurs. Ignored if BeginDragDropOperation() was not successful.
  static void FinishDragDrop(const ezDragDropInfo* pInfo);

  /// \brief Call this when a drag leave event occurs. Ignored if BeginDragDropOperation() was not successful.
  static void CancelDragDrop();


  /// \brief For targets that do not support full dragging, but only dropping on a single target, this allows to query whether there is a handler for the given target. See also DropOnly().
  static bool CanDropOnly(const ezDragDropInfo* pInfo);

  /// \brief Executes a complete drop action on a target that does not support continuous dragging. See also CanDropOnly().
  static bool DropOnly(const ezDragDropInfo* pInfo);

public:
  ezDragDropHandler();


protected:

  /// \brief Used to ask a handler whether it knows how to handle a certain drag & drop situation.
  ///
  /// The return value is a priority. By default CanHandle should return 0 or 1. To override an existing handler, values larger than 1 may be returned to take precedence.
  virtual float CanHandle(const ezDragDropInfo* pInfo) const = 0;

  /// \brief Potentially called by the drag drop target to request information about how to determine the ezDragDropInfo data.
  virtual void RequestConfiguration(ezDragDropConfig* pConfigToFillOut) {}

  /// \brief Called shortly after CanHandle returned true to begin handling a drag operation.
  virtual void OnDragBegin(const ezDragDropInfo* pInfo) = 0;

  /// \brief Called to update the drag operation with the latest state.
  virtual void OnDragUpdate(const ezDragDropInfo* pInfo) = 0;

  /// \brief Called when the drag operation leaves the designated area. The handler will be destroyed after this. It should clean up all temporary objects that it created before.
  virtual void OnDragCancel() = 0;

  /// \brief Final call to finish the drag & drop operation. Handler is destroyed after this.
  virtual void OnDrop(const ezDragDropInfo* pInfo) = 0;

private:
  static ezDragDropHandler* FindDragDropHandler(const ezDragDropInfo* pInfo);

  static ezDragDropHandler* s_pActiveDnD;
};
