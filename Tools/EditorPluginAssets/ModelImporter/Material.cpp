#include <Tools/EditorPluginAssets/ModelImporter/Material.h>

namespace ezModelImporter
{
  const Material::Property* Material::GetProperty(SemanticHint::Enum hint) const
  {
    for (const Material::Property& prop : m_Properties)
    {
      if (prop.m_SemanticHint == hint)
        return &prop;
    }
    return nullptr;
  }

  const Material::TextureReference* Material::GetTexture(SemanticHint::Enum hint) const
  {
    for (const Material::TextureReference& texture : m_Textures)
    {
      if (texture.m_SemanticHint == hint)
        return &texture;
    }
    return nullptr;
  }
}
