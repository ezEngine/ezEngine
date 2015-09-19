#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezMaterialAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetProperties);

public:
  ezMaterialAssetProperties();

  ezString m_sBaseMaterial;
  ezString m_sShader;
  ezString m_sPermutationVarValues;
  ezString m_sTextureDiffuse;
  ezString m_sTextureMask;
  ezString m_sTextureNormal;
  

private:

};




