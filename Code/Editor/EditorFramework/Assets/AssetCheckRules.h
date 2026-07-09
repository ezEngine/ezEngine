#pragma once

#include <EditorFramework/Assets/AssetCheckRule.h>

/// Reports tag-set entries that are not registered in the project's tag configuration.
///
/// Unknown tags are ignored at runtime, so these are reported as warnings. Auto-fix removes them.
class EZ_EDITORFRAMEWORK_DLL ezUnknownTagsAssetCheckRule : public ezAssetCheckRule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUnknownTagsAssetCheckRule, ezAssetCheckRule);

public:
  virtual ezStringView GetDisplayName() const override { return "Unknown Tags"; }
  virtual ezStringView GetDescription() const override;
  virtual bool CanFix() const override { return true; }

protected:
  virtual void CheckProperty(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject, const ezAbstractProperty* pProp) override;
};

/// Reports properties marked with ezRequiredAttribute that are left empty, or, for game object /
/// component reference properties, that reference an object which does not exist (anymore).
///
/// There is no sensible value to auto-fix this with, so these are always reported as errors.
class EZ_EDITORFRAMEWORK_DLL ezRequiredPropertyAssetCheckRule : public ezAssetCheckRule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRequiredPropertyAssetCheckRule, ezAssetCheckRule);

public:
  virtual ezStringView GetDisplayName() const override { return "Required Properties"; }
  virtual ezStringView GetDescription() const override;

protected:
  virtual void CheckProperty(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject, const ezAbstractProperty* pProp) override;
};

/// Reports ezGameObjects that have no name, no components and no child objects, i.e. that have no
/// effect on the scene. Auto-fix removes them.
class EZ_EDITORFRAMEWORK_DLL ezEmptyGameObjectAssetCheckRule : public ezAssetCheckRule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEmptyGameObjectAssetCheckRule, ezAssetCheckRule);

public:
  virtual ezStringView GetDisplayName() const override { return "Empty Game Objects"; }
  virtual ezStringView GetDescription() const override;
  virtual bool CanFix() const override { return true; }
  virtual bool AppliesToDocumentType(ezStringView sDocumentTypeName) const override;

  virtual void CheckDocument(ezAssetCheckContext& ref_ctx) override;

protected:
  virtual void CheckObject(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject) override;

private:
  // Objects are collected while walking the tree and only removed afterwards, since removing an
  // object during the traversal would invalidate the sibling array a parent frame is iterating over.
  ezDynamicArray<const ezDocumentObject*> m_EmptyObjects;
};
