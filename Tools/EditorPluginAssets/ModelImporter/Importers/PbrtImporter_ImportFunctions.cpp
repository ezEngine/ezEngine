#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Importers/PbrtImporter_Context.h>
#include <EditorPluginAssets/ModelImporter/Importers/PbrtImporter_ImportFunctions.h>
#include <EditorPluginAssets/ModelImporter/Importers/PbrtImporter_ParsingFunctions.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <EditorPluginAssets/ModelImporter/Material.h>
#include <EditorPluginAssets/ModelImporter/ModelImporter.h>

#include <Foundation/Logging/Log.h>

namespace ezModelImporter
{
  using namespace Pbrt;

  namespace PbrtScopeFunctions
  {
    void WorldBegin(ParseContext& context)
    {
      context.EnterWorld();
    }

    void WorldEnd(ParseContext& context)
    {
      context.ExitWorld();
    }

    void AttributeBegin(ParseContext& context)
    {
      //context.PushActiveMaterial();
      context.PushActiveTransform();
    }

    void AttributeEnd(ParseContext& context)
    {
      //context.PopActiveMaterial();
      context.PopActiveTransform();
    }

    void TransformBegin(ParseContext& context)
    {
      context.PushActiveTransform();
    }

    void TransformEnd(ParseContext& context)
    {
      context.PopActiveTransform();
    }

    void ObjectEnd(ParseContext& context)
    {
      context.ObjectEnd();
    }
  }

  namespace PbrtTransformFunctions
  {
    void Identity(Pbrt::ParseContext& context, ezStringView& remainingSceneText)
    {
      context.PeekActiveTransform().SetIdentity();
    }

    void Translate(ParseContext& context, ezStringView& remainingSceneText)
    {
      ezVec3 translation;
      if (PbrtParseHelper::ParseVec3(remainingSceneText, translation).Succeeded())
      {
        context.PeekActiveTransform().SetGlobalTransform(context.PeekActiveTransform(), ezTransform(translation));
      }
      else
        ezLog::Error("Failed parsing Translate transform command.");
    }
    void Rotate(ParseContext& context, ezStringView& remainingSceneText)
    {
      float values[4];
      if (PbrtParseHelper::ParseFloats(remainingSceneText, ezMakeArrayPtr(values), 4).Succeeded())
      {
        ezQuat rotation;
        rotation.SetFromAxisAndAngle(ezVec3(values[1], values[2], values[3]), ezAngle::Degree(values[0]));
        context.PeekActiveTransform().SetGlobalTransform(context.PeekActiveTransform(), ezTransform(ezVec3(0.0f), rotation));
      }
      else
        ezLog::Error("Failed parsing Rotate transform command.");
    }
    void Scale(ParseContext& context, ezStringView& remainingSceneText)
    {
      ezVec3 scale;
      if (PbrtParseHelper::ParseVec3(remainingSceneText, scale).Succeeded())
      {
        context.PeekActiveTransform().SetGlobalTransform(context.PeekActiveTransform(), ezTransform(ezVec3(0.0f), ezQuat::IdentityQuaternion(), scale));
      }
      else
        ezLog::Error("Failed parsing Scale transform command.");
    }

    void LookAt(ParseContext& context, ezStringView& remainingSceneText)
    {
      float values[9];
      if (PbrtParseHelper::ParseFloats(remainingSceneText, ezMakeArrayPtr(values), 9).Succeeded())
      {
        ezMat4 lookAt;
        lookAt.SetLookAtMatrix(ezVec3(values[0], values[1], values[2]), ezVec3(values[3], values[4], values[5]), ezVec3(values[6], values[7], values[8]));
        context.PeekActiveTransform().SetGlobalTransform(context.PeekActiveTransform(), ezTransform(lookAt));
      }
      else
        ezLog::Error("Failed parsing LookAt transform command.");
    }

    ezResult ParseMat4(ezStringView& remainingSceneText, ezMat4& outMat)
    {
      return PbrtParseHelper::ParseFloats(remainingSceneText, ezArrayPtr<float>(&outMat.m_fElementsCM[0], 16), 16);
    }

    void Transform(ParseContext& context, ezStringView& remainingSceneText)
    {
      ezMat4 mat;
      if (ParseMat4(remainingSceneText, mat).Succeeded())
        context.PeekActiveTransform() = ezTransform(mat);
      else
        ezLog::Error("Failed parsing Transform transform command.");
    }

    void ConcatTransform(ParseContext& context, ezStringView& remainingSceneText)
    {
      ezMat4 mat;
      if (ParseMat4(remainingSceneText, mat).Succeeded())
        context.PeekActiveTransform().SetGlobalTransform(context.PeekActiveTransform(), ezTransform(mat));
      else
        ezLog::Error("Failed parsing Transform transform command.");
    }

    //void CoordinateSystem(Pbrt::ParseContext& context, ezStringView& remainingSceneText)
    //{
    //  ezStringView name = PbrtParseHelper::ReadBlock(remainingSceneText, '\"', '\"');
    //}

    //void CoordSysTransform(Pbrt::ParseContext& context, ezStringView& remainingSceneText)
    //{
    //}
  }

  namespace PbrtObjectParseFunctions
  {
    template<typename T>
    void CopyParamToArray(ezDynamicArray<T>& targetArray, const Parameter& param)
    {
      targetArray.Reserve(param.data.GetCount());
      for (ezUInt32 elem = 0; elem < param.data.GetCount(); ++elem)
        targetArray.PushBack(param.data[elem].Get<T>());
    }

    void Shape(ezStringView type, ezArrayPtr<Parameter> parameters, ParseContext& context, ezModelImporter::Scene& outScene)
    {
      // Get/Create node for current transform.
      ObjectHandle parentNodeHandle;
      if (!context.GetActiveObject() && !context.PeekActiveTransform().IsIdentical(ezTransform::Identity()))
      {
        // todo: Recycle node if it hasn't changed since last time.
        ezUniquePtr<Node> node = EZ_DEFAULT_NEW(Node);
        node->m_RelativeTransform = context.PeekActiveTransform();
        parentNodeHandle = outScene.AddNode(std::move(node));
      }

      ezUniquePtr<ezModelImporter::Mesh> mesh;

      if (type.IsEqual_NoCase("plymesh"))
      {
        if (parameters.IsEmpty())
        {
          ezLog::ErrorPrintf("Expected at least single parameter for plymesh shape.", type.GetData());
          return;
        }
        if (!parameters[0].name.IsEqual_NoCase("filename") || parameters[0].data.GetCount() != 1 || !parameters[0].data[0].IsA<ezString>())
        {
          ezLog::ErrorPrintf("Expected filename parameter for plymesh shape.", type.GetData());
          return;
        }

        // Load mesh file
        ezStringBuilder meshFilename = context.GetModelFilename();
        meshFilename = meshFilename.GetFileDirectory();
        meshFilename.AppendPath(parameters[0].data[0].Get<ezString>());
        ezSharedPtr<Scene> subScene = ezModelImporter::Importer::GetSingleton()->ImportScene(meshFilename);
        if (!subScene)
        {
          ezLog::Error("Failed to load mesh '{0}'.", meshFilename.GetData());
          return;
        }
        mesh = EZ_DEFAULT_NEW(ezModelImporter::Mesh, std::move(*subScene->MergeAllMeshes()));
      }
      else if (type.IsEqual_NoCase("trianglemesh"))
      {
        // Read data.
        ezDynamicArray<int> indices;
        ezDynamicArray<ezVec3> positions;
        ezDynamicArray<ezVec3> normals;
        ezDynamicArray<ezVec3> tangents;
        ezDynamicArray<float> texcoords;
        for (ezUInt32 i = 0; i < parameters.GetCount(); ++i)
        {
          if (parameters[i].name.IsEqual_NoCase("P"))
          {
            if(parameters[i].type != ParamType::VECTOR3)
              ezLog::WarningPrintf("PBRT triangle mesh parameter 'P' is not a vec3 array.", type.GetData());
            CopyParamToArray(positions, parameters[i]);
          }
          else if (parameters[i].name.IsEqual_NoCase("N"))
          {
            if (parameters[i].type != ParamType::VECTOR3)
              ezLog::WarningPrintf("PBRT triangle mesh parameter 'N' is not a vec3 array.", type.GetData());
            CopyParamToArray(normals, parameters[i]);
          }
          else if (parameters[i].name.IsEqual_NoCase("S"))
          {
            if (parameters[i].type != ParamType::VECTOR3)
              ezLog::WarningPrintf("PBRT triangle mesh parameter 'S' is not a vec3 array.", type.GetData());
            CopyParamToArray(tangents, parameters[i]);
          }
          else if (parameters[i].name.IsEqual_NoCase("uv"))
          {
            if (parameters[i].type != ParamType::FLOAT)
              ezLog::WarningPrintf("PBRT triangle mesh parameter 'uv' is not a float array.", type.GetData());
            CopyParamToArray(texcoords, parameters[i]);
          }
          else if (parameters[i].name.IsEqual_NoCase("indices"))
          {
            if (parameters[i].type != ParamType::INT)
              ezLog::WarningPrintf("PBRT triangle mesh parameter 'indece' is not an int array.", type.GetData());
            CopyParamToArray(indices, parameters[i]);
          }
        }


        if (positions.IsEmpty())
        {
          ezLog::ErrorPrintf("PBRT triangle mesh has no positions.", type.GetData());
          return;
        }
        if (indices.IsEmpty())
        {
          ezLog::ErrorPrintf("PBRT triangle mesh has no indices.", type.GetData());
          return;
        }
        if (indices.GetCount() % 3 != 0)
        {
          ezLog::ErrorPrintf("PBRT triangle mesh has not n*3 indices.", type.GetData());
          return;
        }
        if (texcoords.GetCount() % 2 != 0)
        {
          ezLog::ErrorPrintf("PBRT triangle mesh has not n*2 floats in its texcoord array.", type.GetData());
          return;
        }

        // Build mesh.
        mesh = EZ_DEFAULT_NEW(ezModelImporter::Mesh);
        mesh->AddTriangles(indices.GetCount() / 3);

        ezHybridArray<VertexDataStream*, 4> streams;

        VertexDataStream* positionStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::Position, 3);
        streams.PushBack(positionStream);
        positionStream->ReserveData(positions.GetCount());
        for (ezUInt32 i = 0; i < positions.GetCount(); ++i)
          positionStream->AddValue(positions[i]);

        if (!normals.IsEmpty())
        {
          VertexDataStream* normalStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::Normal, 3);
          streams.PushBack(normalStream);
          normalStream->ReserveData(normals.GetCount());
          for (ezUInt32 i = 0; i < normals.GetCount(); ++i)
            normalStream->AddValue(normals[i]);
        }
        if (!tangents.IsEmpty())
        {
          VertexDataStream* tangentStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::Tangent, 3);
          streams.PushBack(tangentStream);
          tangentStream->ReserveData(tangents.GetCount());
          for (ezUInt32 i = 0; i < tangents.GetCount(); ++i)
            tangentStream->AddValue(tangents[i]);
        }
        if (!texcoords.IsEmpty())
        {
          VertexDataStream* texcoordStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::TexCoord0, 2);
          streams.PushBack(texcoordStream);
          texcoordStream->AddValues(texcoords);
        }

        ezArrayPtr<Mesh::Triangle> triangleList = mesh->GetTriangles();
        for (VertexDataStream* stream : streams)
        {
          ezUInt32 numComponents = stream->GetNumElementsPerVertex();
          for (ezUInt32 i = 0; i < triangleList.GetCount(); ++i)
          {
            stream->SetDataIndex(triangleList[i].m_Vertices[0], indices[i * 3] * numComponents);
            stream->SetDataIndex(triangleList[i].m_Vertices[1], indices[i * 3 + 1] * numComponents);
            stream->SetDataIndex(triangleList[i].m_Vertices[2], indices[i * 3 + 2] * numComponents);
          }
        }
      }
      else
      {
        ezLog::Warning("PBRT '{0}' shapes are not supported.", type.GetData());
        return;
      }

      if (mesh)
      {
        // Wire in last material. If active material is invalid, still do it to remove any reference to old scene.
        if (context.PeekActiveMaterial())
        {
          if (mesh->GetNumSubMeshes() == 0)
          {
            SubMesh submesh;
            submesh.m_Material = *context.PeekActiveMaterial();
            submesh.m_uiFirstTriangle = 0;
            submesh.m_uiTriangleCount = mesh->GetNumTriangles();
            mesh->AddSubMesh(submesh);
          }
          else
          {
            for (ezUInt32 i = 0; i < mesh->GetNumSubMeshes(); ++i)
              mesh->GetSubMesh(i).m_Material = *context.PeekActiveMaterial();
          }
        }
        else
        {
          for (ezUInt32 i = 0; i < mesh->GetNumSubMeshes(); ++i)
            mesh->GetSubMesh(i).m_Material = MaterialHandle();
        }

        // Add to output scene or object.
        if (Object* obj = context.GetActiveObject())
          obj->AddMesh(std::move(mesh));
        else
        {
          auto meshHandle = outScene.AddMesh(std::move(mesh));

          // Wire in last node.
          if (Node* parentNode = outScene.GetObject<Node>(parentNodeHandle))
          {
            parentNode->m_Children.PushBack(meshHandle);
          }
        }
      }
    }

    void ReadMaterialParameter(ParseContext& context, SemanticHint::Enum semantic, const char* materialParameter, ezModelImporter::Material& material, ezArrayPtr<Parameter> parameters, ezVariant default)
    {
      for (const Parameter& param : parameters)
      {
        if (!param.data.IsEmpty() && param.name.IsEqual_NoCase(materialParameter))
        {
          if (param.type == ParamType::TEXTURE)
          {
            ezString textureName = param.data[0].Get<ezString>();
            const char* textureFilename = context.LookUpTextureFilename(textureName);
            if (textureFilename)
              material.m_Textures.PushBack(TextureReference(semantic, ezString(param.name), textureFilename));
          }
          else
          {
            material.m_Properties.ExpandAndGetRef() = Property(semantic, materialParameter, param.data[0]);
          }
          return;
        }
      }

      if (default.IsValid())
        material.m_Properties.PushBack(Property(semantic, materialParameter, default));
    }

    ezUniquePtr<ezModelImporter::Material> PareMaterialImpl(ezStringView type, ezArrayPtr<Parameter> parameters, ParseContext& context, ezModelImporter::Scene& outScene)
    {
      ezUniquePtr<ezModelImporter::Material> newMaterial = EZ_DEFAULT_NEW(ezModelImporter::Material);

      newMaterial->m_Name = "";
      newMaterial->m_Properties.PushBack(Property("type", type));

      // http://www.pbrt.org/fileformat.html#materials
      if (type.IsEqual_NoCase("glass"))
      {
        ReadMaterialParameter(context, SemanticHint::REFLECTIVITY, "Kr", *newMaterial, parameters, 1.0f); // The reflectivity of the surface.
        ReadMaterialParameter(context, SemanticHint::OPACITY, "Kt", *newMaterial, parameters, 1.0f); // The transmissivity of the surface.
        ReadMaterialParameter(context, SemanticHint::REFRACTIONINDEX, "index", *newMaterial, parameters, 1.5f); // The index of refraction of the inside of the object. (pbrt implicitly assumes that the exterior of objects is a vacuum, with IOR of 1.)
      }
      else if (type.IsEqual_NoCase("KdSubsurface"))
      {
        ReadMaterialParameter(context, SemanticHint::DIFFUSE, "Kd", *newMaterial, parameters, 0.5f); // Diffuse scattering coefficient used to derive scattering properties.
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "meanfreepath", *newMaterial, parameters, 1.0f); // Average distance light travels in the medium before scattering.
        ReadMaterialParameter(context, SemanticHint::REFRACTIONINDEX, "index", *newMaterial, parameters, 1.0f); // The index of refraction inside the object.
        ReadMaterialParameter(context, SemanticHint::REFLECTIVITY, "Kr", *newMaterial, parameters, 1.0f); // Specular reflection term; this coefficient is modulated with the dielectric Fresnel equation to give the amount of specular reflection.
      }
      if (type.IsEqual_NoCase("matte"))
      {
        ReadMaterialParameter(context, SemanticHint::DIFFUSE, "Kd", *newMaterial, parameters, 0.5f); // The diffuse reflectivity of the surface.
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "sigma", *newMaterial, parameters, 0.0f); // The sigma parameter for the Oren-Nayar model, in degrees. If this is zero, the surface exhibits pure Lambertian reflection.
      }
      else if (type.IsEqual_NoCase("measured"))
      {
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "filename", *newMaterial, parameters, ezVariant()); // The diffuse reflectivity of the surface.
      }
      else if (type.IsEqual_NoCase("metal"))
      {
        ReadMaterialParameter(context, SemanticHint::REFRACTIONINDEX, "eta", *newMaterial, parameters, 0.5f); // Index of refraction to use in computing the material's reflectance.
        ReadMaterialParameter(context, SemanticHint::REFLECTIVITY, "k", *newMaterial, parameters, 0.5f); // Absorption coefficient to use in computing the material's reflectance.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "roughness", *newMaterial, parameters, 0.01f); // Roughness of the material's microfacet distribution. Smaller values become increasingly close to perfect specular reflection. This value should be between zero and one.
      }
      else if (type.IsEqual_NoCase("mirror"))
      {
        ReadMaterialParameter(context, SemanticHint::REFLECTIVITY, "Kr", *newMaterial, parameters, 0.5f); // The reflectivity of the mirror. This value can be used to make colored or dim reflections.
      }
      else if (type.IsEqual_NoCase("mixture"))
      {
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "amount", *newMaterial, parameters, 0.5f); // Weighting factor for the blend between materials. A value of zero corresponds to just "namedmaterial1", a value of one corredponds to just "namedmaterial2", and values in between interpolate linearly.
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "namedmaterial1", *newMaterial, parameters, ezVariant()); // Name of first material to be interpolated between.
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "namedmaterial2", *newMaterial, parameters, ezVariant()); // Name of second material to be interpolated between.
      }
      else if (type.IsEqual_NoCase("plastic"))
      {
        ReadMaterialParameter(context, SemanticHint::DIFFUSE, "Kd", *newMaterial, parameters, 0.25f); // The diffuse reflectivity of the surface.
        ReadMaterialParameter(context, SemanticHint::METALLIC, "Ks", *newMaterial, parameters, 0.25f); // The specular reflectivity of the surface.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "roughness", *newMaterial, parameters, 0.1f); // The roughness of the surface, from 0 to 1. Larger values result in larger, more blurry highlights.
      }
      else if (type.IsEqual_NoCase("shinymetal"))
      {
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "roughness", *newMaterial, parameters, 0.1f); // The roughness of the surface.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "Ks", *newMaterial, parameters, 1.0f); // The coefficient of glossy reflection.
        ReadMaterialParameter(context, SemanticHint::METALLIC, "Kr", *newMaterial, parameters, 1.0f); // The coefficient of specular reflection.
      }
      else if (type.IsEqual_NoCase("substrate"))
      {
        ReadMaterialParameter(context, SemanticHint::DIFFUSE, "Kd", *newMaterial, parameters, 0.5f); // The coefficient of diffuse reflection.
        ReadMaterialParameter(context, SemanticHint::METALLIC, "Ks", *newMaterial, parameters, 0.5f); // The coefficient of specular reflection.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "uroughness", *newMaterial, parameters, 0.1f); // The roughness of the surface in the u direction.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "vroughness", *newMaterial, parameters, 0.1f); // The roughness of the surface in the v direction.
      }
      else if (type.IsEqual_NoCase("subsurface"))
      {
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "name", *newMaterial, parameters, ezVariant()); // Name of measured subsurface scattering coefficients. See the file src/core/volume.cpp in the pbrt distribution for all of the measurements that are available.
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "sigma_a", *newMaterial, parameters, ezVec3(0.0011f, 0.0024f, 0.014f)); // Absorption coefficient of the volume, measured in mm^-1.
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "sigma_prime_s", *newMaterial, parameters, ezVec3(2.55f, 3.12f, 3.77f)); // Reduced scattering coefficient of the volume, measured in mm^-1.
        ReadMaterialParameter(context, SemanticHint::UNKNOWN, "scale", *newMaterial, parameters, 1.0f); // Scale factor that is applied to sigma_a and sigma_prime_s. This is particularly useful when the scene is not measured in mm and the coefficients need to be scaled accordingly. For example, if the scene is modeled in meters, then a scale factor of 0.001 would be appropriate.
        ReadMaterialParameter(context, SemanticHint::REFRACTIONINDEX, "index", *newMaterial, parameters, 1.3f); // Index of refraction of the scattering volume.
      }
      else if (type.IsEqual_NoCase("translucent"))
      {
        ReadMaterialParameter(context, SemanticHint::DIFFUSE, "Kd", *newMaterial, parameters, 0.25f); // The coefficient of diffuse reflection and transmission.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "Ks", *newMaterial, parameters, 0.25f); // The coefficient of specular reflection and transmission.
        ReadMaterialParameter(context, SemanticHint::METALLIC, "reflect", *newMaterial, parameters, 0.5f); // Fraction of light reflected.
        ReadMaterialParameter(context, SemanticHint::OPACITY, "transmit", *newMaterial, parameters, 0.5f); // Fraction of light transmitted.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "roughness", *newMaterial, parameters, 0.1f); // The roughness of the surface. (This value should be between 0 and 1).
      }
      else if (type.IsEqual_NoCase("uber"))
      {
        ReadMaterialParameter(context, SemanticHint::DIFFUSE, "Kd", *newMaterial, parameters, 0.25f); // The coefficient of diffuse reflection.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "Ks", *newMaterial, parameters, 0.25f); // The coefficient of glossy reflection.
        ReadMaterialParameter(context, SemanticHint::METALLIC, "Kr", *newMaterial, parameters, 0.25f); // The coefficient of specular reflection.
        ReadMaterialParameter(context, SemanticHint::ROUGHNESS, "roughness", *newMaterial, parameters, 0.1f); // The roughness of the surface.
        ReadMaterialParameter(context, SemanticHint::REFRACTIONINDEX, "index", *newMaterial, parameters, 0.1f); // Index of refraction of the surface. This value is used in both the microfacet model for specular reflection as well as for computing a Fresnel reflection term for perfect specular reflection.
        ReadMaterialParameter(context, SemanticHint::OPACITY, "opacity", *newMaterial, parameters, 1.0f); // The opacity of the surface.Note that when less than one, the uber material transmits light without refracting it.
      }

      // Put in all parameters not loaded yet.
      for (Parameter& param : parameters)
      {
        // More general semantic hint mapping.
        SemanticHint::Enum semanticHint = SemanticHint::UNKNOWN;
        if(param.name.IsEqual_NoCase("kd")) semanticHint = SemanticHint::DIFFUSE;
        else if (param.name.IsEqual_NoCase("ks")) semanticHint = SemanticHint::ROUGHNESS;
        else if (param.name.IsEqual_NoCase("kr")) semanticHint = SemanticHint::METALLIC;
        else if (param.name.IsEqual_NoCase("bumpmap")) semanticHint = SemanticHint::NORMAL;

        bool found = false;
        if (param.type == ParamType::TEXTURE)
        {
          for (ezUInt32 i = 0; i < newMaterial->m_Textures.GetCount(); ++i)
          {
            if (param.name.IsEqual_NoCase(newMaterial->m_Textures[i].m_Semantic.GetData()))
            {
              found = true;
              break;
            }
          }
          if (!found)
          {
            ezString textureName = param.data[0].Get<ezString>();
            const char* textureFilename = context.LookUpTextureFilename(textureName);
            if (textureFilename)
              newMaterial->m_Textures.PushBack(TextureReference(semanticHint, ezString(param.name), textureFilename));
          }
        }
        else
        {
          for (ezUInt32 i = 0; i < newMaterial->m_Properties.GetCount(); ++i)
          {
            if (param.name.IsEqual_NoCase(newMaterial->m_Properties[i].m_Semantic.GetData()))
            {
              found = true;
              break;
            }
          }
          if (!found)
          {
            newMaterial->m_Properties.PushBack(Property(semanticHint, ezString(param.name), param.data[0]));
          }
        }
      }

      return std::move(newMaterial);
    }

    void Material(ezStringView type, ezArrayPtr<Parameter> parameters, ParseContext& context, ezModelImporter::Scene& outScene)
    {
      ezUniquePtr<ezModelImporter::Material> newMaterial = PareMaterialImpl(type, parameters, context, outScene);
      context.PushActiveMaterial(outScene.AddMaterial(std::move(newMaterial)));
    }

    void MakeNamedMaterial(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene)
    {
      // "type" is name and the first parameter is the type.
      ezString name = type;
      ezString materialType;
      if (parameters.IsEmpty() || parameters[0].type != ParamType::STRING ||
        parameters[0].data.IsEmpty() || parameters[0].name.Compare_NoCase("type"))
      {
        ezLog::WarningPrintf("PBRT make named material should have a type parameter.", type.GetData()); // This sometimes happens and need to handle that.
      }
      else
      {
        materialType = parameters[0].data[0].Get<ezString>();
        parameters = parameters.GetSubArray(1);
      }

      ezUniquePtr<ezModelImporter::Material> newMaterial = PareMaterialImpl(materialType, parameters, context, outScene);
      newMaterial->m_Name = name;
      context.AddNamedMaterial(name, outScene.AddMaterial(std::move(newMaterial)));
    }

    void NamedMaterial(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene)
    {
      ezString materialName(type);
      if (context.MakeNamedMaterialActive(materialName).Failed())
      {
        ezLog::Error("PBRT make 'NamedMaterial' material name '{0}' is not known.", materialName.GetData());
      }
    }

    void Texture(ezStringView type, ezArrayPtr<Parameter> parameters, ParseContext& context, ezModelImporter::Scene& outScene)
    {
      // http://www.pbrt.org/fileformat.html#textures

      // Type is actually name.
      // Second parameter is implementation class. We can't map that.
      // All other parameters don't matter either, only the "filename" parameter, without that the texture is useless to us.

      ezString filename;
      for(const Parameter& param : parameters)
      {
        if (param.name.IsEqual_NoCase("filename"))
        {
          if (param.type != ParamType::STRING || param.data.GetCount() != 1)
          {
            ezLog::Error("Texture's filename parameter is not a string and/or does not have one value.");
            continue;
          }

          filename = param.data[0].Get<ezString>();
          break;
        }

        // Can't load "scale" and similar texture correctly, but if they contain a txture reference, we can try to use that.
        else if ((param.name.IsEqual_NoCase("tex1") || param.name.IsEqual_NoCase("tex2")) && param.type == ParamType::TEXTURE && !param.data.IsEmpty())
        {
          filename = context.LookUpTextureFilename(param.data[0].Get<ezString>());
        }
      }

      if (filename.IsEmpty())
      {
        ezLog::Warning("Texture '{0}' does not have a filename, unable to import.", type);
      }
      else
      {
        context.AddTexture(ezString(type), filename);
      }
    }

    void ObjectBegin(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene)
    {
      ezString objectName(type);
      if (parameters.GetCount() != 0)
        ezLog::Warning("Expected 0 parameters for ObjectBegin command (name '{0}').", objectName);

      context.ObjectBegin(objectName);
    }

    void ObjectInstance(ezStringView type, ezArrayPtr<Pbrt::Parameter> parameters, Pbrt::ParseContext& context, ezModelImporter::Scene& outScene)
    {
      ezString objectName(type);
      if (parameters.GetCount() != 0)
        ezLog::Warning("Expected 0 parameters for ObjectInstance command (name '{0}').", objectName);

      Object* object = context.LookUpObject(objectName);
      if (!object)
      {
        ezLog::Error("Can't instantiate object: No object with name '{0}' known.", objectName);
        return;
      }

      ezArrayPtr<ObjectHandle> meshes = object->InstantiateMeshes(outScene);
      ezTransform transform;

      auto nodeHandle = outScene.AddNode(EZ_DEFAULT_NEW(Node));
      Node* node = outScene.GetObject<Node>(nodeHandle);
      node->m_Name = objectName;
      node->m_RelativeTransform.SetGlobalTransform(context.PeekActiveTransform(), object->m_Transform);
      for (ObjectHandle meshHandle : meshes)
      {
        node->m_Children.PushBack(meshHandle);
      }
    }
  }
}
