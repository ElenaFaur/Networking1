#include <gtest/gtest.h>
#include "olc_net.h"

using namespace std::chrono_literals;

enum class CustomMsgTypes : uint32_t{   //defining custom message types

    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class CustomServer : public olc::net::server_interface<CustomMsgTypes>
{
    public:
        CustomServer(uint16_t nPort) : olc::net::server_interface<CustomMsgTypes>(nPort){};

    public:
        //finally, all it does is provide implementations for the three functions we expect the user to override

        virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client){

            // informing the client that it has been accepted
            // we'll create a message "ServerAccept" and we'll send it to the client
            olc::net::message<CustomMsgTypes> msg;
            msg.header.id = CustomMsgTypes::ServerAccept;
            client -> Send(msg);
            
            return true;
        }

};

class CustomClient : public olc::net::client_interface<CustomMsgTypes>
{
};

// --- tests:

//starting server check
TEST(ServerConnectTest, ServerStartCheck){

    CustomServer *servptr = new CustomServer(60000);

    ASSERT_TRUE(servptr -> Start()); 

    servptr -> Stop();
    delete servptr;
}

//stopping server check
TEST(ServerConnectTest, ServerStopCheck){

    CustomServer *servptr = new CustomServer(60000);

    servptr -> Start();

    ASSERT_TRUE(servptr -> Stop());
    delete servptr;
}

//checking if client successfully connected to server
TEST(ClientConnectTest, ClientConnectCheck){

    CustomServer *servptr = new CustomServer(60000);
    CustomClient *clientptr = new CustomClient;
    
    servptr -> Start();
    std::this_thread::sleep_for(500ms);

    ASSERT_TRUE(clientptr -> Connect("127.0.0.1", 60000));
    std::this_thread::sleep_for(500ms);
    
    ASSERT_TRUE(servptr -> OnClientConnect(servptr -> m_deqConnections.back()));
    ASSERT_TRUE(clientptr -> IsConnected());
    

    servptr -> Stop();

    delete servptr;
    delete clientptr;
}

//checking if a client didn't connect to server
TEST(ClientConnectTest, ClientNotConnectCheck){
 
    CustomServer *servptr = new CustomServer(60000);
    CustomClient *clientptr = new CustomClient;

    servptr -> Start();
    std::this_thread::sleep_for(500ms);

    ASSERT_FALSE(clientptr -> IsConnected());

    servptr -> Stop();

    delete servptr;
    delete clientptr;
}

//checking one/first client ID
TEST(ClientConnectTest, ClientIDCheck){

    CustomServer *servptr = new CustomServer(60000);
    CustomClient *clientptr = new CustomClient;

    servptr -> Start();
    std::this_thread::sleep_for(500ms);

    clientptr -> Connect("127.0.0.1", 60000);
    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(10000, servptr -> m_deqConnections.back() -> GetID());
    delete servptr;
    delete clientptr;
}

TEST(ClientConnectTest, ClientsIDsCheck){

    CustomServer *servptr = new CustomServer(60000);
    CustomClient *clientptr1 = new CustomClient;
    CustomClient *clientptr2 = new CustomClient;
    CustomClient *clientptr3 = new CustomClient;

    servptr -> Start();
    std::this_thread::sleep_for(500ms);

    clientptr1 -> Connect("127.0.0.1", 60000);
    std::this_thread::sleep_for(500ms);

    clientptr2 -> Connect("127.0.0.1", 60000);
    std::this_thread::sleep_for(500ms);

    clientptr3 -> Connect("127.0.0.1", 60000);
    std::this_thread::sleep_for(500ms);

    uint32_t IDcounter = 10000;

    for(auto client : servptr -> m_deqConnections){

        ASSERT_EQ(IDcounter, client -> GetID());
        IDcounter++;
    }

    servptr -> Stop();

    delete servptr;
    delete clientptr1;
    delete clientptr2;
    delete clientptr3;
}

int main(int argc, char **argv) 
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}