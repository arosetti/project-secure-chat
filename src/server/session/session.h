#ifndef _SESSION_H
#define _SESSION_H

#include "session/sessionbase.h"
#include "utility/counted_ptr.h"
#include "packetfilter.h"

class Session;
typedef counted_ptr<Session> Session_smart;

class Session : public SessionBase
{
    public:
        Session(int pSock);
        ~Session();

        bool Update(uint32 diff, PacketFilter& updater);

        // THREADUNSAFE
        void KickSession();
        void SetId(uint32 id) { m_id = id; }
        void setSmartPointer(Session_smart m_ses);
        void deleteSmartPointer();

        // THREADSAFE 
        bool IsInChannel() { return channel_name == "" ? false : true; }        
        uint32 GetId() { return m_id; }
        void SendWaitQueue(int position);
        void SetInQueue(bool state) { m_inQueue = state; }

        // Handle
        void Handle_Ping(Packet& packet); 
        void Handle_ServerSide(Packet& packet);
        void HandleMessage(Packet& packet); 
        void HandleJoinChannel(Packet& packet);       
  
    private:

        virtual int _SendPacket(const Packet& pct);

        uint32 m_id;
        Session_smart smartThis;
        bool m_inQueue;
        // Channel
        std::string channel_name;
};

#endif
