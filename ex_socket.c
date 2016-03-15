/* Server program example for IPv4 */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ctype.h>
#include <signal.h>
#include "pthread.h"

#define DEFAULT_PORT 2007
// default TCP socket type
#define DEFAULT_PROTO SOCK_STREAM


/////////////////////////////////////////////////////
typedef struct arg_info
{
    int     socket_type;
    int     port;
    char    *pIP_address;
    int     loopflag;
    int     maxloop;

} arg_info_t;

/////////////////////////////////////////////////////
#define err(string, args...)    do{ pthread_mutex_lock(&g_log_mutex);                                       \
                                    fprintf(stderr, "[%s,#%d] " string , __FUNCTION__, __LINE__, ## args);  \
                                    pthread_mutex_unlock(&g_log_mutex);                                     \
                                }while(0)

#define msg(string, args...)    do{ pthread_mutex_lock(&g_log_mutex);                                       \
                                    fprintf(stderr, "[%s,#%d] " string , __FUNCTION__, __LINE__, ## args);  \
                                    pthread_mutex_unlock(&g_log_mutex);                                     \
                                }while(0)

/////////////////////////////////////////////////////
static int      g_bExit = 0;
static pthread_mutex_t  g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
/////////////////////////////////////////////////////
static void
_usage(char *progname)
{
    fprintf(stderr,"Usage: %s -p [protocol] -e [port_num] -i [ip_address] -l [iterations]\n", progname);
    fprintf(stderr,"Where:\n\t- [protocol] is one of TCP or UDP\n");
    fprintf(stderr,"\t- [port_num] is the port to listen on\n");
    fprintf(stderr,"\t- [ip_address] is the ip address (in dotted\n");
    fprintf(stderr,"\t  decimal notation) to bind to. But it is not useful here...\n");
    fprintf(stderr,"\t- [iterations] is the number of loops to execute.\n");
    fprintf(stderr,"\t- (-l by itself makes client run in an infinite loop,\n");
    fprintf(stderr,"\t- Hit Ctrl-C to terminate server program...\n");
    fprintf(stderr,"\t- The defaults are TCP, 2007 and INADDR_ANY.\n");
//    WSACleanup();
//    exit(1);
}

static int
_parse_arg(
    int         argc,
    char        **argv,
    arg_info_t  *pArg_info)
{
    int     result = 0;

    do{
        int     i;
        if( argc < 2 )
        {
            result = __LINE__;
            _usage(argv[0]);
            break;
        }

        for(i = 1; i < argc; i++)
        {
            // switches or options...
            if ((argv[i][0] == '-') || (argv[i][0] == '/'))
            {
                // Change to lower...if any
                switch(tolower(argv[i][1]))
                {
                    // if -p or /p
                    case 'p':
                        if (!stricmp(argv[i+1], "TCP"))
                            pArg_info->socket_type = SOCK_STREAM;
                        else if (!stricmp(argv[i+1], "UDP"))
                            pArg_info->socket_type = SOCK_DGRAM;
                        else
                        {
                             _usage(argv[0]);
                            result = __LINE__;
                            break;
                        }

                        i++;
                        break;

                    // if -i or /i, for server it is not so useful...
                    case 'i':
                        pArg_info->pIP_address = argv[++i];
                        break;

                    // if -e or /e
                    case 'e':
                        pArg_info->port = atoi(argv[++i]);
                        break;

                    case 'l':
                        pArg_info->loopflag =1;
                        if (argv[i+1])
                        {
                            if (argv[i+1][0] != '-')
                                pArg_info->maxloop = atoi(argv[i+1]);
                        }
                        else
                            pArg_info->maxloop = -1;
                        i++;
                        break;

                    // No match...
                    default:
                        _usage(argv[0]);
                        result = __LINE__;
                        break;
                }

                if( result )
                    break;
            }
            else
            {
                _usage(argv[0]);
                result = __LINE__;
                break;
            }
        }
    }while(0);

    return result;
}

static void*
_thread_server(void *argv)
{
    arg_info_t          *pArg_info = (arg_info_t*)argv;
    char                Buffer[128];
    int                 fromlen, retval;
    struct sockaddr_in  local, from;
    SOCKET              listen_socket, msgsock;

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = (!pArg_info->pIP_address) ? INADDR_ANY : inet_addr(pArg_info->pIP_address);

    /* Port MUST be in Network Byte Order */
    local.sin_port = htons(pArg_info->port);
    // TCP socket
    listen_socket = socket(AF_INET, pArg_info->socket_type, 0);

    if (listen_socket == INVALID_SOCKET)
    {
        err("Server: socket() failed with error %d\n", WSAGetLastError());
        g_bExit = 1;
        goto EXIT;
    }
    else
        msg("Server: socket() is OK.\n");

    // bind() associates a local address and port combination with the socket just created.
    // This is most useful when the application is a
    // server that has a well-known port that clients know about in advance.
    if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
    {
        err("Server: bind() failed with error %d\n", WSAGetLastError());
        g_bExit = 1;
        goto EXIT;
    }
    else
        msg("Server: bind() is OK.\n");

    // So far, everything we did was applicable to TCP as well as UDP.
    // However, there are certain steps that do not work when the server is
    // using UDP. We cannot listen() on a UDP socket.
    if (pArg_info->socket_type != SOCK_DGRAM)
    {
        if (listen(listen_socket, 5) == SOCKET_ERROR)
        {
            err("Server: listen() failed with error %d\n", WSAGetLastError());
            g_bExit = 1;
            goto EXIT;
        }
        else
            msg("Server: listen() is OK.\n");
    }
    msg("Server: I'm listening and waiting connection\non port %d, protocol %s\n",
           pArg_info->port, (pArg_info->socket_type == SOCK_STREAM) ? "TCP":"UDP");

    while( g_bExit == 0 )
    {
        fromlen = sizeof(from);
        // accept() doesn't make sense on UDP, since we do not listen()
        if (pArg_info->socket_type != SOCK_DGRAM)
        {
            msgsock = accept(listen_socket, (struct sockaddr*)&from, &fromlen);
            if (msgsock == INVALID_SOCKET)
            {
                err("Server: accept() error %d\n", WSAGetLastError());
                g_bExit = 1;
                goto EXIT;
            }
            else
                msg("Server: accept() is OK.\n");
            msg("Server: accepted connection from %s, port %d\n", inet_ntoa(from.sin_addr), htons(from.sin_port)) ;
        }
        else
            msgsock = listen_socket;

        // In the case of SOCK_STREAM, the server can do recv() and send() on
        // the accepted socket and then close it.
        // However, for SOCK_DGRAM (UDP), the server will do recvfrom() and sendto()  in a loop.
        if (pArg_info->socket_type != SOCK_DGRAM)
            retval = recv(msgsock, Buffer, sizeof(Buffer), 0);
        else
        {
            retval = recvfrom(msgsock, Buffer, sizeof(Buffer), 0, (struct sockaddr *)&from, &fromlen);
            msg("Server: Received datagram from %s\n", inet_ntoa(from.sin_addr));
        }

        if (retval == SOCKET_ERROR)
        {
            err("Server: recv() failed: error %d\n", WSAGetLastError());
            closesocket(msgsock);
            continue;
        }
        else
            msg("Server: recv() is OK.\n");

        if (retval == 0)
        {
            msg("Server: Client closed connection.\n");
            closesocket(msgsock);
            continue;
        }
        msg("Server: Received %d bytes, data \"%s\" from client\n", retval, Buffer);

        msg("Server: Echoing the same data back to client...\n");
        if (pArg_info->socket_type != SOCK_DGRAM)
            retval = send(msgsock, Buffer, sizeof(Buffer), 0);
        else
            retval = sendto(msgsock, Buffer, sizeof(Buffer), 0, (struct sockaddr *)&from, fromlen);

        if (retval == SOCKET_ERROR)
        {
            err("Server: send() failed: error %d\n", WSAGetLastError());
        }
        else
            msg("Server: send() is OK.\n");

        if (pArg_info->socket_type != SOCK_DGRAM)
        {
            msg("Server: I'm waiting more connection, try running the client\n");
            msg("Server: program from the same computer or other computer...\n");
            closesocket(msgsock);
        }
        else
            msg("Server: UDP server looping back for more requests\n");

        Sleep(1);
    }

EXIT:
    pthread_exit(NULL);
    return 0;
}

static void*
_thread_clint(void *argv)
{
    arg_info_t          *pArg_info = (arg_info_t*)argv;
    char                Buffer[128];
    char                *server_name= "localhost"; // default to localhost
    int                 i, loopcount, retval;
    unsigned int        addr;
    struct sockaddr_in  server;
    struct hostent      *hp;
    SOCKET              conn_socket;

    // Attempt to detect if we should call gethostbyname() or gethostbyaddr()
    if (isalpha(server_name[0]))
    {
        // server address is a name
        hp = gethostbyname(server_name);
    }
    else
    {
        // Convert nnn.nnn address to a usable one
        addr = inet_addr(server_name);
        hp = gethostbyaddr((char *)&addr, 4, AF_INET);
    }
    if (hp == NULL )
    {
        err("Client: Cannot resolve address \"%s\": Error %d\n", server_name, WSAGetLastError());
        g_bExit = 1;
        goto EXIT;
    }
    else
        msg("Client: gethostbyaddr() is OK.\n");

    // Copy the resolved information into the sockaddr_in structure
    memset(&server, 0, sizeof(server));
    memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = htons(pArg_info->port);

    conn_socket = socket(AF_INET, pArg_info->socket_type, 0); /* Open a socket */
    if (conn_socket <0 )
    {
        err("Client: Error Opening socket: Error %d\n", WSAGetLastError());
        g_bExit = 1;
        goto EXIT;
    }
    else
        msg("Client: socket() is OK.\n");

    // Notice that nothing in this code is specific to whether we
    // are using UDP or TCP.
    // We achieve this by using a simple trick.
    //    When connect() is called on a datagram socket, it does not
    //    actually establish the connection as a stream (TCP) socket
    //    would. Instead, TCP/IP establishes the remote half of the
    //    (LocalIPAddress, LocalPort, RemoteIP, RemotePort) mapping.
    //    This enables us to use send() and recv() on datagram sockets,
    //    instead of recvfrom() and sendto()
    msg("Client: Client connecting to: %s.\n", hp->h_name);
    if (connect(conn_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        err("Client: connect() failed: %d\n", WSAGetLastError());
        g_bExit = 1;
        goto EXIT;
    }
    else
        msg("Client: connect() is OK.\n");

    // Test sending some string
    loopcount = 0;

    while( g_bExit == 0 )
    {
        wsprintf(Buffer,"This is a test message from client #%d", loopcount++);
        retval = send(conn_socket, Buffer, sizeof(Buffer), 0);
        if (retval == SOCKET_ERROR)
        {
            err("Client: send() failed: error %d.\n", WSAGetLastError());
            g_bExit = 1;
            goto EXIT;
        }
        else
            msg("Client: send() is OK.\n");
        msg("Client: Sent data \"%s\"\n", Buffer);

        retval = recv(conn_socket, Buffer, sizeof(Buffer), 0);
        if (retval == SOCKET_ERROR)
        {
            err("Client: recv() failed: error %d.\n", WSAGetLastError());
            closesocket(conn_socket);
            g_bExit = 1;
            goto EXIT;
        }
        else
            msg("Client: recv() is OK.\n");

        // We are not likely to see this with UDP, since there is no
        // 'connection' established.
        if (retval == 0)
        {
            err("Client: Server closed connection.\n");
            closesocket(conn_socket);
            g_bExit = 1;
            goto EXIT;
        }

        msg("Client: Received %d bytes, data \"%s\" from server.\n", retval, Buffer);
        if (!pArg_info->loopflag)
        {
            msg("Client: Terminating connection...\n");
            g_bExit = 1;
            break;
        }
        else
        {
            if ((loopcount >= pArg_info->maxloop) && (pArg_info->maxloop >0))
                break;
        }

        Sleep(1);
    }

EXIT:
    pthread_exit(NULL);
    return 0;
}
////////////////////////////////////////////////
static void
_sig_handler(int sig)
{
    msg("receive signal %d\n", sig);
    g_bExit = 1;
    return;
}

int main(int argc, char **argv)
{
    int             result = 0;
    arg_info_t      arg_info = {0};
    WSADATA         wsaData;

    signal(SIGINT, _sig_handler);
    signal(SIGTERM, _sig_handler);

    // Request Winsock version 2.2
    if ((result = WSAStartup(0x202, &wsaData)) != 0)
    {
        err("Server: WSAStartup() failed with error %d\n", result);
        WSACleanup();
        return -1;
    }

    do{
        pthread_t   t_server, t_clint;


        result = _parse_arg(argc, argv, &arg_info);
        if( result )
            break;

        pthread_create(&t_server, NULL, _thread_server, (void*)&arg_info);
        pthread_create(&t_clint, NULL, _thread_clint, (void*)&arg_info);

        while( g_bExit == 0 )
            Sleep(2);

        pthread_join(t_server, NULL);
        pthread_join(t_clint, NULL);

    }while(0);

    WSACleanup();

    return 0;
}

#if 0
/**
 * C:\>myclient -
 * Usage: myclient -p [protocol] -n [server name/IP] -e [port_num] -l [iterations]
 * Where:
 *         protocol is one of TCP or UDP
 *         - server is the IP address or name of server
 *         - port_num is the port to listen on
 *         - iterations is the number of loops to execute.
 *         - (-l by itself makes client run in an infinite loop,
 *         - Hit Ctrl-C to terminate it)
 *         - The defaults are TCP , localhost and 2007
 *
 * C:\>myclient -p TCP -n 127.0.0.1 -e 5656 -l 3
 * Client: WSAStartup() is OK.
 * Client: gethostbyaddr() is OK.
 * Client: socket() is OK.
 * Client: Client connecting to: localhost.
 * Client: connect() is OK.
 * Client: send() is OK.
 * Client: Sent data "This is a test message from client #0"
 * Client: recv() is OK.
 * Client: Received 128 bytes, data "This is a test message from client #0" from server.
 * Client: send() is OK.
 * Client: Sent data "This is a test message from client #1"
 * Client: recv() failed: error 10053.
 *
 *
 * C:\>myclient -p TCP -n 127.0.0.1 -e 5656 -l 3
 * Client: WSAStartup() is OK.
 * Client: gethostbyaddr() is OK.
 * Client: socket() is OK.
 * Client: Client connecting to: localhost.
 * Client: connect() is OK.
 * Client: send() is OK.
 * Client: Sent data "This is a test message from client #0"
 * Client: recv() is OK.
 * Client: Received 128 bytes, data "This is a test message from client #0" from server.
 * Client: send() is OK.
 * Client: Sent data "This is a test message from client #1"
 * Client: recv() failed: error 10053.
 *
 * C:\>
 */

#endif
