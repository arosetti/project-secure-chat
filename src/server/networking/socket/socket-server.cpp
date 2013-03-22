#include "socket-server.h"

#include "session-manager.h"
#include "session.h"

SocketServer::SocketServer(NetworkManager& netmanager, uint32 d) throw(SocketException): 
    MethodRequest(), m_netmanager(netmanager), m_diff(d), active(true)     
{

}

SocketServer::~SocketServer()
{
    ::close(sock_listen);
    sock_listen = INVALID_SOCKET;
}

void SocketServer::Init(int port) throw(SocketException)
{
    SetupSocket(port);
    SetupEpoll();
}

void SocketServer::SetBlocking(int sock, const bool block)
    throw(SocketException)
{
    int flags;
 
    if(!block)
        INFO("debug", "setting socket %d non-blocking\n", sock);

    if ((flags = fcntl (sock, F_GETFL, 0)) < 0)
        throw SocketException("[setBlocking() -> fnctl()]", true);

    if (!block)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK ;

    if(fcntl (sock, F_SETFL, flags) < 0)
        throw SocketException("[setBlocking() -> fnctl()]", true);
}

void SocketServer::SetupAddrInfo(int family, int socktype, int protocol)
{
    memset(&serverinfo, 0, sizeof(struct addrinfo));
    serverinfo.ai_family   = family;
    serverinfo.ai_socktype = socktype;
    //serverinfo.ai_protocol = protocol;
    serverinfo.ai_flags    = AI_PASSIVE;
}

void SocketServer::SetupSocket(int port) throw(SocketException)
{
    stringstream s_port;
    int yes = 1;
    struct addrinfo *ai_res;

    SetupAddrInfo(AF_UNSPEC, SOCK_STREAM , 0);

    s_port << port;

    if (getaddrinfo (NULL, s_port.str().c_str(), &serverinfo, &serverinfo_res) != 0)
        throw SocketException("[getaddrinfo()]", true);

    for (ai_res = serverinfo_res; ai_res != NULL; ai_res = ai_res->ai_next)
    {
        if ((sock_listen = socket(ai_res->ai_family, ai_res->ai_socktype, ai_res->ai_protocol)) < 0)
            continue;

        if (bind(sock_listen, ai_res->ai_addr, ai_res->ai_addrlen) == 0)
        {
            INFO("debug", "succesful bind!\n");
            if (serverinfo_res == NULL)
                throw SocketException("bind failed!", false);
            
            freeaddrinfo (serverinfo_res);

            if (setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
                throw SocketException("[setsockopt()]", true);

            SetBlocking(sock_listen, false);

            if (listen(sock_listen, SOMAXCONN) < 0)
                throw SocketException("[listen()]", true);
            break;
        }    
        
        INFO("debug", "bind failed!!\n");
        close(sock_listen);
    }
}

void SocketServer::SetupEpoll() throw(SocketException)
{
    events = (struct epoll_event*) calloc(MAXEVENTS, sizeof(event));

    if ((epoll_fd = epoll_create1(0)) < 0)
        throw SocketException("[epoll_create()]", true);

    event.events = EPOLLIN | EPOLLET;
    event.data.fd = sock_listen;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_listen, &event) < 0)
        throw SocketException("[epoll_ctl()]", true);
}

int SocketServer::Call()
{
    Init(CFG_GET_INT("server_port"));
    //InitCallback(&handle_session_manager_task);

    INFO("debug", "* listening on port: %d\n", CFG_GET_INT("server_port"));

    int res = -1, sock_new, nbytes, i = 0;
    
    INFO("debug", "* epoll thread started\n");

    while(1)
    {
        try
        {
            if ((res = epoll_wait(epoll_fd, events, MAXEVENTS, -1)) < 0)
                throw SocketException("[epoll_wait()]", true);

            INFO("debug", "* epollwait res %d\n",res);

            for (i = 0; i < res; i++)
            {
                if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN)))
                {
                    INFO("debug", "epoll error\n");
                    close(events[i].data.fd);
                    continue;
                }
                else if (sock_listen == events[i].data.fd)
                {
                    INFO("debug", "accepting\n");
                    while (1)
                    {
                        struct sockaddr in_addr;
                        socklen_t       in_len;
                        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                        in_len = sizeof in_addr;

                        if ((sock_new = accept (sock_listen, &in_addr, &in_len)) < 0)
                        {
                            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                                break;
                            else
                            {
                                perror("accept");
                                break;
                            }
                        }

                        if (getnameinfo(&in_addr, in_len,
                                        hbuf, sizeof hbuf,
                                        sbuf, sizeof sbuf,
                                        NI_NUMERICHOST | NI_NUMERICSERV) == 0)
                        {
                            INFO("debug", "client_new %d "
                                   "(host=%s, port=%s)\n", sock_new, hbuf, sbuf);
                        }
                        else
                            throw SocketException("[getnameinfo()]", true);

                        SetBlocking(sock_new, false);
                        event.data.fd = sock_new;
                        event.events = EPOLLIN | EPOLLET;

                        if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, sock_new, &event) < 0)
                            throw SocketException("[epoll_ctl()]", true);

                        INFO("debug","epoll create session\n");

                        s = new Session_smart(s_manager->AddSession(sock_new));

                        //s_manager->AddSession(sock_new);
                        //cb_notify(new_connection_net_task());
                    }

                    continue;
                }
                else
                {                    
                    m_netmanager.QueueRecive(*s);
                }
            }
        }
        catch(SocketException e)
        {
            cout << e.what() << endl;
        }
    }
    
    close(sock_new);
    return 0;
}
