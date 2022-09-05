#include "custom_server.h"

CustomServer::CustomServer(uint16_t nPort) : olc::net::server_interface<CustomMsgTypes>(nPort){};

bool CustomServer::OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client){

    //we'll create a message "ServerAccept" and we'll send it to the client
    olc::net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::ServerAccept;
    client -> Send(msg);
    
    return true;
}

bool CustomServer::OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client){

    //useful to know when a server removes a client
    std::cout << "Removing client [" << client -> GetID() << "]\n";
    return true;
}

void CustomServer::OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client, olc::net::message<CustomMsgTypes>& msg){

    switch(msg.header.id){

                case CustomMsgTypes::ServerPing:{
                    
                    std::cout << "[" << client -> GetID() << "]: Server Ping\n";

                    //simply bounce the message back to client
                    client -> Send(msg);

                }
                break;

                case CustomMsgTypes::MessageAll:{

                    std::cout << "[" << client -> GetID() << "]: Message All\n";
                    olc::net::message<CustomMsgTypes> msg;
                    msg.header.id = CustomMsgTypes::ServerMessage;
                    //put in the body the client id that sent the request in the first place
                    msg << client -> GetID();   //GetID() is const, so the type of 2nd paramter in "<<" overload should be so
                    
                    //specifying the message but also tells it to explicitly ignore the client that 
                    // -- sent the message in the first place
                    MessageAllClients(msg, client);     //function from server interface
                }
                break;
            }
}
