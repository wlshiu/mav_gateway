/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file phy_linker_tcp.c
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/03/01
 * @license
 * @description
 */

#if defined(__MINGW32__)
    #define WIN32_LEAN_AND_MEAN
    #define _MSWSOCK_
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
#endif

#include "phy_linker_desc.h"
#include "phy_linker.h"

#include "platform_def.h"
#include "err_code_def.h"
//=============================================================================
//				  Constant Definition
//=============================================================================

//=============================================================================
//				  Macro Definition
//=============================================================================

//=============================================================================
//				  Structure Definition
//=============================================================================

//=============================================================================
//				  Global Data Definition
//=============================================================================

//=============================================================================
//				  Private Function Definition
//=============================================================================
#if defined(__MINGW32__)
static int
inet_pton(int af, const char *src, char *dst)
{
#define NS_INADDRSZ  4
    unsigned char tmp[NS_INADDRSZ], *tp;
    int saw_digit = 0;
    int octets = 0;
    int ch;

    if( AF_INET != af )
        return -1;

    *(tp = tmp) = 0;

    while ((ch = *src++) != '\0')
    {
        if (ch >= '0' && ch <= '9')
        {
            uint32_t n = *tp * 10 + (ch - '0');

            if (saw_digit && *tp == 0)
                return 0;

            if (n > 255)
                return 0;

            *tp = n;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return 0;
    }
    if (octets < 4)
        return 0;

    memcpy(dst, tmp, NS_INADDRSZ);

    return 1;
}
#endif

static int
linker_tcp_init(
    pl_mgr_t    *pMgr,
    pl_args_t   *pArgs)
{
    int     result = 0;

    dbg_msg("\n");
    return result;
}

static int
linker_tcp_deinit(
    pl_mgr_t    *pMgr)
{
    int     result = 0;
    dbg_msg("\n");

    return result;
}

static int
linker_tcp_trans(
    pl_mgr_t    *pMgr,
    pl_args_t   *pArgs)
{
    int     result = 0;
    dbg_msg("\n");
    return result;
}

static int
linker_tcp_revc(
    pl_mgr_t    *pMgr,
    pl_args_t   *pArgs)
{
    int     result = 0;
    dbg_msg("\n");
    return result;
}
//=============================================================================
//				  Public Function Definition
//=============================================================================
pl_desc_t   pl_desc_t_tcp_desc =
{
    .name   = "linker with tcp",
    .id     = (unsigned int)PL_TYPE_TCP,
    .init   = linker_tcp_init,
    .deinit = linker_tcp_deinit,
    .trans  = linker_tcp_trans,
    .recv   = linker_tcp_revc,
};
