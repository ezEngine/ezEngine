#include <CoreUtils/PCH.h>
#include <CoreUtils/Geometry/OBJLoader.h>
#include <Foundation/Utilities/ConversionUtils.h>


ezOBJLoader::FaceVertex::FaceVertex()
{
  m_uiPositionID = 0;
  m_uiNormalID = 0;
  m_uiTexCoordID = 0;
}

ezOBJLoader::Face::Face()
{
  m_uiMaterialID = 0;
}

void ezOBJLoader::Clear()
{
  m_Positions.Clear();
  m_Normals.Clear();
  m_TexCoords.Clear();
  m_Faces.Clear();
  m_Materials.Clear();
}

static ezStringView ReadLine(ezStringView& szPos)
{
  while (szPos.GetCharacter() != '\0' && ezStringUtils::IsWhiteSpace(szPos.GetCharacter()))
    ++szPos;

  const char* szStart = szPos.GetData();

  while (szPos.GetCharacter() != '\0' && szPos.GetCharacter() != '\r' && szPos.GetCharacter() != '\n')
    ++szPos;

  const char* szEnd = szPos.GetData();

  while (szPos.GetCharacter() != '\0' && ezStringUtils::IsWhiteSpace(szPos.GetCharacter()))
    ++szPos;

  return ezStringView(szStart, szEnd);
}

static ezStringView ReadString(ezStringView& szPos)
{
  while (szPos.GetCharacter() != '\0' && ezStringUtils::IsWhiteSpace(szPos.GetCharacter()))
    ++szPos;

  const char* szStart = szPos.GetData();

  while (szPos.GetCharacter() != '\0' && !ezStringUtils::IsWhiteSpace(szPos.GetCharacter()))
    ++szPos;

  const char* szEnd = szPos.GetData();

  while (szPos.GetCharacter() != '\0' && ezStringUtils::IsWhiteSpace(szPos.GetCharacter()))
    ++szPos;

  return ezStringView(szStart, szEnd);
}

static bool SkipSlash(ezStringView& szPos)
{
  if (szPos.GetCharacter() != '/')
    return false;

  ++szPos;

  return (szPos.GetCharacter() != ' ' && szPos.GetCharacter() != '\t');
}

ezResult ezOBJLoader::LoadOBJ(const char* szFile, bool bIgnoreMaterials)
{
  ezFileReader File;
  if (File.Open(szFile).Failed())
    return EZ_FAILURE;

  ezString sContent;
  sContent.ReadAll(File);

  // which data has been found in the file
  bool bContainsTexCoords = false;
  bool bContainsNormals = false;

  ezUInt32 uiCurMaterial = 0xFFFFFFFF;

  ezStringView sText = sContent;

  ezUInt32 uiPositionOffset = m_Positions.GetCount();
  ezUInt32 uiNormalOffset = m_Normals.GetCount();
  ezUInt32 uiTexCoordOffset = m_TexCoords.GetCount();

  while (sText.IsValid())
  {
    ezStringView sLine = ReadLine(sText);
    const ezStringView sFirst = ReadString(sLine);

    if (sFirst.IsEqual_NoCase("v"))// line declares a vertex
    {
      ezVec3 v(0.0f);
      ezConversionUtils::ExtractFloatsFromString(sLine.GetData(), 3, &v.x);

      m_Positions.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("vt")) // line declares a texture coordinate
    {
      bContainsTexCoords = true;

      ezVec3 v(0.0f);
      ezConversionUtils::ExtractFloatsFromString(sLine.GetData(), 3, &v.x); // reads up to three texture-coordinates

      m_TexCoords.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("vn"))	// line declares a normal
    {
      bContainsNormals = true;

      ezVec3 v(0.0f);
      ezConversionUtils::ExtractFloatsFromString(sLine.GetData(), 3, &v.x);
      v.Normalize();	// make sure normals are indeed normalized

      m_Normals.PushBack(v);
    }
    else if (sFirst.IsEqual_NoCase("f"))	// line declares a face
    {
      Face face;
      face.m_uiMaterialID = uiCurMaterial;

      const char* szCurPos;

      // loop through all vertices, that are found
      while (sLine.IsValid())
      {
        ezInt32 id;

        // read the position index
        if (ezConversionUtils::StringToInt(sLine.GetData(), id, &szCurPos).Failed())
          break;	// nothing found, face-declaration is finished

        sLine.SetStartPosition(szCurPos);

        FaceVertex Vertex;
        Vertex.m_uiPositionID = uiPositionOffset + id - 1; // OBJ indices start at 1, so decrement them to start at 0

        // texcoords were declared, so they will be used in the faces
        if (bContainsTexCoords)
        {
          if (!SkipSlash(sLine))
            break;

          if (ezConversionUtils::StringToInt(sLine.GetData(), id, &szCurPos).Failed())
            break;

          sLine.SetStartPosition(szCurPos);

          Vertex.m_uiTexCoordID = uiTexCoordOffset + id - 1; // OBJ indices start at 1, so decrement them to start at 0
        }

        // normals were declared, so they will be used in the faces
        if (bContainsNormals)
        {
          if (!SkipSlash(sLine))
            break;

          if (ezConversionUtils::StringToInt(sLine.GetData(), id, &szCurPos).Failed())
            break;

          sLine.SetStartPosition(szCurPos);

          Vertex.m_uiNormalID = uiNormalOffset + id - 1;		// OBJ indices start at 1, so decrement them to start at 0
        }

        // stores the next vertex of the face
        face.m_Vertices.PushBack(Vertex);
      }

      // only allow faces with at least 3 vertices
      if (face.m_Vertices.GetCount() >= 3)
      {
        ezVec3 v1, v2, v3;
        v1 = m_Positions[face.m_Vertices[0].m_uiPositionID];
        v2 = m_Positions[face.m_Vertices[1].m_uiPositionID];
        v3 = m_Positions[face.m_Vertices[2].m_uiPositionID];

        face.m_vNormal.CalculateNormal(v1, v2, v3);

        // done reading the face, store it
        m_Faces.PushBack(face);
      }
    }
    else if (sFirst.IsEqual_NoCase("usemtl"))		// next material to be used for the following faces
    {
      if (bIgnoreMaterials)
        uiCurMaterial = 0xFFFFFFFF;
      else
      {
        // look-up the ID of this material

        bool bExisted = false;
        auto mat = m_Materials.FindOrAdd(sLine, &bExisted).Value();

        if (!bExisted)
          mat.m_uiMaterialID = m_Materials.GetCount() - 1;
        
        uiCurMaterial = mat.m_uiMaterialID;
      }
    }
  }

  return EZ_SUCCESS;
}

void ezOBJLoader::SortFacesByMaterial()
{
  // sort all faces by material-ID
  m_Faces.Sort();
}

void ezOBJLoader::ComputeTangentSpaceVectors()
{
  // cannot compute tangents without texture-coordinates
  if (!HasTextureCoordinates())
    return;

  for (ezUInt32 f = 0; f < m_Faces.GetCount(); ++f)
  {
    Face& face = m_Faces[f];

    const ezVec3 p1 = m_Positions[face.m_Vertices[0].m_uiPositionID];
    const ezVec3 p2 = m_Positions[face.m_Vertices[1].m_uiPositionID];
    const ezVec3 p3 = m_Positions[face.m_Vertices[2].m_uiPositionID];

    const ezVec3 tc1 = m_TexCoords[face.m_Vertices[0].m_uiTexCoordID];
    const ezVec3 tc2 = m_TexCoords[face.m_Vertices[1].m_uiTexCoordID];
    const ezVec3 tc3 = m_TexCoords[face.m_Vertices[2].m_uiTexCoordID];

    ezVec3 v2v1 = p2 - p1;
    ezVec3 v3v1 = p3 - p1;

    float c2c1_T = tc2.x - tc1.x;
    float c2c1_B = tc2.y - tc1.y;

    float c3c1_T = tc3.x - tc1.x;
    float c3c1_B = tc3.y - tc1.y;

    float fDenominator = c2c1_T * c3c1_B - c3c1_T * c2c1_B;

    float fScale1 = 1.0f / fDenominator;

    ezVec3 T, B;
    T = ezVec3((c3c1_B * v2v1.x - c2c1_B * v3v1.x) * fScale1,
               (c3c1_B * v2v1.y - c2c1_B * v3v1.y) * fScale1,
               (c3c1_B * v2v1.z - c2c1_B * v3v1.z) * fScale1);

    B = ezVec3((-c3c1_T * v2v1.x + c2c1_T * v3v1.x) * fScale1,
               (-c3c1_T * v2v1.y + c2c1_T * v3v1.y) * fScale1,
               (-c3c1_T * v2v1.z + c2c1_T * v3v1.z) * fScale1);

    T.Normalize();
    B.Normalize();

    face.m_vTangent = T;
    face.m_vBiTangent = face.m_vNormal.Cross(face.m_vTangent).GetNormalized();
  }
}

ezResult ezOBJLoader::LoadMTL(const char* szFile, const char* szMaterialBasePath)
{
  ezFileReader File;
  if (File.Open(szFile).Failed())
    return EZ_FAILURE;

  ezString sContent;
  sContent.ReadAll(File);

  ezStringView sText = sContent;

  ezString sCurMatName;
  ezStringBuilder sTemp;

  while (sText.IsValid())
  {
    ezStringView sLine = ReadLine(sText);
    const ezStringView sFirst = ReadString(sLine);

    if (sFirst.IsEqual_NoCase("newmtl")) // declares a new material with a given name
    {
      sCurMatName = sLine;

      bool bExisted = false;
      auto mat = m_Materials.FindOrAdd(sCurMatName, &bExisted).Value();

      if (!bExisted)
      {
        mat.m_uiMaterialID = m_Materials.GetCount() - 1;
      }
    }
    else if (sFirst.IsEqual_NoCase("map_Kd"))
    {
      sTemp = szMaterialBasePath;
      sTemp.AppendPath(sLine.GetData());

      m_Materials[sCurMatName].m_sDiffuseTexture = sTemp;
    }
  }

  return EZ_SUCCESS;
}





EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Geometry_Implementation_OBJLoader);

