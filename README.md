
# Description

As you can see, this repository contains 4 important folders: devcontainer,NetCommon,demo,unitest

## devcontainer

This folder contains the linux container, which was created to simplify things for the client-server project

## NetCommon

This folder contains the Client-server project, accomplished with boost::asio. It's  a networking framework project using interfaces for both client and the server, using boost::asio cross-platform C++ Library.

#### Lauching the aplication

The order to launch the executables is "Net_server", then "Net_client".
After launching the server, the terminal will display the "[SERVER] Started." message. Once the server has started, you can launch multiple clients that are notified through a message, where says that the server accepted the connection.


## demo

This folder includes a simple example c++ in networking

## unitest

This folder includes google tests for the client-server project
