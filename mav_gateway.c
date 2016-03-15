/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file mav_gateway.c
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "err_code_def.h"
#include "phy_linker.h"
#include "platform_def.h"
#include <pthread.h>


#include "mleak_check.h"
//=============================================================================
//                Constant Definition
//=============================================================================
#define MAX_FORWARD_NUM                     5
#define MAX_STRING_SIZE                     64

#define PERIOD_HEARTBEAT_SEC                1
#define PERIOD_CHECK_LINK_STATUS_MSEC       330
//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
/**
 *  device setup info
 */
typedef struct dev_item
{
    char            dev_name[MAX_STRING_SIZE];

    union {
        struct {
            unsigned int     baudrate;
        } serial;

        struct {
            unsigned int     port_num;
        } network_addr;
    } u;

    pl_type_t       pl_type;
    pl_handle_t     *pHPl;

} dev_item_t;

/**
 *  mav gateway mgr
 */
typedef struct mav_gateway
{
    dev_item_t      master;

    int             forwarding_cnt;
    dev_item_t      forwarding_out[MAX_FORWARD_NUM];

} mav_gateway_t;
//=============================================================================
//                Global Data Definition
//=============================================================================
static mav_gateway_t    g_mav_gateway = {.forwarding_cnt = 0,};

/**
 *  for local test
 */
pthread_mutex_t  g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
//=============================================================================
//                Private Function Definition
//=============================================================================
static void
_usage(char *progname)
{
    log_msg("Usage: %s [operation]\n"
            "\t--master:    Specifies which port (serial, USB or network address/port) the UAV is communicating on.\n"
            "\t                 e.g. /dev/ttyS1,57600, or tcp:127.0.0.1:14550 \n"
            "\t--out:       Forward the MAVLink packets to a remote device (serial, USB or network address/port).\n"
            "\n",
            progname);
    return ;
}

static int
_check_dev_item_type(
    dev_item_t  *pDev_item)
{
    int     result = 0;

    do{
        if( pDev_item->dev_name[0] == '/' )
        { // serial or USB
            int             rval = 0;
            char            *pCur = 0;
            unsigned int    baudrate = 0;

            pCur = strchr(pDev_item->dev_name, ',');
            if( !pCur )
            {
                err_msg("err, wrong format in serial device !\n");
                result = ERRCODE_INVALID_PARAM;
                break;
            }

            *pCur = '\0';
            pCur++;

            rval = sscanf(pCur, "%u", &baudrate);
            if( rval != 1 )
            {
                err_msg("err, wrong format in serial device !\n");
                result = ERRCODE_INVALID_PARAM;
                break;
            }

            pDev_item->u.serial.baudrate = baudrate;
            pDev_item->pl_type           = PL_TYPE_UART;
        }
        else if( tolower(pDev_item->dev_name[0]) == 'u' ||
                 tolower(pDev_item->dev_name[0]) == 't' )
        { // network address/port
            int             rval = 0;
            char            *pCur = 0;
            unsigned int    port_num = 0;
            pl_type_t       pl_type = PL_TYPE_UNKNOW;

            pCur = strchr(pDev_item->dev_name, ':');
            if( !pCur )
            {
                err_msg("err, wrong format in network address !\n");
                result = ERRCODE_INVALID_PARAM;
                break;
            }
            *pCur = '\0';
            pCur++;

            if( 0 == strnicmp(pDev_item->dev_name, "udp", 3) )
                pl_type = PL_TYPE_UDP;
            else if( 0 == strnicmp(pDev_item->dev_name, "tcp", 3) )
                pl_type = PL_TYPE_TCP;
            else
            {
                err_msg("err, wrong format in network address !\n");
                result = ERRCODE_INVALID_PARAM;
                break;
            }

            snprintf(pDev_item->dev_name, MAX_STRING_SIZE, "%s", pCur);

            pCur = strrchr(pDev_item->dev_name, ':');
            if( !pCur )
            {
                err_msg("err, wrong format in network address !\n");
                result = ERRCODE_INVALID_PARAM;
                break;
            }

            *pCur = '\0';
            pCur++;

            rval = sscanf(pCur, "%u", &port_num);
            if( rval != 1 )
            {
                err_msg("err, wrong format in network address !\n");
                result = ERRCODE_INVALID_PARAM;
                break;
            }

            pDev_item->u.network_addr.port_num = port_num;
            pDev_item->pl_type                 = pl_type;
        }
        else
        {
            result = ERRCODE_INVALID_PARAM;
            err_msg("err, un-know device name !\n");
        }
    }while(0);

    return result;
}

static int
_parse_args(
    int             argc,
    char            **argv,
    mav_gateway_t   *pMgr)
{
    int     result = ERRCODE_OK;

    do{
        int     isBreak = 0;
        if( argc < 2 )
        {
            result = ERRCODE_INVALID_PARAM;
            _usage(argv[0]);
            break;
        }

        argv++; argc--;
        while( argc )
        {
            if (!strcmp(argv[0], "--master"))
            {
                dev_item_t  *pDev_item = &pMgr->master;

                argv++; argc--;

                snprintf(pDev_item->dev_name, MAX_STRING_SIZE, "%s", argv[0]);
                result = _check_dev_item_type(pDev_item);
                if( result )
                {
                    err_msg("err, wrong master setting !\n");
                    isBreak = 1;
                    break;
                }
            }
            else if (!strcmp(argv[0], "--out"))
            {
                dev_item_t  *pDev_item = 0;

                argv++; argc--;
                if( g_mav_gateway.forwarding_cnt == MAX_FORWARD_NUM )
                {
                    err_msg("err, Over forwarding num %d\n", g_mav_gateway.forwarding_cnt);
                    break;
                }

                pDev_item = &pMgr->forwarding_out[pMgr->forwarding_cnt];
                snprintf(pDev_item->dev_name, MAX_STRING_SIZE, "%s", argv[0]);
                result = _check_dev_item_type(pDev_item);
                if( result )
                {
                    err_msg("err, wrong out setting !\n");
                    isBreak = 1;
                    break;
                }
                g_mav_gateway.forwarding_cnt++;
            }
            else
            {
                // No match...
                _usage(argv[0]);
                isBreak = 1;
                result = ERRCODE_INVALID_PARAM;
                break;
            }

            argv++; argc--;
        }

        if( isBreak )
            break;
    }while(0);
    return result;
}
//=============================================================================
//                Public Function Definition
//=============================================================================
static void
_sig_handler(int sig)
{
    err_msg("receive signal %d\n", sig);

    exit(0);
    return;
}

int
main(int argc, char **argv)
{
    int                 result = 0;
    pl_init_info_t      init_info = {0};
    pl_trans_info_t     trans_info = {0};
    pl_recv_info_t      recv_info = {0};

#if defined(__MINGW32__)
    {
        WSADATA         wsaData;
        // Request Winsock version 2.2
        if ((result = WSAStartup(0x202, &wsaData)) != 0)
        {
            err_msg("Server: WSAStartup() failed with error %d\n", result);
            WSACleanup();
            return -1;
        }
    }
#endif

    // signal event handle
    signal(SIGINT, _sig_handler);
    signal(SIGTERM, _sig_handler);
#if !defined(__MINGW32__)
    signal(SIGHUP, _sig_handler);
    signal(SIGUSR1, _sig_handler);
    signal(SIGQUIT, _sig_handler);
    signal(SIGKILL, _sig_handler);
    signal(SIGSEGV, _sig_handler);
#endif

    if( _parse_args(argc, argv, &g_mav_gateway) )
    {
        err_msg("err, Wrong arguments !!!\n");
        return -1;
    }

    do{
        // create master handle
        if( g_mav_gateway.master.pl_type != PL_TYPE_UNKNOW )
        {
            dev_item_t  *pDev_item = &g_mav_gateway.master;

            pl_create_handle(&pDev_item->pHPl);
            if( !pDev_item->pHPl )
            {
                err_msg("err, master create pl handler fail !!\n");
                result = -2;
                break;
            }

            init_info.pl_type = pDev_item->pl_type;
            pl_init(pHPl, &init_info);
        }

        // create forward out handler

    }while(0);

//    pl_create_handle(&pHPl);
//
//
//    init_info.pl_type = PL_TYPE_TCP;
//    pl_init(pHPl, &init_info);
//
//    pl_trans(pHPl, &trans_info);
//
//    pl_recv(pHPl, &recv_info);
//
//    pl_deinit(pHPl);
//
//    pl_destroy_handle(&pHPl);

    // check memory leak
    mlead_dump();

#if defined(__MINGW32__)
    WSACleanup();
#endif
    return result;
}

