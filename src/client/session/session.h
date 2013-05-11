#ifndef _SESSION_H
#define _SESSION_H

#include "session/sessionbase.h"
#include "../networking/socket-client.h"
#include "utility/counted_ptr.h"
#include "xml-message.h"

class Session;
typedef counted_ptr<Session> Session_smart;

class Session : public SessionBase
{
    SocketClient* c_Socket;
    string        s_pwd;
    string        f_key_pub, f_key_priv, f_server_key_pub;
  public:
    Session();
    ~Session();

    bool Connect();
    bool Disconnect();
    void ResetSocket();

    bool Update();    

    void HandleClientSide(Packet&);    
    void HandlePing(Packet&);
    void HandleMessage(Packet&);
    void HandleServerMessage(Packet&);
    void HandleJoinChannel(Packet&);
    void HandleLeaveChannel(Packet&);
    void HandleUpdateKey(Packet&);
    void HandleLogin(Packet& packet);
    bool HandleSend(const char* msg);

    const char* GetPassword();
    void SetPassword(const char *);
    bool HavePassword();
    void ClearPassword();
    void UpdateKeyFilenames();
    bool TestRsa();
        
    void SendToGui(std::string str, std::string nick, char type);

  private:
  
    bool Update(uint32 diff);
    int _SendPacket(Packet&);
};

#endif
