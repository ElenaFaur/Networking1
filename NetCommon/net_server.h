#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace olc
{
    namespace net
    {
        template<typename T>
        class server_interface
        {
            public:
            // Create a server, ready to listen on specified port
                server_interface(uint16_t port):m_asioAcceptor(m_asioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port))
                {

                }

                virtual ~server_interface()
                {
                    Stop();
                }
                // Starts the server!
                bool Start()
                {
                    try
                    {
                        // Issue a task to the asio context - This is important
					// as it will prime the context with "work", and stop it
					// from exiting immediately. Since this is a server, we 
					// want it primed ready to handle clients trying to
					// connect.
                        WaitForClientConnection();

                        // Launch the asio context in its own thread

                        m_threadContext=std::thread([this](){m_asioContext.run();});
                    }
                    catch(std::exception& e)
                    {
                        //Something prohibited the server from listening
                        std::cerr <<"[SERVER] Exception: "<< e.what() << '\n';
                        return false;
                    }

                    std::cout<<"[SERVER] Started!\n";
                    return true;
                    
                }
                // Stops the server!
                bool Stop()
                {
                    //Request the context to close
                    m_asioContext.stop();

                    //Tidy up the context thread
                    if(m_threadContext.joinable())
                        m_threadContext.join();

                    //Inform someone, anybody, if they care...
                    std::cout<<"[SERVER] Stopped!\n";
                    return true;
                }

                //ASYNC - Instruct asio to wait for connection
                void WaitForClientConnection()
                {
                    // Prime context with an instruction to wait until a socket connects. This
				// is the purpose of an "acceptor" object. It will provide a unique socket
				// for each incoming connection attempt
                    m_asioAcceptor.async_accept(
                        [this](std::error_code ec, boost::asio::ip::tcp::socket socket)
                        {
                            // Triggered by incoming connection request
                            if(!ec)
                            {
                                std::cout<<"[SERVER] New Connection: "<<socket.remote_endpoint()<<"\n";

                                std::shared_ptr<connection<T>> newconn=std::make_shared<connection<T>>(connection<T>::owner::server,m_asioContext, std::move(socket),m_qMessagesIn);

                                //Give the user server a chance to deny connection
                                if(OnClientConnect(newconn))
                                {
                                    //Connection allowed, so add to container of new connections
                                    m_deqConnections.push_back(std::move(newconn));

                                    // And very important! Issue a task to the connection's
								// asio context to sit and wait for bytes to arrive!
                                    m_deqConnections.back()->ConnectToClient(this,nIDCounter++);

                                    std::cout<<"["<<m_deqConnections.back()->GetID()<<"] Connection Aproved\n";
                                }
                                else
                                {
                                    std::cout<<"[-----] Connection Denied\n";
                                }
                            }
                            else
                            {
                                //Error has occurred during acceptance
                                std::cout<<"[SERVER] New Connection Error: "<<ec.message()<<"\n";
                            }

                            //Prime the asio context with more work - again simply wait for
                            //another connection...
                            WaitForClientConnection();
                        }
                    );

                }

                //Send a message to a specific client
                void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
                {
                    if(client && client->IsConnected())
                    {
                        client->Send(msg);
                    }
                    else
                    {
                        OnClientDisconnect(client);
                        client.reset();
                        m_deqConnections.erase(std::remove(m_deqConnections.begin(),m_deqConnections.end(),client),m_deqConnections.end());
                    }
                }

                //Send message to all clients
                void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient=nullptr)
                {
                    bool bInvalidClientExists=false;
                    for(auto& client : m_deqConnections)
                    {
                        //Check client is connected...
                        if(client && client->IsConnected())
                        {
                            //..it is!
                            if(client!=pIgnoreClient)
                                client->Send(msg);
                        }
                        else
                        {
                            //The client couldnt be contacted, so assume it has disconnected
                            OnClientDisconnect(client);
                            client.reset();
                            bInvalidClientExists=true;
                        }
                    }
                    if(bInvalidClientExists)
                        m_deqConnections.erase(std::remove(m_deqConnections.begin(),m_deqConnections.end(),nullptr),m_deqConnections.end());
                }

                void Update(size_t nMaxMessages=-1, bool bWait=false)
                {
                    if(bWait) m_qMessagesIn.wait();
                    // Process as many messages as you can up to the value
				    // specified
                    size_t nMessageCount=0;
                    while(nMessageCount<nMaxMessages && !m_qMessagesIn.empty())
                    {
                        //Grab the front message
                        auto msg=m_qMessagesIn.pop_front();

                        //Pass to message handler
                        OnMessage(msg.remote,msg.msg);

                        nMessageCount++;
                    }
                }

            protected:
                //Called when a client connects, you can veto the connection by returnin false
                virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
                {
                    return false;
                }

                //Called when a client appears to have disconnected
                virtual bool OnClientDisconnect(std::shared_ptr<connection<T>> client)
                {
                    return false;
                }

                //Called when a message arrives
                virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg)
                {

                }
            public:
                 //called when a client is validated
                virtual void OnClientValidated(std::shared_ptr<connection<T>> client)
                {

                }
            protected:
                //Thread Safe Queue for incoming message packets
                tsqueue<owned_message<T>> m_qMessagesIn;
            public:
                //Container of active validated connections
                std::deque<std::shared_ptr<connection<T>>> m_deqConnections;
            protected:
                //Order of declaration is important - it is also the order of initialisation
                boost::asio::io_context m_asioContext;
                std::thread m_threadContext;

                //These things need an asio context
                boost::asio::ip::tcp::acceptor m_asioAcceptor;

                //Clients will be identified in the "wider system" via an ID
                uint32_t nIDCounter=10000;
        };
    }
}