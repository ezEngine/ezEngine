/// \brief Many Linux APIs will fill out error on failure. This converts the error into an error code and a human-readable error message.
/// Pass in the linux `errno` symbol. Be careful when printing multiple values, a function could clear `errno` as a side-effect so it is best to store it in a temp variable before printing a complex error message.
/// You may have to include #include <errno.h> use this.
/// \sa https://man7.org/linux/man-pages/man3/errno.3.html
struct ezArgErrno
{
  inline explicit ezArgErrno(ezInt32 iErrno)
    : m_iErrno(iErrno)
  {
  }

  ezInt32 m_iErrno;
};

EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgErrno& arg);

struct ezArgErrorCode
{
  inline explicit ezArgErrorCode(ezUInt32 uiErrorCode)
    : m_ErrorCode(uiErrorCode)
  {
  }

  ezUInt32 m_ErrorCode;
};

EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgErrorCode& arg);
