#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include <vector>

//prime asio with an instruction that if some data arrives read it and display it to the console 
std::vector<char> vBuffer(1 * 1024);

//function which will link to asio to handle the data reading for us
void GrabSomeData(boost::asio::ip::tcp::socket& socket){

    //use it in asynchronous mode
    //because it's asynchronous this isn't going to do anything right away
    // -- instead, it will prime the context with some work to do when data is avalaible on that socket to read
    //task to do: read the data from the socket, put it in the buffer, and display it on the screen
    //this function needs a lambda function as well
    socket.async_read_some(boost::asio::buffer(vBuffer.data(), vBuffer.size()),
                            [&](std::error_code ec, std::size_t length){    //length - the amount of code that was read 
                                                                            // -- by the read_some function
                                if(!ec){

                                    std::cout << "\n\nRead " << length << " bytes\n\n";

                                    for(int i = 0; i < length; i++){
                                        std::cout << vBuffer[i];
                                    }   //displaying bytes to the console

                                    GrabSomeData(socket);
                                }
                                //when some data arrives on that socket, asio is going to implement the code within the lambda
                                //if in the first instance it only reads 100 bytes, but the server has sent 10k bytes,
                                // -- GrabSomeData will call the async_read_some function again and will grab a bit more and so on...
                                // -- until there is no data left to read -- and in that point the async function will wait 
                                // -- until there is some (new) data to read
                            });
    
}


int main(){
     
    std::cout << "h\n";
    
    boost::system::error_code ec;
    //create a context - essentially the platform specific interface
    boost::asio::io_context context;
    //we can consider this as an unique instance of asio

    //give some fake tasks to asio so the context doesn't finish
    //create some fake work for the context to do (the program may end up executing after the context finished executing)
    boost::asio::io_context::work idleWork(context);
    
    //start the context:
    //when we want to wait for something, it invariably blocks and we don't want i/o context to be blocked all the time
    //solution: run the context in its own thread
    //this gives the context some space in which can execute these instructions (priming the context), without blocking our main program
    std::thread thrContext = std::thread([&]() { context.run(); });


    //ips: 93.184.216.34, 51.38.81.49
    //get the address of somewhere we wish to connect to
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("51.38.81.49", ec), 80);
    //we can consider this endpoint as an address
    //an endpoint is defined by an ip address and a port
    //we pass the ip address as a string in a way asio can understand (make_address) (80 is the port)
    //if the string address is malfunctioned, the ec (error_code variable) will hold an error

    // boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1", ec), 80); //communicate with yourself (ec)

    //create a (networking) socket, the context will deliver implementation
    boost::asio::ip::tcp::socket socket(context);
    //it is the hook in the operating system's network drivers 
    //will act as the doorway to the network that we're connected to
    //when we create the socket, we associate it with the asio context

    //tell socket to try and connect to the address that we specified
    socket.connect(endpoint, ec);

    //checking the error code:
    if(!ec){
        std::cout << "Connected!" << std::endl;
    }else{
        std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
    }
    
    //if we run this program, it will attempt to connect to this ip address (the ip of "example.com")
    //basic websites like this communicate on port 80 (http)

    //if the connection is succesful, then if we call the socket.open(), that should return true
    // -- which means that we have a live and active connection 
    if(socket.is_open()){

        GrabSomeData(socket);
        //prime the asio context with the ability to read bytes before I send some data 

        std::string sRequest = 
            "GET /index.html HTTP/1.1\r\n"  //way of defining string on multiple lines of code
            "HOST: example.com\r\n"
            "Connection: close\r\n\r\n";    
        //since we are trying to connect to a website
        // -- the server at the other end is expecting an htpp request 

        //send the request after we construct it (the request is a string literal, like in real life also)
        socket.write_some(boost::asio::buffer(sRequest.data(), sRequest.size()), ec);
        //string::data returns a pointer to an array (that includes the same sequence of chars + an additional '\0' at the end)
        //it's synonym with string::c_str
        //write some means please try and send as much of this data as possible
        //when reading and writing data in asio, we work with asio::buffer (a container that contains an array of bytes)

        //after writing our request to the server, no longer interested in reading data manually --> call GrabSomeData(socket);
        //GrabSomeData(socket); -> moving this on line 84 (nothing happes because the program exited too quickly)

        //preventing the program stopping prematurely we can use a big delay:
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2000ms);

        context.stop();

        if(thrContext.joinable()){
            thrContext.join();
        }

        //demo of reading manually: (before adding GrabSomeData)

        // //we must introduce a delay (it takes time from the server to answer to our request)
        // using namespace std::chrono_literals;
        // std::this_thread::sleep_for(200ms); //this is a bad ideea to introduce in the code - we can make use of socket::wait

        // socket.wait(socket.wait_read);
        // //this will block the program my running program until there is some data avalaible to read (replace for delay)

        // size_t bytes = socket.available();
        // std::cout << "Bytes avalaible: " << bytes << std::endl;
        // //all data may not be avalaible - so we need a solution

        // if(bytes > 0){  //if there are some bytes, we are going to read them from the socket

        //     std::vector<char> vBuffer(bytes); 
        //     socket.read_some(boost::asio::buffer(vBuffer.data(), vBuffer.size()), ec); //asio in synchronous mode
        //     //vector::data returns a direct pointer to the memory array used internally by the vector

        //     for(auto c : vBuffer){
        //         std::cout << c;
        //     }

        // }

    }
    
    
}