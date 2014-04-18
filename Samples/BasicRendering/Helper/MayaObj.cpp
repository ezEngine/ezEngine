#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/HashTable.h>

#include <Helper/MayaObj.h>
#include <Helper/Misc.h>
#include <RendererFoundation/Device/Device.h>


EZ_FORCE_INLINE ezUInt64 FaceIndicesToKey(ezUInt32 PosIndex, ezUInt32 TexCoordIndex, ezUInt32 NormIndex)
{
  return ((ezUInt64)(PosIndex & 0xFFFFFu)) << 40u | ((ezUInt64)(TexCoordIndex & 0xFFFFFu)) << 20u | (ezUInt64)(NormIndex & 0xFFFFFu) << 0u;
}

namespace DontUse
{
  MayaObj* MayaObj::LoadFromFile(const char* szPath, ezGALDevice* pDevice)
  {

    ezDynamicArray<ezUInt8> FileContent;

    if(!ReadCompleteFile(szPath, FileContent).Succeeded())
      return nullptr;

    FileContent.PushBack('\0');

    ezStringBuilder sContent = (const char*) &FileContent[0];

    ezDynamicArray<ezString> Lines;
    Lines.Reserve(2048);
    sContent.ReplaceAll("\r", "");

    sContent.Split(true, Lines, "\n");


    ezDynamicArray<ezVec3> VPositions;
    VPositions.Reserve(1024);

    ezDynamicArray<ezVec3> VNormals;
    VNormals.Reserve(1024);

    ezDynamicArray<ezVec2> VTexCoords;
    VTexCoords.Reserve(1024);

    ezDynamicArray<Vertex> Vertices;
    Vertices.Reserve(1024);

    ezDynamicArray<ezUInt16> Indices;
    Indices.Reserve(2048);

    ezHashTable<ezUInt64, ezUInt16> FaceIndicesToVertexIndex;

    for(ezUInt32 Line = 0; Line < Lines.GetCount(); Line++)
    {
      const ezString& Current = Lines[Line];

      if(Current.StartsWith("v "))
      {
        ezVec3 Pos;
        sscanf(Current.GetData(), "v %f %f %f", &Pos.x, &Pos.y, &Pos.z);

        VPositions.PushBack(Pos);
      }
      else if(Current.StartsWith("vn "))
      {
        ezVec3 Norm;
        sscanf(Current.GetData(), "vn %f %f %f", &Norm.x, &Norm.y, &Norm.z);

        VNormals.PushBack(Norm);
      }
      else if(Current.StartsWith("vt "))
      {
        ezVec2 TexCoord;
        sscanf(Current.GetData(), "vt %f %f", &TexCoord.x, &TexCoord.y);

        VTexCoords.PushBack(TexCoord);
      }
      else if(Current.StartsWith("f "))
      {
        ezUInt32 uiPosIds[3];
        ezUInt32 uiNormIds[3];
        ezUInt32 uiTexCoordIds[3];

        sscanf(Current.GetData(), "f %u/%u/%u %u/%u/%u %u/%u/%u", &uiPosIds[0], &uiTexCoordIds[0], &uiNormIds[0], &uiPosIds[1], &uiTexCoordIds[1], &uiNormIds[1], &uiPosIds[2], &uiTexCoordIds[2], &uiNormIds[2]);

        // Build unique vertices or find exisiting matching ones
        ezUInt16 FaceIndices[3];

        for(ezUInt32 Index = 0; Index < 3; Index++)
        {
          uiPosIds[Index]--; uiTexCoordIds[Index]--; uiNormIds[Index]--;

          ezUInt64 Key = FaceIndicesToKey(uiPosIds[Index], uiTexCoordIds[Index], uiNormIds[Index]);

          if(FaceIndicesToVertexIndex.KeyExists(Key))
            FaceIndices[Index] = FaceIndicesToVertexIndex[Key];
          else
          {
            Vertex Temp;
            Temp.pos = VPositions[uiPosIds[Index]];
            Temp.tex0 = VTexCoords[uiTexCoordIds[Index]];
            Temp.norm = VNormals[uiNormIds[Index]];

            Vertices.PushBack(Temp);
            FaceIndices[Index] = Vertices.GetCount() - 1;

            FaceIndicesToVertexIndex.Insert(Key, FaceIndices[Index], nullptr);
          }
        }

        Indices.PushBack(FaceIndices[1]);
        Indices.PushBack(FaceIndices[2]);
        Indices.PushBack(FaceIndices[0]);
      }
    }

    // Create mode;
    if(Indices.GetCount() > 0)
    {
      MayaObj* Obj = EZ_DEFAULT_NEW(MayaObj)(Vertices, Indices, pDevice);
      return Obj;
    }

    return nullptr;
  }

  MayaObj::MayaObj(const ezArrayPtr<MayaObj::Vertex>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezGALDevice* pDevice)
    : m_pDevice(pDevice)
  {
    m_hVB = m_pDevice->CreateVertexBuffer(sizeof(Vertex), pVertices.GetCount(), pVertices.GetPtr());
    m_hIB = m_pDevice->CreateIndexBuffer(ezGALIndexType::UShort, pIndices.GetCount(), pIndices.GetPtr());

    m_uiPrimitiveCount = pIndices.GetCount() / 3;
  }

  MayaObj::~MayaObj()
  {
    m_pDevice->DestroyBuffer(m_hVB);
    m_pDevice->DestroyBuffer(m_hIB);
  }
}