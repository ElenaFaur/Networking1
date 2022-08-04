clang++ -std=c++17 -o Net_client SimpleClient.cpp async_keys/linux-kbhit.cpp -L /usr/lib/ -lboost_system -lboost_thread -lpthread
clang++ -std=c++17 -o Net_server SimpleServer.cpp -L /usr/lib/ -lboost_system -lboost_thread -lpthread
