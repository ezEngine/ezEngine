
#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#include <Foundation/Communication/Implementation/Linux/PipeChannel_linux.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

#include <sys/socket.h>
#include <sys/un.h>

ezPipeChannel_linux::ezPipeChannel_linux(const char* szAddress, Mode::Enum Mode)
: ezIpcChannel(szAddress, Mode)
{
    ezStringBuilder pipePath = ezOSFile::GetTempDataFolder("ez-pipes");
    pipePath.AppendPath(szAddress);
    pipePath.Append(".server");

    m_serverSocketPath = pipePath;
    pipePath.Shrink(0, 7); // strip .server
    pipePath.Append(".client");
    m_clientSocketPath = pipePath;

    const char* thisSocketPath = (Mode == Mode::Server) ? m_serverSocketPath.GetData() : m_clientSocketPath.GetData();

    m_socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(m_socketFd == -1)
    {
        ezLog::Error("Failed to create unix domain socket. error {}", errno);
        return;
    }

    // If the socket file already exists, delete it
    ezOSFile::DeleteFile(thisSocketPath).IgnoreResult();

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;

    if(strlen(thisSocketPath) >= EZ_ARRAY_SIZE(addr.sun_path) - 1)
    {
        ezLog::Error("Given ipc channel address is to long. Resulting path '{}' path length limit {}", strlen(thisSocketPath), EZ_ARRAY_SIZE(addr.sun_path) - 1);
        close(m_socketFd);
        m_socketFd = -1;
        return;
    }

    strcpy(addr.sun_path, thisSocketPath);
    if(bind(m_socketFd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        ezLog::Error("Failed to bind unix domain socket to '{}' error {}", thisSocketPath, errno);
        close(m_socketFd);
        m_socketFd = -1;
        return;
    }
}

void ezPipeChannel_linux::InternalConnect()
{
    if(m_Connected)
    {
        return;
    }

    if(m_Mode == Mode::Server)
    {
        m_pipeFd = open(m_pipePath.GetData(), O_WRONLY | O_NONBLOCK);
        if(m_pipeFd < 0)
        {
            ezLog::Error("Failed to open named pipe '{}' for writing", m_pipePath);
            return;
        }
    }
    else
    {
        m_pipeFd = open(m_pipePath.GetData(), O_RDONLY);
        if(m_pipeFd < 0)
        {
            ezLog::Error("Failed to open named pipe '{}' for reading", m_pipePath);
            return;
        }
    }
    m_Connected = true;
}

#endif