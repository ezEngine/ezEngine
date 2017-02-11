#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Material.h>

namespace ezModelImporter
{
  const Property* Material::GetProperty(SemanticHint::Enum hint) const
  {
    for (const Property& prop : m_Properties)
    {
      if (prop.m_SemanticHint == hint)
        return &prop;
    }
    return nullptr;
  }

  const TextureReference* Material::GetTexture(SemanticHint::Enum hint) const
  {
    for (const TextureReference& texture : m_Textures)
    {
      if (texture.m_SemanticHint == hint)
        return &texture;
    }
    return nullptr;
  }
}
