#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Math, Mat4)
{
  EZ_TEST_BLOCK(true, "Inverse")
  {
    ezMat4 scale(5.0f, 0.0f, 0.0f, 0.0f,
                 0.0f, 5.0f, 0.0f, 0.0f,
                 0.0f, 0.0f, 5.0f, 0.0f,
                 0.0f, 0.0f, 0.0f, 5.0f);

    ezMat4 inverseScale = scale.GetInverse();
    ezMat4 identity = scale * inverseScale;
    EZ_TEST(identity.IsIdentity() == true);
  }
}