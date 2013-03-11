#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Math/Mat4.h>

EZ_CREATE_SIMPLE_TEST(Math, Mat3)
{
  EZ_TEST_BLOCK(true, "Inverse")
  {
     ezMat3 scale(5.0f, 0.0f, 0.0f,
                  0.0f, 5.0f, 0.0f,
                  0.0f, 0.0f, 5.0f);

     ezMat3 inverseScale = scale.GetInverse();
     ezMat3 identity = scale * inverseScale;
     EZ_TEST(identity.IsIdentity() == true);
  }
}

