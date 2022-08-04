#pragma once
#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace olc
{
    namespace net
    {
        //forward declare (not #included)
        template<typename T>
        class server_interface;

        template<typename T>
        class connection : public std::enable_shared_from_this<connection<T>>
        {
            public:
            // A connection is "owned" by either a server or a client, and its
			// behaviour is slightly different bewteen the two.
                enum class owner
                {
                    server,
                    client
                };
                // Constructor: Specify Owner, connect to context, transfer the socket
			    //Provide reference to incoming message queue
                connection(owner parent, boost::asio::io_context& asioContext,boost::asio::ip::tcp::socket socket,tsqueue<owned_message<T>>& qIn)
                :m_asioContext(asioContext),m_socket(std::move(socket)),m_qMessagesIn(qIn)
                {
                    m_nOwnerType=parent;

                    //construct validation check data
                    if(m_nOwnerType == owner::server){  //if the owner is a server

                        //I'm expected to generate some random data and then send out to the connection
                        //Connection is Server -> Client, construct random data for the client 
                        // -- to transform and send back to validation
                        m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count()); //data to send out

                        //the scrambled version of the data
                        //pre-calculate the result for checking when the client responds
                        m_nHandshakeCheck = scramble(m_nHandshakeOut);    
                    }
                    else{

                        //Connection is Client -> Server, so we have nothing do define, just waiting for the data to be sent
                        // -- from the server
                        m_nHandshakeIn = 0;
                        m_nHandshakeOut = 0;
                    }
                }

                virtual ~connection(){}
                // This ID is used system wide - its how clients will understand other clients
			// exist across the whole system.
                uint32_t GetID() const
                {
                    return id;
                }
            public:
                void ConnectToClient(olc::net::server_interface<T>* server, uint32_t uid = 0)
                {
                    if(m_nOwnerType==owner::server)
                    {
                        if(m_socket.is_open())
                        {
                            id = uid;  //store the id 
                        //was: ReadHeader();

                        //a client has attempted to connect to server, but we wish the client to first
                        // -- validate itself, so first write out the handshake data to be validated
                        WriteValidation();

                        //next, issue a task to sit and wait asynchronously for precisely 
                        // -- the validation data sent back from the client
                        ReadValidation(server); 
                        }
                    }
                }
                void ConnectToServer(const boost::asio::ip::tcp::resolver::results_type &endpoints)
                {
                    //Only clients can connect to servers
                    if(m_nOwnerType == owner::client)
                    {
                        //Request asio context attempts to connect to an endpoint
                        boost::asio::async_connect(m_socket,endpoints,
                        [this](std::error_code ec, boost::asio::ip::tcp::endpoint endpoint)
                        {
                            if(!ec)
                            {
                               //was:
                                    // //issue the task to read the header
                                    // ReadHeader();

                                    //first thing server will do is send packet to be validated
                                    //so wait for that and respond
                                    ReadValidation();
                            }
                        }
                        );
                    }
                }
                void Disconnect()
                {
                    if(IsConnected())
                        boost::asio::post(m_asioContext,[this](){m_socket.close();});
                }
                bool IsConnected() const
                {
                    return m_socket.is_open();
                }
            public:
            // ASYNC - Send a message, connections are one-to-one so no need to specifiy
			// the target, for a client, the target is the server and vice versa
                void Send(const message<T>& msg)
                {
                    boost::asio::post(m_asioContext,
                    [this, msg]()
                    {
                        // If the queue has a message in it, then we must 
						// assume that it is in the process of asynchronously being written.
						// Either way add the message to the queue to be output. If no messages
						// were available to be written, then start the process of writing the
						// message at the front of the queue.
                        bool bWritingMessage=!m_qMessagesOut.empty();
                        m_qMessagesOut.push_back(msg);
                        if(!bWritingMessage)
                        {
                            WriteHeader();
                        }
                    }
                    );
                }
            private:
                //ASYNC - Prime context ready to read a message header
                void ReadHeader()
                {
                    // If this function is called, we are expecting asio to wait until it receives
				// enough bytes to form a header of a message. We know the headers are a fixed
				// size, so allocate a transmission buffer large enough to store it. In fact, 
				// we will construct the message in a "temporary" message object as it's 
				// convenient to work with.
                    boost::asio::async_read(m_socket,boost::asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if(!ec)
                        {
                            // A complete message header has been read, check if this message
							// has a body to follow...
                            if(m_msgTemporaryIn.header.size>0)
                            {
                                // ...it does, so allocate enough space in the messages' body
								// vector, and issue asio with the task to read the body.
                                m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
                                ReadBody();
                            }
                            else
                            {
                                // it doesn't, so add this bodyless message to the connections
								// incoming message queue
                                AddToIncomingMessageQueue();
                            }
                        }
                        else
                        {
                            // Reading form the client went wrong, most likely a disconnect
							// has occurred. Close the socket and let the system tidy it up later.
                            std::cout<<"["<<id<<"] Read Header Fail.\n";
                            m_socket.close();
                        }
                    }
                    );
                }

                //ASYNC - Prime context ready to read a message body
                void ReadBody()
                {
                    // If this function is called, a header has already been read, and that header
				// request we read a body, The space for that body has already been allocated
				// in the temporary message object, so just wait for the bytes to arrive...
                    boost::asio::async_read(m_socket, boost::asio::buffer(m_msgTemporaryIn.body.data(),m_msgTemporaryIn.body.size()),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if(!ec)
                        {
                            // ...and they have! The message is now complete, so add
							// the whole message to incoming queue
                            AddToIncomingMessageQueue();
                        }
                        else
                        {
                            //As above!
                            std::cout<<"["<<id<<"] Read Body Fail.\n";
                            m_socket.close();
                        }
                    }
                    );
                }

                //ASYNC - Prime context to write a message header
                void WriteHeader()
                {
                    // If this function is called, we know the outgoing message queue must have 
				// at least one message to send. So allocate a transmission buffer to hold
				// the message, and issue the work - asio, send these bytes
                    boost::asio::async_write(m_socket, boost::asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
                    [this](std::error_code ec, std::size_t length)
                    {
                        // asio has now sent the bytes - if there was a problem
						// an error would be available...
                        if(!ec)
                        {
                            // ... no error, so check if the message header just sent also
							// has a message body...
                            if(m_qMessagesOut.front().body.size()>0)
                            {
                                // ...it does, so issue the task to write the body bytes
                                WriteBody();
                            }
                            else
                            {
                                // ...it didnt, so we are done with this message. Remove it from 
								// the outgoing message queue
                                m_qMessagesOut.pop_front();

                                // If the queue is not empty, there are more messages to send, so
								// make this happen by issuing the task to send the next header.

                                if(!m_qMessagesOut.empty())
                                {
                                    WriteHeader();
                                }
                            }
                        }
                        else
                        {
                            // ...asio failed to write the message, we could analyse why but 
							// for now simply assume the connection has died by closing the
							// socket. When a future attempt to write to this client fails due
							// to the closed socket, it will be tidied up.
                            std::cout<<"["<<id<<"] Write Header Fail.\n";
                            m_socket.close();
                        }
                    }
                    );
                }

                //ASYNC - Prime context to write a message body
                void WriteBody()
                {
                    // If this function is called, a header has just been sent, and that header
				// indicated a body existed for this message. Fill a transmission buffer
				// with the body data, and send it!
                    boost::asio::async_write(m_socket, boost::asio::buffer(m_qMessagesOut.front().body.data(),m_qMessagesOut.front().body.size()),
                    [this](std::error_code ec,std::size_t length)
                    {
                        if(!ec)
                        {
                            // Sending was successful, so we are done with the message
							// and remove it from the queue
                            m_qMessagesOut.pop_front();
                            // If the queue still has messages in it, then issue the task to 
							// send the next messages' header.
                            if(!m_qMessagesOut.empty())
                            {
                                WriteHeader();
                            }
                        }
                        else
                        {
                            // Sending failed, see WriteHeader() equivalent for description
                            std::cout<<"["<<id<<"] Write Body Fail.\n";
                            m_socket.close();
                        }
                    }
                    );
                }
                // Once a full message is received, add it to the incoming queue
                void AddToIncomingMessageQueue()
                {
                    // Shove it in queue, converting it to an "owned message", by initialising
				// with the a shared pointer from this connection object
                    if(m_nOwnerType == owner::server)
                        m_qMessagesIn.push_back({this->shared_from_this(), m_msgTemporaryIn});
                    else
                        m_qMessagesIn.push_back({nullptr,m_msgTemporaryIn});
                        // We must now prime the asio context to receive the next message. It 
				// wil just sit and wait for bytes to arrive, and the message construction
				// process repeats itself.
                    ReadHeader();
                }

                //"encrypt" data
            uint64_t scramble(uint64_t nInput){

                uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
                out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
                return out ^ 0xC0DEFACE12345678;
            }

            //ASYNC - used by both client and server to write validation packet
            void WriteValidation(){

                boost::asio::async_write(m_socket, boost::asio::buffer(&m_nHandshakeOut, sizeof(uint64_t)), 
                            [this](std::error_code ec, std::size_t length){

                                if(!ec){

                                    //validation data sent, clients should sit and wait for a response (or a closure)
                                    if(m_nOwnerType == owner::client){
                                        
                                        ReadHeader();
                                    }
                                }else{
                                    m_socket.close();
                                }
                            });
            }

            void ReadValidation(olc::net::server_interface<T>* server = nullptr){

                boost::asio::async_read(m_socket, boost::asio::buffer(&m_nHandshakeIn, sizeof(uint64_t)),
                            [this, server](std::error_code ec, std::size_t length){

                                if(!ec){
                                    
                                    //if we're a server, we're expecting ReadValidation() to read the data that
                                    // -- has been computed by the client 
                                    //if we're the client, we're expecting the validation data to be the random data provided
                                    // -- by the server

                                    if(m_nOwnerType == owner::server){
                                        //client has provided valid solution, so allow it to connect properly
                                        if(m_nHandshakeIn == m_nHandshakeCheck){
                                            
                                            std::cout << "Client validated" << std::endl;
                                            server -> OnClientValidated(this -> shared_from_this()); 

                                            //sit waiting to receive data now
                                            ReadHeader();
                                        }else{
                                            //client gave incorrect data, so disconnect
                                            std::cout << "Client Disconnected (Fail Validation)" << std::endl;
                                            m_socket.close();
                                        }
                                    }
                                    else{
                                        //connection to client, so solve puzzle
                                        m_nHandshakeOut = scramble(m_nHandshakeIn);

                                        //write the result
                                        WriteValidation();
                                    }


                                }else{
                                    //some biggerfailure occured
                                    std::cout << "Client Disconnected (ReadValidation)" << std::endl;
                                    m_socket.close();
                                }
                            });
            }


            protected:
                //Each connection has a unique socket to a remote
                boost::asio::ip::tcp::socket m_socket;

                //This context is shared with the whole asio instance
                boost::asio::io_context& m_asioContext;

                //This queue holds all messages to be sent to the remote side of this
                //connection
                tsqueue<message<T>> m_qMessagesOut;  

                //This queue holds all messages that have been recieved from
                //the remote side of this connection. Note it is a reference
                //as the "owner" of this connection is expected to provide a queue
                tsqueue<owned_message<T>>& m_qMessagesIn; 
                // Incoming messages are constructed asynchronously, so we will
			// store the part assembled message here, until it is ready
                message<T> m_msgTemporaryIn;

                //The "owner" decides how some of the connection behaves
                owner m_nOwnerType= owner::server;    
                uint32_t id=0;  

                //handshake validation
            uint64_t m_nHandshakeOut = 0; //what the connection will send outwards
            uint64_t m_nHandshakeIn = 0; //what the connection has received as a result or data to scramble in the first place
            uint64_t m_nHandshakeCheck = 0; //used by the server to perform the comparison to see if the client is valid or not 

            //effectively, the connection object is the glue       
        };
    }
}