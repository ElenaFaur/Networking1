#include "custom_client.h"

void CustomClient::PingServer(){

    olc::net::message<CustomMsgTypes> msg;
	msg.header.id = CustomMsgTypes::ServerPing;

	std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

	msg << timeNow;
	Send(msg);
}

void CustomClient::MessageAll(){

    //the client is going to send something that it wants the server to distribute
	// -- to all of the other clients
	olc::net::message<CustomMsgTypes> msg;
	msg.header.id = CustomMsgTypes::MessageAll;		
	Send(msg);
	//this is a bodyless message, we're just sending the header
}