#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Importers/AssimpImporter.h>
#include <EditorPluginAssets/ModelImporter/Scene.h>
#include <EditorPluginAssets/ModelImporter/Node.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <EditorPluginAssets/ModelImporter/Material.h>

#include <Foundation/Logging/Log.h>

#include <../ThirdParty/AssImp/include/scene.h>
#include <../ThirdParty/AssImp/include/Importer.hpp>
#include <../ThirdParty/AssImp/include/postprocess.h>
#include <../ThirdParty/AssImp/include/Logger.hpp>
#include <../ThirdParty/AssImp/include/LogStream.hpp>
#include <../ThirdParty/AssImp/include/DefaultLogger.hpp>

namespace ezModelImporter
{
  AssimpImporter::AssimpImporter()
  {
    Assimp::Importer importer;
    aiString assimpExtensions;
    importer.GetExtensionList(assimpExtensions);

    ezStringBuilder extensionListString = assimpExtensions.C_Str();
    extensionListString.ReplaceAll("*.", "");

    ezDynamicArray<ezStringView> supportedFileFormatViews;
    extensionListString.Split(false, supportedFileFormatViews, ";");

    m_supportedFileFormats.Reserve(supportedFileFormatViews.GetCount());
    for (ezUInt32 i = 0; i < supportedFileFormatViews.GetCount(); ++i)
    {
      m_supportedFileFormats.PushBack(ezString(supportedFileFormatViews[i]));
    }
  }

  ezArrayPtr<const ezString> AssimpImporter::GetSupportedFileFormats() const
  {
    return ezMakeArrayPtr(m_supportedFileFormats);
  }

  float ConvertAssimpType(float value, bool invert)
  {
    if (invert)
      return 1.0f - value;
    else
      return value;
  }

  ezUInt32 ConvertAssimpType(int value, bool invert)
  {
    return value;
  }

  ezColor ConvertAssimpType(const aiColor3D& value, bool invert)
  {
    if (invert)
      return ezColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return ezColor(value.r, value.g, value.b);
  }

  template<typename assimpType, typename ezType>
  void TryReadAssimpProperty(const char* pKey, unsigned int type, unsigned int idx, SemanticHint::Enum semantic, const aiMaterial& assimpMaterial, Material& material, bool invert = false)
  {
    assimpType assimpValue;
    if (assimpMaterial.Get(pKey, type, idx, assimpValue) == AI_SUCCESS)
    {
      Property& materialProperty = material.m_Properties.ExpandAndGetRef();
      materialProperty.m_Semantic = pKey;
      materialProperty.m_SemanticHint = semantic;
      materialProperty.m_Value = ConvertAssimpType(assimpValue, invert);
    }
  }

  void TryReadAssimpTextures(aiTextureType assimpTextureType, const char* semanticString, SemanticHint::Enum semanticHint, const aiMaterial& assimpMaterial, Material& material)
  {
    material.m_Textures.Reserve(material.m_Textures.GetCount() + assimpMaterial.GetTextureCount(assimpTextureType));
    for (unsigned int i = 0; i < assimpMaterial.GetTextureCount(assimpTextureType); ++i)
    {
      TextureReference& textureReference = material.m_Textures.ExpandAndGetRef();
      aiString path;

      assimpMaterial.GetTexture(assimpTextureType, i, &path, nullptr, &textureReference.m_UVSetIndex, nullptr, nullptr, nullptr);

      textureReference.m_SemanticHint = semanticHint;
      textureReference.m_Semantic = semanticString;
      textureReference.m_FileName = path.C_Str();
    }
  }

  void ImportMaterials(ezArrayPtr<aiMaterial*> assimpMaterials, Scene& outScene, ezDynamicArray<MaterialHandle>& outMaterialHandles)
  {
    outMaterialHandles.Reserve(assimpMaterials.GetCount());

    for (unsigned int materialIdx = 0; materialIdx < assimpMaterials.GetCount(); ++materialIdx)
    {
      aiMaterial* assimpMaterial = assimpMaterials[materialIdx];
      ezUniquePtr<Material> material(EZ_DEFAULT_NEW(Material));

      // Fetch name
      aiString name;
      if (assimpMaterial->Get(AI_MATKEY_NAME, name) != aiReturn_SUCCESS || name.length == 0)
        material->m_Name = ezConversionUtils::ToString(materialIdx);
      else
        material->m_Name = name.C_Str();

      // Read properties.
      // For type mappings see http://assimp.sourceforge.net/lib_html/materials.html
      // Note that we're currently ignoring all parameters around assimps "texture blending stack" as well as "mapping modes".
      material->m_Properties.Reserve(assimpMaterial->mNumProperties);
      TryReadAssimpProperty<aiColor3D, ezColor>(AI_MATKEY_COLOR_DIFFUSE, SemanticHint::DIFFUSE, *assimpMaterial, *material);
      TryReadAssimpProperty<aiColor3D, ezColor>(AI_MATKEY_COLOR_SPECULAR, SemanticHint::METALLIC, *assimpMaterial, *material);
      TryReadAssimpProperty<aiColor3D, ezColor>(AI_MATKEY_COLOR_AMBIENT, SemanticHint::AMBIENT, *assimpMaterial, *material);
      TryReadAssimpProperty<aiColor3D, ezColor>(AI_MATKEY_COLOR_EMISSIVE, SemanticHint::EMISSIVE, *assimpMaterial, *material);
      TryReadAssimpProperty<aiColor3D, ezColor>(AI_MATKEY_COLOR_TRANSPARENT, SemanticHint::OPACITY, *assimpMaterial, *material, true);
      TryReadAssimpProperty<int, ezInt32>(AI_MATKEY_ENABLE_WIREFRAME, SemanticHint::WIREFRAME, *assimpMaterial, *material);
      TryReadAssimpProperty<int, ezInt32>(AI_MATKEY_TWOSIDED, SemanticHint::TWOSIDED, *assimpMaterial, *material);
      TryReadAssimpProperty<int, ezInt32>(AI_MATKEY_SHADING_MODEL, SemanticHint::SHADINGMODEL, *assimpMaterial, *material);
      TryReadAssimpProperty<int, ezInt32>(AI_MATKEY_BLEND_FUNC, SemanticHint::UNKNOWN, *assimpMaterial, *material); // There is only "additive" and "default". Rather impractical so we're mapping to UNKNOWN.
      TryReadAssimpProperty<float, float>(AI_MATKEY_OPACITY, SemanticHint::OPACITY, *assimpMaterial, *material); // Yes, we can end up with two properties with semantic hint "OPACITY"
      TryReadAssimpProperty<float, float>(AI_MATKEY_SHININESS, SemanticHint::ROUGHNESS, *assimpMaterial, *material);
      TryReadAssimpProperty<float, float>(AI_MATKEY_SHININESS_STRENGTH, SemanticHint::METALLIC, *assimpMaterial, *material); // From assimp documentation "Scales the specular color of the material. This value is kept separate from the specular color by most modelers, and so do we."
      TryReadAssimpProperty<float, float>(AI_MATKEY_REFRACTI, SemanticHint::REFRACTIONINDEX, *assimpMaterial, *material);

      // Read textures.
      TryReadAssimpTextures(aiTextureType_DIFFUSE, "Diffuse", SemanticHint::DIFFUSE, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_SPECULAR, "Specular", SemanticHint::METALLIC, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_AMBIENT, "Ambient", SemanticHint::AMBIENT, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_EMISSIVE, "Emissive", SemanticHint::EMISSIVE, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_HEIGHT, "Height", SemanticHint::DISPLACEMENT, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_NORMALS, "Normals", SemanticHint::NORMAL, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_SHININESS, "Shininess", SemanticHint::ROUGHNESS, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_OPACITY, "Opacity", SemanticHint::OPACITY, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_DISPLACEMENT, "Displacement", SemanticHint::DISPLACEMENT, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_LIGHTMAP, "LightMap", SemanticHint::LIGHTMAP, *assimpMaterial, *material);
      TryReadAssimpTextures(aiTextureType_REFLECTION, "Reflection", SemanticHint::METALLIC, *assimpMaterial, *material); // From Assimp documentation "Contains the color of a perfect mirror reflection."

      outMaterialHandles.PushBack(outScene.AddMaterial(std::move(material)));
    }
  }

  void ImportMeshes(ezArrayPtr<aiMesh*> assimpMeshes, const ezDynamicArray<MaterialHandle>& materialHandles, const char* szFileName, Scene& outScene, ezDynamicArray<ObjectHandle>& outMeshHandles)
  {
    outMeshHandles.Reserve(assimpMeshes.GetCount());

    // Note that while Assimp has the same attribute sharing on each data stream.
    // Like a Vertex Buffer but in separate streams - a vertex has always all parameters but may be shared using an index buffer.

    for (unsigned int meshIdx = 0; meshIdx < assimpMeshes.GetCount(); ++meshIdx)
    {
      aiMesh* assimpMesh = assimpMeshes[meshIdx];
      ezUniquePtr<Mesh> mesh(EZ_DEFAULT_NEW(Mesh));
      mesh->m_Name = assimpMesh->mName.C_Str();

      mesh->AddTriangles(assimpMesh->mNumFaces);
      ezHybridArray<VertexDataStream*, 8> vertexDataStreams;

      // Vertex data.
      if (assimpMesh->HasPositions())
      {
        VertexDataStream* positions = mesh->AddDataStream(ezGALVertexAttributeSemantic::Position, 3);
        ezArrayPtr<float> assimpPositionPtr(reinterpret_cast<float*>(assimpMesh->mVertices), assimpMesh->mNumVertices * 3);
        positions->AddValues(assimpPositionPtr);
        vertexDataStreams.PushBack(positions);
      }
      if (assimpMesh->HasNormals())
      {
        VertexDataStream* normals = mesh->AddDataStream(ezGALVertexAttributeSemantic::Normal, 3);
        ezArrayPtr<float> assimpNormalPtr(reinterpret_cast<float*>(assimpMesh->mNormals), assimpMesh->mNumVertices * 3);
        normals->AddValues(assimpNormalPtr);
        vertexDataStreams.PushBack(normals);
      }
      if (assimpMesh->HasVertexColors(0))
      {
        VertexDataStream* colors = mesh->AddDataStream(ezGALVertexAttributeSemantic::Color, 4);
        ezArrayPtr<float> assimpColorsPtr(reinterpret_cast<float*>(assimpMesh->mColors), assimpMesh->mNumVertices * 4);
        colors->AddValues(assimpColorsPtr);
        vertexDataStreams.PushBack(colors);

        if (assimpMesh->GetNumColorChannels() > 1)
          ezLog::Warning("Mesh '%s' in '%s' has %i sets of vertex colors, only the first set will be imported!", mesh->m_Name.GetData(), szFileName, assimpMesh->GetNumColorChannels());
      }
      if (assimpMesh->HasTangentsAndBitangents())
      {
        VertexDataStream* tangents = mesh->AddDataStream(ezGALVertexAttributeSemantic::Tangent, 3);
        VertexDataStream* bitangents = mesh->AddDataStream(ezGALVertexAttributeSemantic::BiTangent, 3);

        ezArrayPtr<float> assimpTangentsPtr(reinterpret_cast<float*>(assimpMesh->mTangents), assimpMesh->mNumVertices * 3);
        ezArrayPtr<float> assimpBitangentsPtr(reinterpret_cast<float*>(assimpMesh->mBitangents), assimpMesh->mNumVertices * 3);
        tangents->AddValues(assimpTangentsPtr);
        bitangents->AddValues(assimpBitangentsPtr);

        vertexDataStreams.PushBack(tangents);
        vertexDataStreams.PushBack(bitangents);
      }
      static_assert(AI_MAX_NUMBER_OF_TEXTURECOORDS < 10, "Need to handle the fact that assimp supports more texcoord sets than we do.");
      for (unsigned int texcoordSet = 0; texcoordSet < assimpMesh->GetNumUVChannels(); ++texcoordSet)
      {
        unsigned int texcoordDimensionality = assimpMesh->mNumUVComponents[texcoordSet];

        VertexDataStream* texcoords = mesh->AddDataStream(static_cast<ezGALVertexAttributeSemantic::Enum>(ezGALVertexAttributeSemantic::TexCoord0 + texcoordSet), texcoordDimensionality);
        texcoords->ReserveData(assimpMesh->mNumVertices);
        for (unsigned int coord = 0; coord < assimpMesh->mNumVertices; ++coord)
        {
          texcoords->AddValues(ezArrayPtr<float>(reinterpret_cast<float*>(assimpMesh->mTextureCoords[texcoordSet] + coord), texcoordDimensionality));
        }
        vertexDataStreams.PushBack(texcoords);
      }

      if (assimpMesh->HasBones())
      {
        /// \todo import animation data
        ezLog::Warning("Mesh '%s' in '%s' has bone animation data. This is not yet supported and won't be imported.", mesh->m_Name.GetData(), szFileName);
      }

      // Triangles/Indices
      for (VertexDataStream* dataStream : vertexDataStreams)
      {
        ezArrayPtr<const Mesh::Triangle> triangles = mesh->GetTriangles();
        ezUInt32 uiNumElementsPerVertex = dataStream->GetNumElementsPerVertex();
        for (unsigned int faceIdx = 0; faceIdx < assimpMesh->mNumFaces; ++faceIdx)
        {
          EZ_ASSERT_DEBUG(assimpMesh->mFaces[faceIdx].mNumIndices == 3, "Expected triangles from Assimp. Triangulate didn't work?");

          dataStream->SetDataIndex(triangles[faceIdx].m_Vertices[0], assimpMesh->mFaces[faceIdx].mIndices[0] * uiNumElementsPerVertex);
          dataStream->SetDataIndex(triangles[faceIdx].m_Vertices[1], assimpMesh->mFaces[faceIdx].mIndices[1] * uiNumElementsPerVertex);
          dataStream->SetDataIndex(triangles[faceIdx].m_Vertices[2], assimpMesh->mFaces[faceIdx].mIndices[2] * uiNumElementsPerVertex);
        }
      }

      // Material - an assimp mesh uses only a single material!
      if (assimpMesh->mMaterialIndex >= materialHandles.GetCount())
        ezLog::Warning("Mesh '%s' in '%s' points to material %i, but there are only %i materials.", mesh->m_Name.GetData(), szFileName, assimpMesh->mMaterialIndex, materialHandles.GetCount());
      else
      {
        SubMesh subMesh;
        subMesh.m_uiFirstTriangle = 0;
        subMesh.m_uiTriangleCount = mesh->GetTriangles().GetCount();
        subMesh.m_Material = materialHandles[assimpMesh->mMaterialIndex];
        mesh->AddSubMesh(subMesh);
      }

      outMeshHandles.PushBack(outScene.AddMesh(std::move(mesh)));
    }
  }

  ObjectHandle ImportNodesRecursive(aiNode* assimpNode, const ezDynamicArray<ObjectHandle>& meshHandles, Scene& outScene)
  {
    Node* newNode = EZ_DEFAULT_NEW(Node);
    ObjectHandle newNodeHandle = outScene.AddNode(ezUniquePtr<Node>(newNode, ezFoundation::GetDefaultAllocator()));

    newNode->m_Name = assimpNode->mName.C_Str();

    // Transformation.
    ezMat4 mTransformation;
    mTransformation.SetFromArray(&assimpNode->mTransformation.a1, ezMatrixLayout::RowMajor);
    newNode->m_RelativeTransform = ezTransform(mTransformation);

    // Add metadata.
    if (assimpNode->mMetaData)
    {
      for (unsigned int metadataIdx = 0; metadataIdx < assimpNode->mMetaData->mNumProperties; ++metadataIdx)
      {
        Node::Metadata data;
        data.m_Key = assimpNode->mMetaData->mKeys[metadataIdx].C_Str();
        switch (assimpNode->mMetaData->mValues[metadataIdx].mType)
        {
        case AI_BOOL:
          data.m_Data = *static_cast<bool*> (assimpNode->mMetaData->mValues[metadataIdx].mData);
          break;
        case AI_INT:
          data.m_Data = *static_cast<ezInt32*>(assimpNode->mMetaData->mValues[metadataIdx].mData);
          break;
        case AI_UINT64:
          data.m_Data = *static_cast<ezInt64*>(assimpNode->mMetaData->mValues[metadataIdx].mData);
          break;
        case AI_FLOAT:
          data.m_Data = *static_cast<float*>(assimpNode->mMetaData->mValues[metadataIdx].mData);
          break;
        case AI_AISTRING:
          data.m_Data = static_cast<char*>(assimpNode->mMetaData->mValues[metadataIdx].mData);
          break;
        case AI_AIVECTOR3D:
          data.m_Data = *static_cast<ezVec3*>(assimpNode->mMetaData->mValues[metadataIdx].mData);
          break;
        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
      }
    }

    // Associate meshes.
    for (unsigned int meshIdx = 0; meshIdx < assimpNode->mNumMeshes; ++meshIdx)
    {
      newNode->m_Children.PushBack(meshHandles[assimpNode->mMeshes[meshIdx]]);
    }

    // Associate lights.
    // TODO

    // Associate cameras.
    // TODO

    // Import children.
    for (unsigned int childIdx = 0; childIdx < assimpNode->mNumChildren; ++childIdx)
    {
      newNode->m_Children.PushBack(ImportNodesRecursive(assimpNode->mChildren[childIdx], meshHandles, outScene));
    }

    return newNodeHandle;
  }

  void ImportNodes(aiNode* assimpRootNode, const ezDynamicArray<ObjectHandle>& meshHandles, Scene& outScene)
  {
    // If assimps root node has no transformation and no meta data, we're ignoring it.
    if (assimpRootNode->mTransformation.IsIdentity() && assimpRootNode->mMetaData == nullptr)
    {
      for (unsigned int childIdx = 0; childIdx < assimpRootNode->mNumChildren; ++childIdx)
      {
        ImportNodesRecursive(assimpRootNode->mChildren[childIdx],meshHandles, outScene);
      }
    }
    else
    {
      ImportNodesRecursive(assimpRootNode, meshHandles, outScene);
    }
  }

  ezSharedPtr<Scene> AssimpImporter::ImportScene(const char* szFileName)
  {
    class aiLogStream : public Assimp::LogStream
    {
    public:
      void write(const char* message)
      {
        ezLog::Dev("AssImp: %s", message);
      }
    };
    Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL);
    const unsigned int severity = Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn;
    Assimp::DefaultLogger::get()->attachStream(new aiLogStream(), severity);

    Assimp::Importer importer;

    // Note: ReadFileFromMemory is not able to read dependent files even if use our own Assimp::IOSystem. It is possible to use ReadFile instead but this involves leads to a lot of code...
    // Triangulate:           Our mesh format cannot handle anything else.
    // JoinIdenticalVertices: Assimp doesn't use index buffer at all if this is not specified.
    // TransformUVCoords:     As of now we do not have a concept for uv transforms.
    // Process_FlipUVs:       Assimp assumes OpenGl style UV coordinate system otherwise.
    const aiScene* assimpScene = importer.ReadFile(szFileName, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_TransformUVCoords | aiProcess_FlipUVs);
    if (!assimpScene)
    {
      ezLog::Error("Assimp importer failed to load model %s with error %s.", szFileName, importer.GetErrorString());
      return nullptr;
    }

    ezSharedPtr<Scene> outScene = EZ_DEFAULT_NEW(Scene);

    // Import materials.
    ezDynamicArray<MaterialHandle> materialHandles;
    ImportMaterials(ezArrayPtr<aiMaterial*>(assimpScene->mMaterials, assimpScene->mNumMaterials), *outScene, materialHandles);

    // Import meshes.
    ezDynamicArray<ObjectHandle> meshHandles;
    ImportMeshes(ezArrayPtr<aiMesh*>(assimpScene->mMeshes, assimpScene->mNumMeshes), materialHandles, szFileName, *outScene, meshHandles);

    // Import nodes.
    ezDynamicArray<ObjectHandle> nodeHandles;
    ImportNodes(assimpScene->mRootNode, meshHandles, *outScene);

    // Import lights.
    // TODO

    // Import cameras.
    // TODO

    // Import nodes and build hierarchy.

    return outScene;
  }
}
