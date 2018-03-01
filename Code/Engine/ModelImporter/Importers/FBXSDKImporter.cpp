#include <PCH.h>
#include <ModelImporter/Importers/FBXSDKImporter.h>
#include <ModelImporter/Scene.h>
#include <ModelImporter/Node.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/VertexData.h>
#include <ModelImporter/Material.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/DynamicArray.h>

#if defined(BUILDSYSTEM_BUILD_WITH_OFFICIAL_FBX_SDK)
#include <fbxsdk.h>
using namespace fbxsdk;
#endif

namespace ezModelImporter
{
#if defined(BUILDSYSTEM_BUILD_WITH_OFFICIAL_FBX_SDK)

  struct FBXImportContext
  {
    FbxImporter* pImporter;
    FbxScene* pScene;
    ezSharedPtr<Scene> pOutScene;
    ezMap<FbxSurfaceMaterial*, MaterialHandle> fbxMaterialsToEz;
    ezMap<FbxMesh*, ObjectHandle> processedMeshes;
  };

  ezVec3 ConvertFromFBX(const FbxDouble3& fbxDouble3)
  {
    return ezVec3(static_cast<float>(fbxDouble3.mData[0]), static_cast<float>(fbxDouble3.mData[1]), static_cast<float>(fbxDouble3.mData[2]));
  }

  ezVec4 ConvertFromFBX(const FbxVector4& fbxVec4)
  {
    return ezVec4(static_cast<float>(fbxVec4.mData[0]), static_cast<float>(fbxVec4.mData[1]), static_cast<float>(fbxVec4.mData[2]), static_cast<float>(fbxVec4.mData[3]));
  }

  ezVec4 ConvertFromFBX(const FbxColor& fbxColor)
  {
    return ezVec4(static_cast<float>(fbxColor.mRed), static_cast<float>(fbxColor.mGreen), static_cast<float>(fbxColor.mBlue), static_cast<float>(fbxColor.mAlpha));
  }

  ezVec2 ConvertFromFBX(const FbxVector2& fbxVec2)
  {
    return ezVec2(static_cast<float>(fbxVec2.mData[0]), static_cast<float>(fbxVec2.mData[1]));
  }

  ezQuat ConvertFromFBX(const FbxQuaternion& fbxQuat)
  {
    return ezQuat(static_cast<float>(fbxQuat.mData[0]), static_cast<float>(fbxQuat.mData[1]), static_cast<float>(fbxQuat.mData[2]), static_cast<float>(fbxQuat.mData[3]));
  }

  // Helper structure, due to the nature of FBX mapping the elements of vertices potentially
  // differently we build a list of unique vertices and de-duplicate them later to build a properly
  // indexed mesh
  struct FBXVertex
  {
    EZ_DECLARE_POD_TYPE();

    FBXVertex()
      : pos(ezVec3::ZeroVector())
      , normal(ezVec3::ZeroVector())
      , tangent(ezVec3::ZeroVector())
      , binormal(ezVec3::ZeroVector())
      , uv0(ezVec2::ZeroVector())
      , uv1(ezVec2::ZeroVector())
      , color(1, 1, 1, 1)
    {}

    bool operator == (const FBXVertex& other) const
    {
      return ezMemoryUtils::ByteCompare(this, &other) == 0;
    }

    ezVec3 pos;
    ezVec3 normal;
    ezVec3 tangent;
    ezVec3 binormal;
    ezVec2 uv0;
    ezVec2 uv1;
    ezVec4 color;
  };

  void TryAddTextureReference(fbxsdk::FbxSurfaceMaterial* pMaterial, const char* szFBXName, SemanticHint::Enum semanticHint, const char* szSemantic, ezModelImporter::Material* pMat)
  {
    FbxProperty pProperty = pMaterial->FindProperty(szFBXName);
    if (int iNumTextures = pProperty.GetSrcObjectCount<FbxTexture>())
    {
      // Note that this only uses the first texture if existent
      FbxTexture* pTexture = pProperty.GetSrcObject<FbxTexture>(0);

      if (pTexture->GetClassId().Is(FbxFileTexture::ClassId))
      {
        FbxFileTexture* pFileTexture = static_cast<FbxFileTexture*>(pTexture);
        TextureReference& textureReference = pMat->m_Textures.ExpandAndGetRef();
        textureReference.m_SemanticHint = semanticHint;
        textureReference.m_Semantic = szSemantic;
        textureReference.m_FileName = pFileTexture->GetFileName();
      }
    }
  }

  ObjectHandle ImportMesh(FbxMesh* pMesh, FBXImportContext& ImportContext)
  {
    if (ImportContext.processedMeshes.Find(pMesh).IsValid())
    {
      return ImportContext.processedMeshes[pMesh];
    }

    if (!pMesh->IsTriangleMesh())
    {
      ezLog::Error("FBX mesh '{0}' is not triangulated. Select \"Triangulate\" during export.", pMesh->GetName());
      return ObjectHandle();
    }

    // Import materials
    {
      FbxNode* pNode = pMesh->GetNode();

      const int iMaterialCount = pNode->GetMaterialCount();
      for (int iMat = 0; iMat < iMaterialCount; ++iMat)
      {
        auto pMaterial = pNode->GetMaterial(iMat);

        if (ImportContext.fbxMaterialsToEz.Contains(pMaterial))
          continue;

        ezUniquePtr<ezModelImporter::Material> newMat(EZ_DEFAULT_NEW(ezModelImporter::Material));
        newMat->m_Name = pMaterial->GetName();

        if (pMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
        {
          auto pPhongMaterial = static_cast<FbxSurfacePhong*>(pMaterial);

          {
            Property& materialProperty = newMat->m_Properties.ExpandAndGetRef();
            materialProperty.m_Semantic = "DiffuseColor";
            materialProperty.m_SemanticHint = SemanticHint::DIFFUSE;
            ezVec3 color = ConvertFromFBX(pPhongMaterial->Diffuse.Get());
            materialProperty.m_Value = ezColor(color.x, color.y, color.z, 1.0f);
          }

          {
            Property& materialProperty = newMat->m_Properties.ExpandAndGetRef();
            materialProperty.m_Semantic = "EmissiveColor";
            materialProperty.m_SemanticHint = SemanticHint::EMISSIVE;
            ezVec3 color = ConvertFromFBX(pPhongMaterial->Emissive.Get());
            float factor = static_cast<float>(pPhongMaterial->EmissiveFactor.Get());
            materialProperty.m_Value = ezColor(color.x * factor, color.y * factor, color.z * factor, 1.0f);
          }

          {
            Property& materialProperty = newMat->m_Properties.ExpandAndGetRef();
            materialProperty.m_Semantic = "Opacity";
            materialProperty.m_SemanticHint = SemanticHint::OPACITY;
            float transparency = static_cast<float>(pPhongMaterial->TransparencyFactor.Get());
            materialProperty.m_Value = transparency;
          }

          {
            Property& materialProperty = newMat->m_Properties.ExpandAndGetRef();
            materialProperty.m_Semantic = "Roughness";
            materialProperty.m_SemanticHint = SemanticHint::ROUGHNESS;
            float shininess = static_cast<float>(pPhongMaterial->Shininess.Get());
            materialProperty.m_Value = 1.0f - shininess;
          }

          TryAddTextureReference(pMaterial, fbxsdk::FbxSurfaceMaterial::sDiffuse, SemanticHint::DIFFUSE, "Diffuse", newMat.Borrow());
          TryAddTextureReference(pMaterial, fbxsdk::FbxSurfaceMaterial::sEmissive, SemanticHint::EMISSIVE, "Emissive", newMat.Borrow());
          TryAddTextureReference(pMaterial, fbxsdk::FbxSurfaceMaterial::sNormalMap, SemanticHint::NORMAL, "Normals", newMat.Borrow());
        }
        else if (pMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
        {
          auto pLambertMaterial = static_cast<FbxSurfaceLambert*>(pMaterial);

          {
            Property& materialProperty = newMat->m_Properties.ExpandAndGetRef();
            materialProperty.m_Semantic = "DiffuseColor";
            materialProperty.m_SemanticHint = SemanticHint::DIFFUSE;
            ezVec3 color = ConvertFromFBX(pLambertMaterial->Diffuse.Get());
            materialProperty.m_Value = ezColor(color.x, color.y, color.z, 1.0f);
          }

          {
            Property& materialProperty = newMat->m_Properties.ExpandAndGetRef();
            materialProperty.m_Semantic = "EmissiveColor";
            materialProperty.m_SemanticHint = SemanticHint::EMISSIVE;
            ezVec3 color = ConvertFromFBX(pLambertMaterial->Emissive.Get());
            float factor = static_cast<float>(pLambertMaterial->EmissiveFactor.Get());
            materialProperty.m_Value = ezColor(color.x * factor, color.y * factor, color.z * factor, 1.0f);
          }

          {
            Property& materialProperty = newMat->m_Properties.ExpandAndGetRef();
            materialProperty.m_Semantic = "Opacity";
            materialProperty.m_SemanticHint = SemanticHint::OPACITY;
            float transparency = static_cast<float>(pLambertMaterial->TransparencyFactor.Get());
            materialProperty.m_Value = transparency;
          }

          TryAddTextureReference(pMaterial, fbxsdk::FbxSurfaceMaterial::sDiffuse, SemanticHint::DIFFUSE, "Diffuse", newMat.Borrow());
          TryAddTextureReference(pMaterial, fbxsdk::FbxSurfaceMaterial::sEmissive, SemanticHint::EMISSIVE, "Emissive", newMat.Borrow());
          TryAddTextureReference(pMaterial, fbxsdk::FbxSurfaceMaterial::sNormalMap, SemanticHint::NORMAL, "Normals", newMat.Borrow());
        }
        else
        {
          ezLog::Warning("Unknown material type for material '{0}'", pMaterial->GetName());
        }

        ImportContext.fbxMaterialsToEz.Insert(pMaterial, ImportContext.pOutScene->AddMaterial(std::move(newMat)));
      }
    }

    // Establish the material mapping type
    MaterialHandle AllSameMaterial;

    for (int iMatElement = 0; iMatElement < pMesh->GetElementMaterialCount(); ++iMatElement)
    {
      FbxGeometryElementMaterial* pMaterialElement = pMesh->GetElementMaterial(iMatElement);

      switch (pMaterialElement->GetMappingMode())
      {
      case FbxGeometryElement::eAllSame:
        {
          FbxSurfaceMaterial* pMaterial = pMesh->GetNode()->GetMaterial(pMaterialElement->GetIndexArray().GetAt(0));

          if (!ImportContext.fbxMaterialsToEz.Contains(pMaterial))
          {
            ezLog::Warning("FBX material mapping type is set to eAllSame but the referenced material wasn't imported. Assigning default material.");
          }

          AllSameMaterial = *ImportContext.fbxMaterialsToEz.GetValue(pMaterial);
        }
        break;

      case FbxGeometryElement::eByPolygon:
        {
          // Mapping is handled with sub mesh creation
        }
        break;

      default:
        {
          ezLog::Warning("FBX import encountered unsupported material mapping mode, currently supported are: eAllSame, eByPolygon.");
        }
      }
    }

    ezUniquePtr<Mesh> mesh = EZ_DEFAULT_NEW(Mesh);

    const int iPolygonCount = pMesh->GetPolygonCount();
    const FbxVector4* pVertexPositions = pMesh->GetControlPoints();
    const int iVertexCount = pMesh->GetControlPointsCount();

    const bool bHasUV0 = pMesh->GetElementUVCount() > 0;
    const bool bHasUV1 = pMesh->GetElementUVCount() > 1;
    const bool bHasColor0 = pMesh->GetElementVertexColorCount() > 0;
    const bool bHasNormal = pMesh->GetElementNormalCount() > 0;
    const bool bHasTangent = pMesh->GetElementTangentCount() > 0;
    const bool bHasBinormal = pMesh->GetElementBinormalCount() > 0;
    const bool bHasPolygonGroups = pMesh->GetElementPolygonGroupCount() > 0;


    ezDynamicArray<FBXVertex> uniqueVertices;
    uniqueVertices.Reserve(static_cast<ezUInt32>(iPolygonCount) * 3);

    int vertexId = 0;

    for (int poly = 0; poly < iPolygonCount; ++poly)
    {
      const int iPolygonSize = pMesh->GetPolygonSize(poly);
      EZ_ASSERT_DEV(iPolygonSize == 3, "FBX importer only works on triangulated meshes.");

      for (int polyVertex = 0; polyVertex < iPolygonSize; ++polyVertex)
      {
        int iIndex = pMesh->GetPolygonVertex(poly, polyVertex);

        FBXVertex vertex;
        vertex.pos = ConvertFromFBX(pMesh->GetControlPointAt(iIndex)).GetAsVec3();

        if (bHasNormal)
        {
          FbxGeometryElementNormal* pNormalElement = pMesh->GetElementNormal(0);
          if (pNormalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
          {
            switch (pNormalElement->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
              vertex.normal = ConvertFromFBX(pNormalElement->GetDirectArray().GetAt(vertexId)).GetAsVec3();
              break;

            case FbxGeometryElement::eIndexToDirect:
              {
                int id = pNormalElement->GetIndexArray().GetAt(vertexId);
                vertex.normal = ConvertFromFBX(pNormalElement->GetDirectArray().GetAt(id)).GetAsVec3();
              }
              break;

            default:
              break; // other reference modes not shown here!
            }
          }
        }

        if (bHasTangent)
        {
          FbxGeometryElementTangent* pTangentElement = pMesh->GetElementTangent(0);
          if (pTangentElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
          {
            switch (pTangentElement->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
              vertex.tangent = ConvertFromFBX(pTangentElement->GetDirectArray().GetAt(vertexId)).GetAsVec3();
              break;

            case FbxGeometryElement::eIndexToDirect:
              {
                int id = pTangentElement->GetIndexArray().GetAt(vertexId);
                vertex.tangent = ConvertFromFBX(pTangentElement->GetDirectArray().GetAt(id)).GetAsVec3();
              }
              break;

            default:
              break; // other reference modes not shown here!
            }
          }
        }

        if (bHasBinormal)
        {
          FbxGeometryElementBinormal* pBinormalElement = pMesh->GetElementBinormal(0);
          if (pBinormalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
          {
            switch (pBinormalElement->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
              vertex.binormal = ConvertFromFBX(pBinormalElement->GetDirectArray().GetAt(vertexId)).GetAsVec3();
              break;

            case FbxGeometryElement::eIndexToDirect:
              {
                int id = pBinormalElement->GetIndexArray().GetAt(vertexId);
                vertex.binormal = ConvertFromFBX(pBinormalElement->GetDirectArray().GetAt(id)).GetAsVec3();
              }
              break;

            default:
              break; // other reference modes not shown here!
            }
          }
        }

        // Get UV coordinates, this imports at maximum UV0 and UV1
        for (int uvElement = 0; uvElement < pMesh->GetElementUVCount() && uvElement < 2; ++uvElement)
        {
          FbxGeometryElementUV* pUVElement = pMesh->GetElementUV(uvElement);

          ezVec2 uv;
          uv.SetZero();

          switch (pUVElement->GetMappingMode())
          {
          case FbxGeometryElement::eByControlPoint:
            switch (pUVElement->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
              uv = ConvertFromFBX(pUVElement->GetDirectArray().GetAt(iIndex));
              break;
            case FbxGeometryElement::eIndexToDirect:
              {
                int id = pUVElement->GetIndexArray().GetAt(iIndex);
                uv = ConvertFromFBX(pUVElement->GetDirectArray().GetAt(id));
              }
              break;
            default:
              break; // other reference modes not implemented yet
            }
            break;

          case FbxGeometryElement::eByPolygonVertex:
            {
              int textureUVIndex = pMesh->GetTextureUVIndex(poly, polyVertex);
              switch (pUVElement->GetReferenceMode())
              {
              case FbxGeometryElement::eDirect:
              case FbxGeometryElement::eIndexToDirect:
                {
                  uv = ConvertFromFBX(pUVElement->GetDirectArray().GetAt(textureUVIndex));
                }
                break;
              default:
                break; // other reference modes not shown here!
              }
            }
            break;

          case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
          case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
          case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
            break;
          default:
            break;
          }

          // make the V coordinate conform with the expected convention
          // TODO: I could not find a flag in the FBX SDK that defines the V coordinate origin
          uv.y = 1.0f - uv.y;

          switch (uvElement)
          {
          case 0:
            vertex.uv0 = uv;
            break;
          case 1:
            vertex.uv1 = uv;
            break;
          default:
            break;
          }
        }

        if (bHasColor0)
        {
          FbxGeometryElementVertexColor* pVertexColorElement = pMesh->GetElementVertexColor(0);

          switch (pVertexColorElement->GetMappingMode())
          {
          default:
            break;
          case FbxGeometryElement::eByControlPoint:
            switch (pVertexColorElement->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
              vertex.color = ConvertFromFBX(pVertexColorElement->GetDirectArray().GetAt(iIndex));
              break;
            case FbxGeometryElement::eIndexToDirect:
              {
                int id = pVertexColorElement->GetIndexArray().GetAt(iIndex);
                vertex.color = ConvertFromFBX(pVertexColorElement->GetDirectArray().GetAt(id));
              }
              break;
            default:
              break; // other reference modes not implemented
            }
            break;

          case FbxGeometryElement::eByPolygonVertex:
            {
              switch (pVertexColorElement->GetReferenceMode())
              {
              case FbxGeometryElement::eDirect:
                vertex.color = ConvertFromFBX(pVertexColorElement->GetDirectArray().GetAt(vertexId));
                break;
              case FbxGeometryElement::eIndexToDirect:
                {
                  int id = pVertexColorElement->GetIndexArray().GetAt(vertexId);
                  vertex.color = ConvertFromFBX(pVertexColorElement->GetDirectArray().GetAt(id));
                }
                break;
              default:
                break; // other reference modes not shown here!
              }
            }
            break;

          case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
          case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
          case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
            break;
          }
        }

        vertexId++;
        uniqueVertices.PushBack(vertex);
      }
    }

    // De-duplicate the vertices to build a proper indexed mesh
    // This might require speed up for large models since this is quadratic in complexity
    ezDynamicArray<FBXVertex> deduplicatedVertices; deduplicatedVertices.Reserve(uniqueVertices.GetCount() / 2);
    ezDynamicArray<ezUInt32> indices; indices.Reserve(uniqueVertices.GetCount());

    for (const auto& vertex : uniqueVertices)
    {
      // TODO: Unfortunately it is a problem to deduplicate vertices this way:
      // when data streams such as normals, tangents etc. are not available, or contain garbage data (normals are all zero, this happens)
      // it will deduplicate vertices that must not be merged, ie. from triangles that face into different directions (e.g. double-sided geometry)
      // Additionally, even if the data is 'good', but the user enabled 'recalculate normals', we must not deduplicate this BEFORE we have the final,
      // recomputed data, otherwise we merge vertices into one, that may be different after the recomputation
      //
      // since this breaks some meshes, I deactivated deduplication for now

      ezUInt32 deduplicatedIndex = ezInvalidIndex;// deduplicatedVertices.IndexOf(vertex);
      if (deduplicatedIndex != ezInvalidIndex)
      {
        indices.PushBack(deduplicatedIndex);
      }
      else
      {
        deduplicatedVertices.PushBack(vertex);
        indices.PushBack(deduplicatedVertices.GetCount() - 1);
      }
    }


    ezHybridArray<ezModelImporter::VertexDataStream*, 8> streams;

    VertexDataStream* positionDataStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::Position, 3, VertexElementType::FLOAT);
    streams.PushBack(positionDataStream);

    VertexDataStream* normalDataStream = nullptr;
    if (bHasNormal)
    {
      normalDataStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::Normal, 3, VertexElementType::FLOAT);
      streams.PushBack(normalDataStream);
    }

    VertexDataStream* tangentDataStream = nullptr;
    if (bHasTangent)
    {
      tangentDataStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::Tangent, 3, VertexElementType::FLOAT);
      streams.PushBack(tangentDataStream);
    }

    VertexDataStream* bitangentDataStream = nullptr;
    if (bHasBinormal)
    {
      bitangentDataStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::BiTangent, 3, VertexElementType::FLOAT);
      streams.PushBack(bitangentDataStream);
    }

    VertexDataStream* uv0DataStream = nullptr;
    if (bHasUV0)
    {
      uv0DataStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::TexCoord0, 2, VertexElementType::FLOAT);
      streams.PushBack(uv0DataStream);
    }

    VertexDataStream* uv1DataStream = nullptr;
    if (bHasUV1)
    {
      uv1DataStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::TexCoord1, 2, VertexElementType::FLOAT);
      streams.PushBack(uv1DataStream);
    }

    VertexDataStream* colorDataStream = nullptr;
    if (bHasColor0)
    {
      colorDataStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::Color, 4, VertexElementType::FLOAT);
      streams.PushBack(colorDataStream);
    }

    for (auto& vertex : deduplicatedVertices)
    {
      positionDataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.pos.x), 3 * sizeof(float)));

      if (bHasNormal)
      {
        normalDataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.normal.x), 3 * sizeof(float)));
      }

      if (bHasTangent)
      {
        tangentDataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.tangent.x), 3 * sizeof(float)));
      }

      if (bHasBinormal)
      {
        bitangentDataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.binormal.x), 3 * sizeof(float)));
      }

      if (bHasUV0)
      {
        uv0DataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.uv0.x), 2 * sizeof(float)));
      }

      if (bHasUV1)
      {
        uv1DataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.uv1.x), 2 * sizeof(float)));
      }

      if (bHasColor0)
      {
        colorDataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.color.x), 4 * sizeof(float)));
      }
    }

    // Material assignment, easy case: all polygons share the same material. Take a shortcut and add a single submesh.
    if (!AllSameMaterial.IsInvalidated())
    {
      SubMesh subMesh;
      subMesh.m_Material = AllSameMaterial;
      subMesh.m_uiFirstTriangle = 0;
      subMesh.m_uiTriangleCount = static_cast<ezUInt32>(iPolygonCount);

      mesh->AddSubMesh(subMesh);
    }
    else
    {
      // Create a sub mesh for each polygon using the material mapping
      // provided by the FBX

      FbxGeometryElementMaterial* pMaterialElement = pMesh->GetElementMaterial(0);

      for (int poly = 0; poly < iPolygonCount; ++poly)
      {
        const int iPolygonSize = pMesh->GetPolygonSize(poly);
        EZ_ASSERT_ALWAYS(iPolygonSize == 3, "FBX importer only works on triangulated meshes.");

        // Get the correct material handle
        MaterialHandle materialHandle;
        if (pMaterialElement)
        {
          auto pFBXMat = pMesh->GetNode()->GetMaterial(pMaterialElement->GetIndexArray().GetAt(poly));

          if (auto pMatHandle = ImportContext.fbxMaterialsToEz.GetValue(pFBXMat))
          {
            materialHandle = *pMatHandle;
          }
        }

        SubMesh subMesh;
        subMesh.m_Material = materialHandle;
        subMesh.m_uiFirstTriangle = static_cast<ezUInt32>(poly);
        subMesh.m_uiTriangleCount = 1;

        mesh->AddSubMesh(subMesh);
      }
    }

    // Add indices
    mesh->AddTriangles(indices.GetCount() / 3);

    ezArrayPtr<ezModelImporter::Mesh::Triangle> triangleList = mesh->GetTriangles();
    for (ezModelImporter::VertexDataStream* stream : streams)
    {
      ezUInt32 uiAttributeSize = stream->GetAttributeSize();
      for (ezUInt32 i = 0; i < triangleList.GetCount(); ++i)
      {
        stream->SetDataIndex(triangleList[i].m_Vertices[0], indices[i * 3 + 0] * uiAttributeSize);
        stream->SetDataIndex(triangleList[i].m_Vertices[1], indices[i * 3 + 1] * uiAttributeSize);
        stream->SetDataIndex(triangleList[i].m_Vertices[2], indices[i * 3 + 2] * uiAttributeSize);
      }
    }

    // If we built a submesh per polygon to account for the material mapping
    // we want to merge them to reduce the number of submeshes.
    if (AllSameMaterial.IsInvalidated())
    {
      mesh->MergeSubMeshesWithSameMaterials();
    }

    auto meshHandle = ImportContext.pOutScene->AddMesh(std::move(mesh));
    ImportContext.processedMeshes[pMesh] = meshHandle;

    return meshHandle;
  }

  ObjectHandle ImportNodeRecursive(FbxNode* pNode, FBXImportContext& ImportContext)
  {
    ezUniquePtr<Node> node = EZ_DEFAULT_NEW(Node);

    node->m_Name = pNode->GetName();

    {
      FbxAMatrix localTransform = pNode->EvaluateLocalTransform();

      node->m_RelativeTransform.m_vPosition = ConvertFromFBX(localTransform.GetT()).GetAsVec3();
      node->m_RelativeTransform.m_vScale = ConvertFromFBX(localTransform.GetS()).GetAsVec3();
      node->m_RelativeTransform.m_qRotation = ConvertFromFBX(localTransform.GetQ());

      // for inexplicable reasons, EvaluateLocalTransform returns a different (correct) value,
      // than what is stored in pNode->LclRotation, so don't try to use that
    }

    for (int i = 0; i < pNode->GetChildCount(); ++i)
    {
      node->m_Children.PushBack(ImportNodeRecursive(pNode->GetChild(i), ImportContext));
    }

    if (auto* pNodeAttribute = pNode->GetNodeAttribute())
    {
      if (pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
      {
        ObjectHandle meshHandle = ImportMesh(static_cast<FbxMesh*>(pNodeAttribute), ImportContext);
        if (!meshHandle.IsValid())
        {
          ezLog::Error("Couldn't import mesh for node '{0}'", pNode->GetName());
          return ObjectHandle();
        }

        node->m_Children.PushBack(meshHandle);
      }
    }

    return ImportContext.pOutScene->AddNode(std::move(node));
  }



  FBXSDKImporter::FBXSDKImporter()
  {
    m_supportedFileFormats.PushBack("fbx");

    // Create the FBX manager object
    m_pFBXManager = FbxManager::Create();
    if (!m_pFBXManager)
    {
      ezLog::Error("Couldn't create FBX SDK manager. FBX import will not be available.");
    }
    else
    {
      // Set I/O (import & export settings) object
      FbxIOSettings* ios = FbxIOSettings::Create(m_pFBXManager, IOSROOT);
      m_pFBXManager->SetIOSettings(ios);

      // Load available plugins
      FbxString lPath = FbxGetApplicationDirectory();
      ezStringBuilder sFbxPluginDir = lPath.Buffer();
      sFbxPluginDir.AppendPath("FbxPlugins");
      m_pFBXManager->LoadPluginsDirectory(sFbxPluginDir.GetData());
    }
  }

  FBXSDKImporter::~FBXSDKImporter()
  {
    if (m_pFBXManager)
    {
      m_pFBXManager->Destroy();
      m_pFBXManager = nullptr;
    }
  }

  ezArrayPtr<const ezString> FBXSDKImporter::GetSupportedFileFormats() const
  {
    return ezMakeArrayPtr(m_supportedFileFormats);
  }

  ezSharedPtr<Scene> FBXSDKImporter::ImportScene(const char* szFileName)
  {
    if (!m_pFBXManager)
    {
      ezLog::SeriousWarning("Can't import '{0}' since no FBX manager is available.", szFileName);
      return nullptr;
    }

    FbxImporter* pImporter = FbxImporter::Create(m_pFBXManager, szFileName);
    EZ_SCOPE_EXIT(if (pImporter) { pImporter->Destroy(); pImporter = nullptr; });

    const bool importStatus = pImporter->Initialize(szFileName, -1, m_pFBXManager->GetIOSettings());

    int fileMajor, fileMinor, fileRevision;
    pImporter->GetFileVersion(fileMajor, fileMinor, fileRevision);
    {
      if (!importStatus)
      {
        ezLog::Error("FBX import failed, status: '{0}'", pImporter->GetStatus().GetErrorString());

        if (pImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
          int sdkMajor, sdkMinor, sdkRevision;
          FbxManager::GetFileFormatVersion(sdkMajor, sdkMinor, sdkRevision);

          ezLog::Error("FBX file format of SDK:  {0}.{1}.{2}", sdkMajor, sdkMinor, sdkRevision);
          ezLog::Error("FBX file format of file: {0}.{1}.{2}", fileMajor, fileMinor, fileRevision);
        }
      }
    }


    if (!pImporter->IsFBX())
    {
      ezLog::Error("FBX imported didn't recognize '{0}' as FBX, no import will occur.", szFileName);
      return nullptr;
    }

    ezLog::Info("FBX file format of file: {0}.{1}.{2}", fileMajor, fileMinor, fileRevision);

    FbxScene* pScene = FbxScene::Create(m_pFBXManager, szFileName);
    EZ_SCOPE_EXIT(if (pScene) { pScene->Destroy(); pScene = nullptr; });

    // Do the actual import into the FBX data structure
    if (!pImporter->Import(pScene))
    {
      ezLog::Error("FBX import failed, status: '{0}'", pImporter->GetStatus().GetErrorString());
      if (pImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
      {
        ezLog::Warning("FBX file is password protected, password input not yet supported.");
      }

      return nullptr;
    }

    FbxAxisSystem converToGL(FbxAxisSystem::eOpenGL);
    converToGL.ConvertScene(pScene);

    ezSharedPtr<Scene> outScene = EZ_DEFAULT_NEW(Scene);

    {
      FBXImportContext ImportContext{ pImporter, pScene, outScene };

      // Import hierarchy
      if (pScene->GetRootNode())
      {
        if (!ImportNodeRecursive(pScene->GetRootNode(), ImportContext).IsValid())
        {
          ezLog::Error("Errors during FBX import of '{0}'", szFileName);
          return nullptr;
        }
      }
    }

    return outScene;
  }

#else
  // Empty stub implementation when not building with the official FBX SDK
  FBXSDKImporter::FBXSDKImporter()
  {
  }

  FBXSDKImporter::~FBXSDKImporter()
  {
  }

  ezArrayPtr<const ezString> FBXSDKImporter::GetSupportedFileFormats() const
  {
    return ezMakeArrayPtr(m_supportedFileFormats);
  }

  ezSharedPtr<Scene> FBXSDKImporter::ImportScene(const char* szFileName)
  {
    return nullptr;
  }

#endif

  }
