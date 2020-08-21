#pragma once

/**
* https://blackpawn.com/texts/lightmaps/default.html
* https://learnopengl.com/In-Practice/Text-Rendering
*/

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Types.h>

struct ezTextureAtlasElement
{
  EZ_DECLARE_POD_TYPE();

  struct ezTextureAtlasElementInput
  {
    ezUInt32 Width;
    ezUInt32 Height;
  };

  ezTextureAtlasElementInput Input;

  struct ezTextureAtlasElementOutput
  {
    ezUInt32 x;
    ezUInt32 y;
    ezUInt32 Index;
    ezInt32 Page;
  };

  ezTextureAtlasElementOutput Output;
};

class ezTextureAtlasLayout
{
  struct ezTextureAtlasNode
  {
    ezUInt32 x;
    ezUInt32 y;
    ezUInt32 Width;
    ezUInt32 Height;
    ezUniquePtr<ezTextureAtlasNode> Left;
    ezUniquePtr<ezTextureAtlasNode> Right;
    bool Full;

    ezTextureAtlasNode()
      : x(0)
      , y(0)
      , Width(0)
      , Height(0)
      , Full(false)
      , Left(nullptr)
      , Right(nullptr)
    {
    }
    ezTextureAtlasNode(ezUInt32 x, ezUInt32 y, ezUInt32 width, ezUInt32 height)
      : x(x)
      , y(y)
      , Width(width)
      , Height(height)
      , Full(false)
      , Left(nullptr)
      , Right(nullptr)
    {
    }
  };

public:
  ezTextureAtlasLayout(ezTextureAtlasLayout&& other) noexcept
    : m_InitialWidth(other.m_InitialWidth)
    , m_InitialHeight(other.m_InitialHeight)
    , m_Width(other.m_Width)
    , m_Height(other.m_Height)
    , m_MaxWidth(other.m_MaxWidth)
    , m_MaxHeight(other.m_MaxHeight)
    , m_PowerOfTwo(other.m_PowerOfTwo)
    , m_RootNode(std::move(other.m_RootNode))
  {
  }

  ezTextureAtlasLayout()
    : m_InitialWidth(0)
    , m_InitialHeight(0)
    , m_Width(0)
    , m_Height(0)
    , m_MaxWidth(0)
    , m_MaxHeight(0)
    , m_PowerOfTwo(false)
    , m_RootNode(nullptr)
  {
  }

  ezTextureAtlasLayout(ezUInt32 width, ezUInt32 height, ezUInt32 maxWidth, ezUInt32 maxHeight, bool powerOfTwo = false)
    : m_InitialWidth(width)
    , m_InitialHeight(height)
    , m_Width(width)
    , m_Height(height)
    , m_MaxWidth(maxWidth)
    , m_MaxHeight(maxHeight)
    , m_PowerOfTwo(powerOfTwo)
  {
    m_RootNode = EZ_DEFAULT_NEW(ezTextureAtlasNode, 0, 0, maxWidth, maxHeight);
  }

  ~ezTextureAtlasLayout()
  {
    if (m_RootNode != nullptr)
      m_RootNode.Clear();
    //EZ_DEFAULT_DELETE(m_RootNode);
  }

  bool AddElement(ezTextureAtlasElement& element);

  void Clear();

  ezUInt32 GetWidth() const { return m_Width; }

  ezUInt32 GetHeight() const { return m_Height; }

private:
  bool AddToNode(ezTextureAtlasNode* node, ezTextureAtlasElement& element, bool allowGrowth);

  ezUInt32 m_InitialWidth;
  ezUInt32 m_InitialHeight;
  ezUInt32 m_Width;
  ezUInt32 m_Height;
  ezUInt32 m_MaxWidth;
  ezUInt32 m_MaxHeight;
  bool m_PowerOfTwo;

  ezUniquePtr<ezTextureAtlasNode> m_RootNode;
};

struct ezTextureAtlasPage
{
  EZ_DECLARE_POD_TYPE();

  ezTextureAtlasPage() = default;

  ezUInt32 Width = 0;
  ezUInt32 Height = 0;
};

class ezTextureAtlasUtility
{
public:
  static void CreateTextureAtlasLayout(ezDynamicArray<ezTextureAtlasElement>& elements, ezDynamicArray<ezTextureAtlasPage>& outPages, ezUInt32 width, ezUInt32 height, ezUInt32 maxWidth, ezUInt32 maxHeight, bool powerOfTwo = false);
};
