#pragma once

#include <Foundation/Types/Variant.h>
#include <Foundation/Types/RefCounted.h>

namespace ezModelImporter
{
  /// Semantical meaning of a material property or texture
  ///
  /// Depending on the model format and the use by the artist, there may be different semantic specifier for the same property/texture or the same specifier for different properties.
  /// To make it easier to work with different formats, an importer implementation can try to map a input semantic to a predefined semantic.
  /// However, since a clear mapping is usually not possible or might lead to data loss we keep the original semantic as a string.
  ///
  /// For example one file format may specify a color with the semantic "diff" and another with "color".
  /// It is reasonable for a importer implementation to map both semantics to a DIFFUSE to make further processing easier.
  ///
  /// Note that most, but not all semantic hints are applicable for both textures and properties.
  /// Parameters are commented to make their expected meaning clearer, but usage may vary!
  struct SemanticHint
  {
    enum Enum
    {
      // Typical pbr shading model parameters.
      DIFFUSE,    ///< Also called "Color", "BaseColor"
      ROUGHNESS,  ///< The non-pbr related "SpecularPower" maps to this as well.
      METALLIC,   ///< The non-pbr related "SpecularColor" maps to this as well

      REFLECTIVITY, ///< https://en.wikipedia.org/wiki/Reflectance#Reflectivity
      REFRACTIONINDEX,  ///< Refraction index. "IOR"

      // Other lighting information.
      AMBIENT,  ///< Usually found in non-pbr materials or for Ambient Occlusion in pbr materials.
      EMISSIVE, ///< Self emmitance of an object.
      LIGHTMAP, ///< Prebacked lighting information, usually irradiance.

      /// How transparent an object is. Also "transmissivity"
      /// Note that some models distinguish between physical opacity and a more articial fade/transparency that also specular reflections.
      /// Some older shading models may use a "Transparency Color"
      OPACITY,

      // Topology.
      NORMAL,       ///< Tangent space normal representation. Also called "bumpmap".
      DISPLACEMENT, ///< Relief height information. Also called "heightmap".

      // Other rendering information.
      TWOSIDED,
      WIREFRAME,
      SHADINGMODEL, ///< Some hint for the used shading model.

      UNKNOWN = -1,
    };
  };

  struct TextureReference
  {
    TextureReference() : m_SemanticHint(SemanticHint::UNKNOWN), m_Semantic(""), m_FileName(), m_UVSetIndex(0) {}
    TextureReference(SemanticHint::Enum semanticHint, const char* semantic, const char* filename) : m_SemanticHint(semanticHint), m_Semantic(semantic), m_FileName(filename), m_UVSetIndex(0) {}
    TextureReference(const char* semantic, const char* filename) : m_SemanticHint(SemanticHint::UNKNOWN), m_Semantic(semantic), m_FileName(filename), m_UVSetIndex(0) {}

    SemanticHint::Enum m_SemanticHint;
    ezString m_Semantic;
    /// Relative file path to the texture.
    ezString m_FileName;
    /// Texcoord set used by this texture.
    ezUInt32 m_UVSetIndex;
  };

  struct Property
  {
    Property() : m_SemanticHint(SemanticHint::UNKNOWN), m_Semantic(""), m_Value() {}
    Property(SemanticHint::Enum semanticHint, const char* semantic, const ezVariant& value) : m_SemanticHint(semanticHint), m_Semantic(semantic), m_Value(value) {}
    Property(const char* semantic, const ezVariant& value) : m_SemanticHint(SemanticHint::UNKNOWN), m_Semantic(semantic), m_Value(value) {}

    SemanticHint::Enum m_SemanticHint;
    ezString m_Semantic;
    ezVariant m_Value;
  };

  struct Material : public ezRefCounted
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
