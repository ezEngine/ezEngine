// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <lttngh/LttngHelpers.h>
#include <arpa/inet.h>
#include <stdio.h> // sprintf

void lttngh_FormatIPv4(const void* pIPv4, char* buf16)
{
    buf16[0] = 0;
    inet_ntop(AF_INET, pIPv4, buf16, LTTNGH_FORMAT_IPV4_LEN);
    assert(strlen(buf16) < LTTNGH_FORMAT_IPV4_LEN);
    buf16[LTTNGH_FORMAT_IPV4_LEN - 1] = 0;
}

void lttngh_FormatIPv6(const void* pIPv6, char* buf48)
{
    buf48[0] = 0;
    inet_ntop(AF_INET6, pIPv6, buf48, LTTNGH_FORMAT_IPV6_LEN);
    assert(strlen(buf48) < LTTNGH_FORMAT_IPV6_LEN);
    buf48[LTTNGH_FORMAT_IPV6_LEN - 1] = 0;
}

void lttngh_FormatSockaddr(const void* pSockaddr, unsigned cbSockaddr,
                           char* buf65)
{
    static unsigned const SizeOfInet4ThroughAddr = offsetof(struct sockaddr_in, sin_zero);
    static unsigned const SizeOfInet6ThroughAddr = offsetof(struct sockaddr_in6, sin6_scope_id);
    static unsigned const SizeOfInet6ThroughScope = offsetof(struct sockaddr_in6, sin6_scope_id) + sizeof(uint32_t);
    static unsigned const MaxHexDigits = LTTNGH_FORMAT_SOCKADDR_LEN - sizeof("0x");
    char* pOut = buf65;
    unsigned i, cb;

    if (cbSockaddr >= 2)
    {
        switch (((struct sockaddr const*)pSockaddr)->sa_family)
        {
        case AF_INET:

            if (cbSockaddr >= SizeOfInet4ThroughAddr)
            {
                struct sockaddr_in const* const pSock = (struct sockaddr_in const*)pSockaddr;
                lttngh_FormatIPv4(&pSock->sin_addr, pOut);

                if (pSock->sin_port != 0)
                {
                    pOut += strlen(pOut);
                    sprintf(pOut, ":%u",
                        ntohs(pSock->sin_port));
                }

                goto Done;
            }
            break;

        case AF_INET6:

            if (cbSockaddr >= SizeOfInet6ThroughAddr)
            {
                struct sockaddr_in6 const* const pSock = (struct sockaddr_in6 const*)pSockaddr;

                if (pSock->sin6_port != 0)
                {
                    *pOut++ = '[';
                }

                lttngh_FormatIPv6(&pSock->sin6_addr, pOut);

                if (cbSockaddr >= SizeOfInet6ThroughScope &&
                    pSock->sin6_scope_id != 0)
                {
                    pOut += strlen(pOut);
                    sprintf(pOut, "%%%u",
                        ntohl(pSock->sin6_scope_id));
                }

                if (pSock->sin6_port != 0)
                {
                    pOut += strlen(pOut);
                    sprintf(pOut, "]:%u",
                        ntohs(pSock->sin6_port));
                }

                goto Done;
            }
            break;
        }
    }

    *pOut++ = '0';
    *pOut++ = 'x';
    *pOut = 0;

    cb = cbSockaddr < (MaxHexDigits / 2) ? cbSockaddr : (MaxHexDigits / 2);
    for (i = 0; i != cb; i += 1)
    {
        sprintf(pOut, "%02X", ((unsigned char const*)pSockaddr)[i]);
        pOut += 2;
    }

Done:

    assert(strlen(buf65) < LTTNGH_FORMAT_SOCKADDR_LEN);
    buf65[LTTNGH_FORMAT_SOCKADDR_LEN - 1] = 0;
}
