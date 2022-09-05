#include <iostream>
#include "olc_net.h"

enum class CustomMsgTypes : uint32_t{   //defining custom message types

    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class CustomServer : public olc::net::server_interface<CustomMsgTypes>{

    public:
        CustomServer(uint16_t nPort);

        //informing the client that it has been accepted
        virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client);

        //called when a client appears to have disconnected
        virtual bool OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client);

        //called when a message arrives
        virtual void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client, olc::net::message<CustomMsgTypes>& msg);
};