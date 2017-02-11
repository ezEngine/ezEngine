#pragma once

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
}
