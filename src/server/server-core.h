#ifndef CORE_SERVER_H
#define CORE_SERVER_H

#include "../shared/common.h"
#include "../shared/networking/socket.h"
#include "../shared/threading/thread.h"
#include "session/sessionsmanager.h"

struct core_thread_params
{
    TCPSocket   *sock;
    //?
};

struct network_thread_params
{
    SessionsManager& sessions;
};

void start_server_core();
void* core_thread(void*);
void* network_thread(void*);

#endif
