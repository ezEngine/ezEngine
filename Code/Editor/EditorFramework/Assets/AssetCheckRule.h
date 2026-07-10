#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Uuid.h>

class ezDocument;
class ezDocumentObject;
class ezObjectAccessorBase;
class ezAssetChecker;
class ezAbstractProperty;

/// Severity of an ezAssetCheckNote.
struct ezAssetCheckSeverity
{
  enum Enum : ezUInt8
  {
    Warning,
    Error,
  };
};

/// A single issue reported by an ezAssetCheckRule for one asset.
struct EZ_EDITORFRAMEWORK_DLL ezAssetCheckNote
{
  ezAssetCheckSeverity::Enum m_Severity = ezAssetCheckSeverity::Warning;
  ezString m_sMessage;
  ezUuid m_ObjectGuid;   ///< Invalid if the note is not tied to a specific object.
  bool m_bFixed = false; ///< True if an auto-fix resolved this issue.
};

/// Passed to ezAssetCheckRule::CheckDocument to inspect the document and report issues.
///
/// The rule reports issues via ReportIssue. If IsAutoFixAllowed returns true, the rule may also
/// modify the document through GetObjectAccessor and report the fixed issues with bFixed = true.
/// The runner (ezAssetChecker) owns the undo transaction; rules must not start or finish one.
class EZ_EDITORFRAMEWORK_DLL ezAssetCheckContext
{
public:
  ezDocument* GetDocument() const { return m_pDocument; }
  ezObjectAccessorBase* GetObjectAccessor() const;

  /// Whether the rule is allowed to modify the document to fix issues.
  bool IsAutoFixAllowed() const { return m_bAutoFix; }

  /// Records an issue. When bFixed is true, m_uiFixCount is incremented so the runner knows the
  /// document was modified and needs to be saved.
  void ReportIssue(ezAssetCheckSeverity::Enum severity, ezStringView sMessage, const ezDocumentObject* pObject = nullptr, bool bFixed = false);

  /// Returns the object's "Name" property (if present and non-empty), otherwise its type name.
  static ezString GetObjectDisplayName(const ezDocumentObject* pObject);

private:
  friend class ezAssetChecker;

  ezDocument* m_pDocument = nullptr;
  bool m_bAutoFix = false;
  ezUInt32 m_uiFixCount = 0; ///< Reset per rule by the runner; counts fixes applied to the document.
  ezDynamicArray<ezAssetCheckNote>* m_pNotes = nullptr;
};

/// Base class for asset check rules.
///
/// Derive in any editor plugin (with EZ_ADD_DYNAMIC_REFLECTION and a default allocator) and the
/// rule is auto-discovered via reflection. A rule inspects an open document and reports issues,
/// optionally fixing them when auto-fix is enabled.
class EZ_EDITORFRAMEWORK_DLL ezAssetCheckRule : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetCheckRule, ezReflectedClass);

public:
  virtual ezStringView GetDisplayName() const = 0;
  virtual ezStringView GetDescription() const = 0;

  /// Whether the rule can fix the issues it reports. If false, auto-fix has no effect for it.
  virtual bool CanFix() const { return false; }

  /// Whether the rule should run on documents of the given type. Defaults to all types.
  virtual bool AppliesToDocumentType(ezStringView sDocumentTypeName) const { return true; }

  /// Inspects the document and reports all issues via ref_ctx.ReportIssue.
  ///
  /// The default implementation visits the document's root object and every object below it and
  /// calls CheckObject for each (child objects reached through properties marked with
  /// ezTemporaryAttribute are skipped). Rules that follow this per-object / per-property structure
  /// only need to override CheckObject or CheckProperty. Override CheckDocument itself to replace
  /// the traversal entirely.
  ///
  /// If ref_ctx.IsAutoFixAllowed() is true, the rule also fixes the issues through
  /// ref_ctx.GetObjectAccessor() and reports them with bFixed = true. The runner manages the undo
  /// transaction, so implementations must not call Start/Finish/CancelTransaction.
  virtual void CheckDocument(ezAssetCheckContext& ref_ctx);

  /// Allocates one instance of every reflected, allocatable rule type, sorted by display name.
  static void CreateRules(ezDynamicArray<ezAssetCheckRule*>& out_rules);
  static void DestroyRules(ezDynamicArray<ezAssetCheckRule*>& ref_rules);

protected:
  /// Inspects a single object as part of the default CheckDocument traversal.
  ///
  /// The default iterates all of the object's properties, skipping those marked with
  /// ezTemporaryAttribute, and calls CheckProperty for each. Child objects are visited separately
  /// by the traversal and must not be recursed into here.
  virtual void CheckObject(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject);

  /// Inspects a single property of an object. The default does nothing.
  ///
  /// Called for Member, Array, Set and Map properties alike. Use GetPropertyValues to read the
  /// contained element values without having to special-case the property category.
  virtual void CheckProperty(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject, const ezAbstractProperty* pProp);

  /// Reads all element values of a property, regardless of its category.
  ///
  /// Member properties yield their single value; Array and Set properties yield all elements; Map
  /// properties yield all values. out_indices receives the index/key that addresses each value,
  /// usable with ezObjectAccessorBase::RemoveValue and related functions; for Member properties it
  /// is an invalid ezVariant. Returns EZ_FAILURE for categories that hold no readable values (such
  /// as pointer properties, whose targets are visited as separate child objects).
  static ezResult GetPropertyValues(ezObjectAccessorBase* pAcc, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_indices, ezDynamicArray<ezVariant>& out_values);

private:
  void VisitObjectRecursive(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject);
};
