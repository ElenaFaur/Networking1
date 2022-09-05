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

/*
@brief Server perspective
Checking if the server started successfully
*/
TEST(TestServerConnect, ServerStartCheck)
{
    
    CustomServer *serverpointer = new CustomServer(60000);
    
    ASSERT_TRUE(serverpointer -> Start()); 

    serverpointer -> Stop();
    delete serverpointer;
}

/*
@brief Server perspective
Testing if the server stopped correctly without any failures after it started
*/
TEST(TestServerConnect, ServerStopCheck)
{

    CustomServer *serverpointer = new CustomServer(60000);
    
    serverpointer -> Start();
    
    ASSERT_TRUE(serverpointer -> Stop());
    
    delete serverpointer;
}

/*
    @brief Client-server connection
    Testing a potential client could establish a connection with the running server
    and checking if the connection was established between the client and the server
*/
TEST(TestClientConnect, ClientConnectCheck)
{

    CustomServer *serverpointer = new CustomServer(60000);
    
    CustomClient *client = new CustomClient;
    
    serverpointer -> Start();
    std::this_thread::sleep_for(500ms);

    ASSERT_TRUE(client -> Connect("127.0.0.1", 60000)); 
    std::this_thread::sleep_for(500ms);
    
    ASSERT_TRUE(serverpointer -> OnClientConnect(serverpointer -> m_deqConnections.back()));
    
    ASSERT_TRUE(client -> IsConnected());
    

    serverpointer -> Stop();

    delete serverpointer;
    delete client;
}

/*
    @brief Client-server connection
    Testing from a client perspective if the server-client connection wasn't yet established
*/
TEST(TestClientConnect, ClientNotConnectCheck)
{
 
    CustomServer *serverpointer = new CustomServer(60000);
    CustomClient *client = new CustomClient;

    serverpointer -> Start();
    std::this_thread::sleep_for(500ms);

    ASSERT_FALSE(client -> IsConnected());

    serverpointer -> Stop();

    delete serverpointer;
    delete client;
}

/*
    @brief Client-server relationship
    Checking if a potential client connected to server has the default ID assigned correctly
    Every client must have assigned an unique ID starting from 10000 
    The ID increments by one everytime when a new client connects to server
*/
TEST(TestClientConnect, ClientIDCheck)
{

    CustomServer *serverpointer = new CustomServer(60000);
    CustomClient *client = new CustomClient;

    serverpointer -> Start();
    std::this_thread::sleep_for(500ms);

    client -> Connect("127.0.0.1", 60000);
    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(10000, serverpointer -> m_deqConnections.back() -> GetID());
    delete serverpointer;
    delete client;
}
/*
    @brief Client-server relationship
    Testing if multiple clients have assigned the correct ID
*/
TEST(TestClientConnect, ClientsIDsCheck)
{

    CustomServer *serverpointer = new CustomServer(60000);
    CustomClient *client1 = new CustomClient;
    CustomClient *client2 = new CustomClient;
    CustomClient *client3 = new CustomClient;

    serverpointer -> Start();
    std::this_thread::sleep_for(500ms);

    client1 -> Connect("127.0.0.1", 60000);
    std::this_thread::sleep_for(500ms);

    client2 -> Connect("127.0.0.1", 60000);
    std::this_thread::sleep_for(500ms);

    client3 -> Connect("127.0.0.1", 60000);
    std::this_thread::sleep_for(500ms);

    uint32_t IDcounter = 10000;

    for(auto client : serverpointer -> m_deqConnections){

        ASSERT_EQ(IDcounter, client -> GetID());
        IDcounter++;
    }

    serverpointer -> Stop();

    delete serverpointer;
    delete client1;
    delete client2;
    delete client3;
}

int main(int argc, char **argv) 
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}