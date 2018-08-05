#include <PCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter/Importers/AssimpImporter.h>
#include <ModelImporter/Material.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/Node.h>
#include <ModelImporter/Scene.h>
#include <ModelImporter/VertexData.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

#include <../ThirdParty/AssImp/include/DefaultLogger.hpp>
#include <../ThirdParty/AssImp/include/Importer.hpp>
#include <../ThirdParty/AssImp/include/LogStream.hpp>
#include <../ThirdParty/AssImp/include/Logger.hpp>
#include <../ThirdParty/AssImp/include/postprocess.h>
#include <../ThirdParty/AssImp/include/scene.h>

namespace ezModelImporter
{
  struct BoneInfo
  {
    ezString m_sBoneName;
    ezUInt32 m_uiParentBoneIndex = 0xFFFFFFFF;
    ezMat4 m_MeshOffset;
    bool m_bMeshOffsetIsValid = false;
  };

  AssimpImporter::AssimpImporter()
  {
    Assimp::Importer importer;
    aiString assimpExtensions;
    importer.GetExtensionList(assimpExtensions);

    ezStringBuilder extensionListString = assimpExtensions.C_Str();

    // When building with the official FBX SDK we disable FBX support in this importer
#if defined(BUILDSYSTEM_BUILD_WITH_OFFICIAL_FBX_SDK)
    extensionListString.ReplaceAll("*.fbx", "");
#endif

    extensionListString.ReplaceAll("*.", "");

    ezDynamicArray<ezStringView> supportedFileFormatViews;
    extensionListString.Split(false, supportedFileFormatViews, ";");

    m_supportedFileFormats.Reserve(supportedFileFormatViews.GetCount());
    for (ezUInt32 i = 0; i < supportedFileFormatViews.GetCount(); ++i)
    {
      if (supportedFileFormatViews[i].IsEmpty())
        continue;

      m_supportedFileFormats.PushBack(ezString(supportedFileFormatViews[i]));
    }
  }

  ezArrayPtr<const ezString> AssimpImporter::GetSupportedFileFormats() const { return ezMakeArrayPtr(m_supportedFileFormats); }

  float ConvertAssimpType(float value, bool invert = false)
  {
    if (invert)
      return 1.0f - value;
    else
      return value;
  }

  ezUInt32 ConvertAssimpType(int value, bool dummy = false) { return value; }

  ezColor ConvertAssimpType(const aiColor3D& value, bool invert = false)
  {
    if (invert)
      return ezColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return ezColor(value.r, value.g, value.b);
  }

  ezMat4 ConvertAssimpType(const aiMatrix4x4& value)
  {
    ezMat4 mTransformation;
    mTransformation.SetFromArray(&value.a1, ezMatrixLayout::RowMajor);
    return mTransformation;
  }

  ezVec3 ConvertAssimpType(const aiVector3D& value) { return ezVec3(value.x, value.y, value.z); }

  ezQuat ConvertAssimpType(const aiQuaternion& value) { return ezQuat(value.x, value.y, value.z, value.w); }

  template <typename assimpType, typename ezType>
  void TryReadAssimpProperty(const char* pKey, unsigned int type, unsigned int idx, SemanticHint::Enum semantic,
                             const aiMaterial& assimpMaterial, Material& material, bool invert = false)
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

  void TryReadAssimpTextures(aiTextureType assimpTextureType, const char* semanticString, SemanticHint::Enum semanticHint,
                             const aiMaterial& assimpMaterial, Material& material)
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
    ezStringBuilder tmp;

    for (unsigned int materialIdx = 0; materialIdx < assimpMaterials.GetCount(); ++materialIdx)
    {
      aiMaterial* assimpMaterial = assimpMaterials[materialIdx];
      ezUniquePtr<Material> material(EZ_DEFAULT_NEW(Material));

      // Fetch name
      aiString name;
      if (assimpMaterial->Get(AI_MATKEY_NAME, name) != aiReturn_SUCCESS || name.length == 0)
        material->m_Name = ezConversionUtils::ToString(materialIdx, tmp);
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
      TryReadAssimpProperty<int, ezInt32>(
          AI_MATKEY_BLEND_FUNC, SemanticHint::UNKNOWN, *assimpMaterial,
          *material); // There is only "additive" and "default". Rather impractical so we're mapping to UNKNOWN.
      TryReadAssimpProperty<float, float>(AI_MATKEY_OPACITY, SemanticHint::OPACITY, *assimpMaterial,
                                          *material); // Yes, we can end up with two properties with semantic hint "OPACITY"
      TryReadAssimpProperty<float, float>(AI_MATKEY_SHININESS, SemanticHint::ROUGHNESS, *assimpMaterial, *material);
      TryReadAssimpProperty<float, float>(AI_MATKEY_SHININESS_STRENGTH, SemanticHint::METALLIC, *assimpMaterial,
                                          *material); // From assimp documentation "Scales the specular color of the material. This value is
                                                      // kept separate from the specular color by most modelers, and so do we."
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
      TryReadAssimpTextures(aiTextureType_REFLECTION, "Reflection", SemanticHint::METALLIC, *assimpMaterial,
                            *material); // From Assimp documentation "Contains the color of a perfect mirror reflection."

      outMaterialHandles.PushBack(outScene.AddMaterial(std::move(material)));
    }
  }

  void ImportMeshes(ezArrayPtr<aiMesh*> assimpMeshes, const ezDynamicArray<MaterialHandle>& materialHandles, const char* szFileName,
                    Scene& outScene, ezDynamicArray<ObjectHandle>& outMeshHandles, ezDynamicArray<BoneInfo>& inout_allMeshBones)
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
        ezArrayPtr<char> assimpPositionPtr(reinterpret_cast<char*>(assimpMesh->mVertices), assimpMesh->mNumVertices * sizeof(ezVec3));
        positions->AddValues(assimpPositionPtr);
        vertexDataStreams.PushBack(positions);
      }
      if (assimpMesh->HasNormals())
      {
        VertexDataStream* normals = mesh->AddDataStream(ezGALVertexAttributeSemantic::Normal, 3);
        ezArrayPtr<char> assimpNormalPtr(reinterpret_cast<char*>(assimpMesh->mNormals), assimpMesh->mNumVertices * sizeof(ezVec3));
        normals->AddValues(assimpNormalPtr);
        vertexDataStreams.PushBack(normals);
      }
      if (assimpMesh->HasVertexColors(0))
      {
        VertexDataStream* colors = mesh->AddDataStream(ezGALVertexAttributeSemantic::Color, 4);
        ezArrayPtr<char> assimpColorsPtr(reinterpret_cast<char*>(assimpMesh->mColors[0]), assimpMesh->mNumVertices * sizeof(ezVec4));
        colors->AddValues(assimpColorsPtr);
        vertexDataStreams.PushBack(colors);

        if (assimpMesh->GetNumColorChannels() > 1)
          ezLog::Warning("Mesh '{0}' in '{1}' has {2} sets of vertex colors, only the first set will be imported!", mesh->m_Name,
                         szFileName, assimpMesh->GetNumColorChannels());
      }
      if (assimpMesh->HasTangentsAndBitangents())
      {
        VertexDataStream* tangents = mesh->AddDataStream(ezGALVertexAttributeSemantic::Tangent, 3);
        VertexDataStream* bitangents = mesh->AddDataStream(ezGALVertexAttributeSemantic::BiTangent, 3);

        ezArrayPtr<char> assimpTangentsPtr(reinterpret_cast<char*>(assimpMesh->mTangents), assimpMesh->mNumVertices * sizeof(ezVec3));
        ezArrayPtr<char> assimpBitangentsPtr(reinterpret_cast<char*>(assimpMesh->mBitangents), assimpMesh->mNumVertices * sizeof(ezVec3));
        tangents->AddValues(assimpTangentsPtr);
        bitangents->AddValues(assimpBitangentsPtr);

        vertexDataStreams.PushBack(tangents);
        vertexDataStreams.PushBack(bitangents);
      }
      static_assert(AI_MAX_NUMBER_OF_TEXTURECOORDS < 10, "Need to handle the fact that assimp supports more texcoord sets than we do.");
      for (unsigned int texcoordSet = 0; texcoordSet < assimpMesh->GetNumUVChannels(); ++texcoordSet)
      {
        unsigned int texcoordDimensionality = assimpMesh->mNumUVComponents[texcoordSet];

        VertexDataStream* texcoords = mesh->AddDataStream(
            static_cast<ezGALVertexAttributeSemantic::Enum>(ezGALVertexAttributeSemantic::TexCoord0 + texcoordSet), texcoordDimensionality);
        texcoords->ReserveData(assimpMesh->mNumVertices);
        for (unsigned int coord = 0; coord < assimpMesh->mNumVertices; ++coord)
        {
          texcoords->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(assimpMesh->mTextureCoords[texcoordSet] + coord),
                                                texcoordDimensionality * sizeof(float)));
        }
        vertexDataStreams.PushBack(texcoords);
      }

      // only import bone weights, when the mesh has a skeleton
      if (assimpMesh->HasBones() && !inout_allMeshBones.IsEmpty())
      {
        VertexDataStream* boneWeightStream = nullptr;
        boneWeightStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::BoneWeights0, 4, VertexElementType::FLOAT);
        vertexDataStreams.PushBack(boneWeightStream);

        VertexDataStream* boneIndicesStream = nullptr;
        boneIndicesStream = mesh->AddDataStream(ezGALVertexAttributeSemantic::BoneIndices0, 4, VertexElementType::UINT32);
        vertexDataStreams.PushBack(boneIndicesStream);

        ezDynamicArray<ezVec4> boneWeightData;
        ezDynamicArray<ezVec4U32> boneIndexData;
        ezDynamicArray<ezUInt8> boneInfluenceCount;

        boneInfluenceCount.SetCountUninitialized(assimpMesh->mNumVertices);
        boneWeightData.SetCountUninitialized(assimpMesh->mNumVertices);
        boneIndexData.SetCountUninitialized(assimpMesh->mNumVertices);

        // init all with zero
        for (ezUInt32 i = 0; i < assimpMesh->mNumVertices; ++i)
        {
          boneInfluenceCount[i] = 0;
          boneWeightData[i].SetZero();
          boneIndexData[i].SetZero();
        }

        for (ezUInt32 bone = 0; bone < assimpMesh->mNumBones; ++bone)
        {
          const auto* pBone = assimpMesh->mBones[bone];
          const ezUInt32 numWeights = pBone->mNumWeights;
          ezUInt32 uiBoneIndex = 0;

          // map this bone to the global skeleton index
          {
            for (uiBoneIndex = 0; uiBoneIndex < inout_allMeshBones.GetCount(); ++uiBoneIndex)
            {
              if (inout_allMeshBones[uiBoneIndex].m_sBoneName == pBone->mName.C_Str())
                break;
            }

            EZ_ASSERT_DEBUG(uiBoneIndex < inout_allMeshBones.GetCount(), "Bone not found");
            inout_allMeshBones[uiBoneIndex].m_MeshOffset = ConvertAssimpType(pBone->mOffsetMatrix);
            inout_allMeshBones[uiBoneIndex].m_bMeshOffsetIsValid = true;
          }

          for (ezUInt32 w = 0; w < numWeights; ++w)
          {
            const auto& wgt = pBone->mWeights[w];
            const ezUInt32 vtxIdx = wgt.mVertexId;
            const ezUInt32 influence = boneInfluenceCount[vtxIdx]++;

            EZ_ASSERT_DEBUG(influence < 4, "Too many bone influences for a single vertex");

            boneWeightData[vtxIdx].GetData()[influence] = wgt.mWeight;
            boneIndexData[vtxIdx].GetData()[influence] = uiBoneIndex;
          }
        }

        boneWeightStream->AddValues(
            ezArrayPtr<char>(reinterpret_cast<char*>(boneWeightData.GetData()), boneWeightData.GetCount() * 4 * sizeof(float)));
        boneIndicesStream->AddValues(
            ezArrayPtr<char>(reinterpret_cast<char*>(boneIndexData.GetData()), boneIndexData.GetCount() * 4 * sizeof(ezUInt32)));
      }

      // Triangles/Indices
      for (VertexDataStream* dataStream : vertexDataStreams)
      {
        ezArrayPtr<const Mesh::Triangle> triangles = mesh->GetTriangles();
        ezUInt32 uiAttributeSize = dataStream->GetAttributeSize();
        for (unsigned int faceIdx = 0; faceIdx < assimpMesh->mNumFaces; ++faceIdx)
        {
          EZ_ASSERT_DEBUG(assimpMesh->mFaces[faceIdx].mNumIndices == 3, "Expected triangles from Assimp. Triangulate didn't work?");

          dataStream->SetDataIndex(triangles[faceIdx].m_Vertices[0], assimpMesh->mFaces[faceIdx].mIndices[0] * uiAttributeSize);
          dataStream->SetDataIndex(triangles[faceIdx].m_Vertices[1], assimpMesh->mFaces[faceIdx].mIndices[1] * uiAttributeSize);
          dataStream->SetDataIndex(triangles[faceIdx].m_Vertices[2], assimpMesh->mFaces[faceIdx].mIndices[2] * uiAttributeSize);
        }
      }

      // Material - an assimp mesh uses only a single material!
      if (assimpMesh->mMaterialIndex >= materialHandles.GetCount())
        ezLog::Warning("Mesh '{0}' in '{1}' points to material {2}, but there are only {3} materials.", mesh->m_Name, szFileName,
                       assimpMesh->mMaterialIndex, materialHandles.GetCount());
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
    newNode->m_RelativeTransform.SetFromMat4(mTransformation);

    // Add metadata.
    if (assimpNode->mMetaData)
    {
      for (unsigned int metadataIdx = 0; metadataIdx < assimpNode->mMetaData->mNumProperties; ++metadataIdx)
      {
        if (assimpNode->mMetaData->mValues[metadataIdx].mData == nullptr)
          continue;

        Node::Metadata data;
        data.m_Key = assimpNode->mMetaData->mKeys[metadataIdx].C_Str();
        switch (assimpNode->mMetaData->mValues[metadataIdx].mType)
        {
          case AI_BOOL:
            data.m_Data = *static_cast<bool*>(assimpNode->mMetaData->mValues[metadataIdx].mData);
            break;
          case AI_INT32:
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
        ImportNodesRecursive(assimpRootNode->mChildren[childIdx], meshHandles, outScene);
      }
    }
    else
    {
      ImportNodesRecursive(assimpRootNode, meshHandles, outScene);
    }
  }

  void ImportSkeletonRecursive(aiNode* assimpNode, ezDynamicArray<BoneInfo>& inout_allMeshBones, ezUInt32 uiParentBoneIdx)
  {
    const ezUInt32 boneIdx = inout_allMeshBones.GetCount();
    auto& boneInfo = inout_allMeshBones.ExpandAndGetRef();

    boneInfo.m_sBoneName = assimpNode->mName.C_Str();
    boneInfo.m_MeshOffset.SetIdentity();
    boneInfo.m_uiParentBoneIndex = uiParentBoneIdx;

    for (ezUInt32 c = 0; c < assimpNode->mNumChildren; ++c)
    {
      ImportSkeletonRecursive(assimpNode->mChildren[c], inout_allMeshBones, boneIdx);
    }
  }

  void ImportSkeleton(aiNode* assimpRootNode, ezDynamicArray<BoneInfo>& inout_allMeshBones)
  {
    ImportSkeletonRecursive(assimpRootNode, inout_allMeshBones, 0xFFFFFFFF);
  }

  void GenerateSkeleton(const ezDynamicArray<BoneInfo>& allMeshBones, Scene& outScene)
  {
    ezSkeletonBuilder sb;
    sb.SetSkinningMode(ezSkeleton::Mode::FourBones);

    for (ezUInt32 boneIdx = 0; boneIdx < allMeshBones.GetCount(); ++boneIdx)
    {
      ezMat4 localTransform = allMeshBones[boneIdx].m_MeshOffset.GetInverse();

      if (allMeshBones[boneIdx].m_uiParentBoneIndex != 0xFFFFFFFF)
      {
        const ezUInt32 uiParentBone = allMeshBones[boneIdx].m_uiParentBoneIndex;

        localTransform = allMeshBones[uiParentBone].m_MeshOffset * localTransform;
      }

      sb.AddBone(allMeshBones[boneIdx].m_sBoneName, localTransform, allMeshBones[boneIdx].m_uiParentBoneIndex);
    }

    outScene.m_pSkeleton = sb.CreateSkeletonInstance();
  }

  void ImportAnimations(const aiScene* assimpScene, Scene& outScene)
  {
    for (ezUInt32 animIdx = 0; animIdx < assimpScene->mNumAnimations; ++animIdx)
    {
      aiAnimation* pAnim = assimpScene->mAnimations[animIdx];

      // duration seems to just be the max number of keyframes (-1)
      const ezUInt32 uiNumKeyframes = ((ezUInt32)pAnim->mDuration) + 1;

      AnimationClip& animClip = outScene.m_AnimationClips.ExpandAndGetRef();
      animClip.m_sClipName = pAnim->mName.C_Str();
      animClip.m_uiFramesPerSecond = static_cast<ezUInt32>(pAnim->mTicksPerSecond);
      animClip.m_uiNumKeyframes = uiNumKeyframes;

      animClip.m_BoneAnimations.SetCount(pAnim->mNumChannels);

      for (ezUInt32 channelIdx = 0; channelIdx < pAnim->mNumChannels; ++channelIdx)
      {
        const auto& channel = pAnim->mChannels[channelIdx];
        BoneAnimation& boneAnim = animClip.m_BoneAnimations[channelIdx];

        boneAnim.m_sBoneName = channel->mNodeName.C_Str();
        boneAnim.m_Keyframes.SetCountUninitialized(uiNumKeyframes);

        for (ezUInt32 keyframeIdx = 0; keyframeIdx < uiNumKeyframes; ++keyframeIdx)
        {
          ezMat4 keyframeMat;
          keyframeMat.SetIdentity();

          // indices could also depend on the animation state (loop, etc) but for now we just clamp
          const ezUInt32 uiPosFrame = ezMath::Min(keyframeIdx, channel->mNumPositionKeys - 1);
          const ezUInt32 uiRotFrame = ezMath::Min(keyframeIdx, channel->mNumRotationKeys - 1);
          const ezUInt32 uiScaFrame = ezMath::Min(keyframeIdx, channel->mNumScalingKeys - 1);

          const ezVec3 keyPos = ConvertAssimpType(channel->mPositionKeys[uiPosFrame].mValue);
          const ezQuat keyRot = ConvertAssimpType(channel->mRotationKeys[uiRotFrame].mValue);
          const ezVec3 keySca = ConvertAssimpType(channel->mScalingKeys[uiScaFrame].mValue);

          keyframeMat.SetRotationalPart(keyRot.GetAsMat3());
          keyframeMat.SetScalingFactors(keySca);
          keyframeMat.SetTranslationVector(keyPos);

          boneAnim.m_Keyframes[keyframeIdx] = keyframeMat;
        }
      }
    }
  }

  ezSharedPtr<Scene> AssimpImporter::ImportScene(const char* szFileName, ezBitflags<ImportFlags> importFlags)
  {
    class aiLogStream : public Assimp::LogStream
    {
    public:
      void write(const char* message) { ezLog::Dev("AssImp: {0}", message); }
    };
    Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL);
    const unsigned int severity = Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn;
    Assimp::DefaultLogger::get()->attachStream(new aiLogStream(), severity);

    Assimp::Importer importer;

    // Note: ReadFileFromMemory is not able to read dependent files even if use our own Assimp::IOSystem. It is possible to use ReadFile
    // instead but this involves leads to a lot of code... Triangulate:           Our mesh format cannot handle anything else.
    // JoinIdenticalVertices: Assimp doesn't use index buffer at all if this is not specified.
    // TransformUVCoords:     As of now we do not have a concept for uv transforms.
    // Process_FlipUVs:       Assimp assumes OpenGl style UV coordinate system otherwise.


    ezUInt32 uiAssimpFlags = 0;
    if (importFlags.IsSet(ImportFlags::Meshes))
    {
      uiAssimpFlags |= aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_TransformUVCoords | aiProcess_FlipUVs;
    }

    const aiScene* assimpScene = importer.ReadFile(szFileName, uiAssimpFlags);
    if (!assimpScene)
    {
      ezLog::Error("Assimp importer failed to load model {0} with error {1}.", szFileName, importer.GetErrorString());
      return nullptr;
    }

    ezSharedPtr<Scene> outScene = EZ_DEFAULT_NEW(Scene);

    // Import materials.
    ezDynamicArray<MaterialHandle> materialHandles;
    ImportMaterials(ezArrayPtr<aiMaterial*>(assimpScene->mMaterials, assimpScene->mNumMaterials), *outScene, materialHandles);

    // Import skeleton
    ezDynamicArray<BoneInfo> allMeshBones;
    if (importFlags.IsAnySet(ImportFlags::Skeleton))
    {
      ImportSkeleton(assimpScene->mRootNode, allMeshBones);
    }

    // Import meshes.
    ezDynamicArray<ObjectHandle> meshHandles;
    if (importFlags.IsAnySet(ImportFlags::Skeleton | ImportFlags::Meshes))
    {
      ImportMeshes(ezArrayPtr<aiMesh*>(assimpScene->mMeshes, assimpScene->mNumMeshes), materialHandles, szFileName, *outScene, meshHandles,
                   allMeshBones);
    }

    if (importFlags.IsAnySet(ImportFlags::Skeleton))
    {
      GenerateSkeleton(allMeshBones, *outScene);
    }

    if (importFlags.IsSet(ImportFlags::Animations) && assimpScene->HasAnimations())
    {
      ImportAnimations(assimpScene, *outScene);
    }

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
