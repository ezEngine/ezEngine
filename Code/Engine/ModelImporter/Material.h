#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/Variant.h>
#include <ModelImporter/Declarations.h>

namespace ezModelImporter
{
  struct EZ_MODELIMPORTER_DLL TextureReference
  {
    TextureReference()
        : m_SemanticHint(SemanticHint::UNKNOWN)
        , m_Semantic("")
        , m_FileName()
        , m_UVSetIndex(0)
    {
    }
    TextureReference(SemanticHint::Enum semanticHint, const char* semantic, const char* filename)
        : m_SemanticHint(semanticHint)
        , m_Semantic(semantic)
        , m_FileName(filename)
        , m_UVSetIndex(0)
    {
    }
    TextureReference(const char* semantic, const char* filename)
        : m_SemanticHint(SemanticHint::UNKNOWN)
        , m_Semantic(semantic)
        , m_FileName(filename)
        , m_UVSetIndex(0)
    {
    }

    SemanticHint::Enum m_SemanticHint;
    ezString m_Semantic;
    /// Relative file path to the texture.
    ezString m_FileName;
    /// Texcoord set used by this texture.
    ezUInt32 m_UVSetIndex;
  };

  struct EZ_MODELIMPORTER_DLL Property
  {
    Property()
        : m_SemanticHint(SemanticHint::UNKNOWN)
        , m_Semantic("")
        , m_Value()
    {
    }
    Property(SemanticHint::Enum semanticHint, const char* semantic, const ezVariant& value)
        : m_SemanticHint(semanticHint)
        , m_Semantic(semantic)
        , m_Value(value)
    {
    }
    Property(const char* semantic, const ezVariant& value)
        : m_SemanticHint(SemanticHint::UNKNOWN)
        , m_Semantic(semantic)
        , m_Value(value)
    {
    }

    SemanticHint::Enum m_SemanticHint;
    ezString m_Semantic;
    ezVariant m_Value;
  };

  struct EZ_MODELIMPORTER_DLL Material : public ezRefCounted
  {
    /// Retrieves first property with the given semantic hint. Null if there is none.
    const Property* GetProperty(SemanticHint::Enum hint) const;

    /// Retrieves first texture with the given semantic hint. Null if there is none.
    const TextureReference* GetTexture(SemanticHint::Enum hint) const;


    ezString m_Name;
    ezHybridArray<TextureReference, 4> m_Textures;
    ezHybridArray<Property, 16> m_Properties;
  };
}

