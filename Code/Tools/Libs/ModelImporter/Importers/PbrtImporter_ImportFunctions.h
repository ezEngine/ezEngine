﻿#pragma once

#include <ModelImporter/Importers/PbrtImporter_Declarations.h>

namespace ezModelImporter
{
  namespace PbrtScopeFunctions
  {
    void WorldBegin(Pbrt::ParseContext& context);

    void WorldEnd(Pbrt::ParseContext& context);

    void AttributeBegin(Pbrt::ParseContext& context);
    void AttributeEnd(Pbrt::ParseContext& context);

    void TransformBegin(Pbrt::ParseContext& context);
    void TransformEnd(Pbrt::ParseContext& context);

    //void ObjectBegin(Pbrt::ParseContext& context); // Handled as object parse function.
    void ObjectEnd(Pbrt::ParseContext& context);
  }

  namespace PbrtTransformFunctions
  {
    void Identity(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
    void Translate(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
    void Rotate(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
    void Scale(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
    void LookAt(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
    void Transform(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
    void ConcatTransform(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
    //void CoordinateSystem(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
    //void CoordSysTransform(Pbrt::ParseContext& context, ezStringView& remainingSceneText);
  }

  namespace PbrtObjectParseFunctions
  {
    void Shape(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene);
    void Material(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene);
    void MakeNamedMaterial(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene);
    void NamedMaterial(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene);
    void Texture(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene);

    void ObjectBegin(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene);
    void ObjectInstance(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene);
  }
}
