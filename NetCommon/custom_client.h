#include <iostream>
#include "olc_net.h"

enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

class CustomClient : public olc::net::client_interface<CustomMsgTypes>
{
    public:
    	void PingServer();
        void MessageAll();
};