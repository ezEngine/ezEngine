#pragma once

#define XR_LOG(exp)                                                                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    auto s = (exp);                                                                                                   \
    if (s != XR_SUCCESS)                                                                                               \
    {                                                                                                                  \
      ezLog::Error("OpenXR failure: {0}:{1} {2}", __FILE__, __LINE__, s);                                                     \
    }                                                                                                                  \
  } while (false)

#define XR_CHECK_LOG(exp)                                                                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    auto s = (exp);                                                                                                   \
    if (s != XR_SUCCESS)                                                                                               \
    {                                                                                                                  \
      ezLog::Error("OpenXR failure: {0}:{1} {2}", __FILE__, __LINE__, s);                                                     \
      return;           \
    }                                                                                                                  \
  } while (false)

#define EZ_CHECK_XR(exp)                                                                                               \
  {                                                                                                                    \
    if (exp != XR_SUCCESS)                                                                                             \
    {                                                                                                                  \
      ezLog::Error("OpenXR failure: {0}:{1}", __FILE__, __LINE__);                                                     \
      return EZ_FAILURE;                                                                                               \
    }                                                                                                                  \
  }

#define XR_SUCCEED_OR_RETURN(code, cleanup)                                                                            \
  do                                                                                                                   \
  {                                                                                                                    \
    auto s = (code);                                                                                                   \
    if (s != XR_SUCCESS)                                                                                               \
    {                                                                                                                  \
      cleanup();                                                                                                       \
      return s;                                                                                                        \
    }                                                                                                                  \
  } while (false)

#define XR_SUCCEED_OR_RETURN_LOG(code, cleanup)                                                                        \
  do                                                                                                                   \
  {                                                                                                                    \
    auto s = (code);                                                                                                   \
    if (s != XR_SUCCESS)                                                                                               \
    {                                                                                                                  \
      ezLog::Error("OpenXR call '{0}' failed with: {1}", EZ_STRINGIZE(code), s);                                       \
      cleanup();                                                                                                       \
      return s;                                                                                                        \
    }                                                                                                                  \
  } while (false)

static void voidFunction() {}
