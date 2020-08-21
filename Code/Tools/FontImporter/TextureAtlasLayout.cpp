#include <FontImporterPCH.h>

#include <TextureAtlasLayout.h>


bool ezTextureAtlasLayout::AddElement(ezTextureAtlasElement& element)
{
  if (element.Input.Width == 0 || element.Input.Height == 0)
  {
    element.Output.x = 0;
    element.Output.y = 0;

    return true;
  }

  if (!AddToNode(m_RootNode.Borrow(), element, false))
  {
    if (!AddToNode(m_RootNode.Borrow(), element, true))
    {
      return false;
    }
  }

  if (m_PowerOfTwo)
  {
    m_Width = ezMath::Max(m_Width, ezMath::PowerOfTwo_Ceil(element.Output.x + element.Input.Width));
    m_Height = ezMath::Max(m_Height, ezMath::PowerOfTwo_Ceil(element.Output.y + element.Input.Height));
  }
  else
  {
    m_Width = ezMath::Max(m_Width, element.Output.x + element.Input.Width);
    m_Height = ezMath::Max(m_Height, element.Output.y + element.Input.Height);
  }

  return true;
}

void ezTextureAtlasLayout::Clear()
{
  m_RootNode.Clear();

  m_RootNode = EZ_DEFAULT_NEW(ezTextureAtlasNode, 0, 0, m_Width, m_Height);

  m_Width = m_InitialWidth;
  m_Height = m_InitialHeight;
}

bool ezTextureAtlasLayout::AddToNode(ezTextureAtlasNode* node, ezTextureAtlasElement& element, bool allowGrowth)
{
  float aspect = node->Width / (float)node->Height;

  if (node->Left && node->Right)
  {
    if (AddToNode(node->Left.Borrow(), element, allowGrowth))
    {
      return true;
    }

    return AddToNode(node->Right.Borrow(), element, allowGrowth);
  }
  else
  {
    if (node->Full)
    {
      return false;
    }

    if (element.Input.Width > node->Width || element.Input.Height > node->Height)
    {
      return false;
    }

    if (!allowGrowth)
    {
      if (node->x + element.Input.Width > m_Width || node->y + element.Input.Height > m_Height)
        return false;
    }

    if (element.Input.Width == node->Width && element.Input.Height == node->Height)
    {
      element.Output.x = node->x;
      element.Output.y = node->y;
      node->Full = true;

      return true;
    }

    float dw = (float)(node->Width - element.Input.Width);
    float dh = (node->Height - element.Input.Height) * aspect;

    if (dw > dh)
    {
      node->Left = EZ_DEFAULT_NEW(ezTextureAtlasNode, node->x, node->y, element.Input.Width, node->Height);
      node->Right = EZ_DEFAULT_NEW(ezTextureAtlasNode, node->x + element.Input.Width, node->y, node->Width - element.Input.Width, node->Height);
    }
    else
    {
      node->Left = EZ_DEFAULT_NEW(ezTextureAtlasNode, node->x, node->y, node->Width, element.Input.Height);
      node->Right = EZ_DEFAULT_NEW(ezTextureAtlasNode, node->x, node->y + element.Input.Height, node->Width, node->Height - element.Input.Height);
    }

    return AddToNode(node->Left.Borrow(), element, allowGrowth);
  }
}

void ezTextureAtlasUtility::CreateTextureAtlasLayout(ezDynamicArray<ezTextureAtlasElement>& elements, ezDynamicArray<ezTextureAtlasPage>& outPages, ezUInt32 width, ezUInt32 height, ezUInt32 maxWidth, ezUInt32 maxHeight, bool powerOfTwo)
{
  // Preserve original index before sorting
  for (ezUInt32 i = 0; i < elements.GetCount(); i++)
  {
    elements[i].Output.Index = i;
    elements[i].Output.Page = -1;
  }

  elements.Sort([](const ezTextureAtlasElement& left, const ezTextureAtlasElement& right) -> bool {
    return left.Input.Width * left.Input.Height > right.Input.Width * right.Input.Height;
  });

  ezDynamicArray<ezTextureAtlasLayout> layouts;

  ezUInt32 remainingCount = elements.GetCount();

  while (remainingCount > 0)
  {
    layouts.PushBack(std::move(ezTextureAtlasLayout(width, height, maxWidth, maxHeight, powerOfTwo)));

    ezTextureAtlasLayout& currentLayout = layouts.PeekBack();

    // Find largest unassigned element that fits
    ezUInt32 sizeLimit = ezMath::MaxValue<ezUInt32>();

    while (true)
    {
      ezUInt32 largestId = -1;

      // Assumes elements are sorted from largest to smallest
      for (ezUInt32 i = 0; i < elements.GetCount(); i++)
      {
        if (elements[i].Output.Page == -1)
        {
          ezUInt32 size = elements[i].Input.Width * elements[i].Input.Height;

          if (size < sizeLimit)
          {
            largestId = i;
            break;
          }
        }
      }

      if (largestId == -1)
      {
        // Nothing fits, start a new page
        break;
      }

      ezTextureAtlasElement& element = elements[largestId];

      // Check if an element is too large to ever fit
      if (element.Input.Width > maxWidth || element.Input.Height > maxHeight)
      {
        ezLog::Warning("Some of the provided elements don't fit in an atlas of provided size. Returning empty array of pages.");

        outPages.Clear();
        return;
      }

      if (currentLayout.AddElement(element))
      {
        element.Output.Page = layouts.GetCount() - 1;
        remainingCount--;
      }
      else
      {
        sizeLimit = element.Input.Width * element.Input.Height;
      }
    }
  }

  outPages.Reserve(layouts.GetCount());

  for (auto& layout : layouts)
  {
    outPages.PushBack({layout.GetWidth(), layout.GetHeight()});
  }
}
