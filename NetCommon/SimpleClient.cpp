#include "custom_client.h"
#include "/workspaces/Repository/ubuntu_networking/NetCommon/async_keys/linux-kbhit.h"

static void sighandler(int signo)
{
    printf("\nSIG\n");
}

int main()
{
    CustomClient c;
    c.Connect("127.0.0.1",60000);

    //bool key[3]={false,false,false};
    //bool old_key[3]={false,false,false};

    bool bQuit=false;
    term_setup(sighandler);
    while(!bQuit)
    {
        /*if(GetForegroundWindow()==GetConsoleWindow())
        {
            key[0]=GetAsyncKeyState('1') & 0x8000;
            key[1]=GetAsyncKeyState('2') & 0x8000;
            key[2]=GetAsyncKeyState('3') & 0x8000;
        }

        if(key[0] && !old_key[0]) c.PingServer();
        if(key[1] && !old_key[1]) c.MessageAll();
        if(key[2] && !old_key[2]) bQuit=true;

        for(int i=0;i<3;i++)
            old_key[i]=key[i];*/

            if (kbhit()) 
            {
            if (keydown(UP)) c.PingServer();
			if (keydown(DOWN)) c.MessageAll();
            if (keydown(ESC)) bQuit = true;
            }
        if(c.IsConnected())
        {
            if(!c.Incoming().empty())
            {
                auto msg=c.Incoming().pop_front().msg;

                switch (msg.header.id)
                {
                    case CustomMsgTypes::ServerAccept:
                    {
                        //Server has responded to a ping request
                        std::cout<<"Server Accepted Connection\n";
                    }
                    break;
                    case CustomMsgTypes::ServerPing:
                    {
                        std::chrono::system_clock::time_point timeNow=std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;
                        msg>>timeThen;
                        std::cout<<"Ping: "<<std::chrono::duration<double>(timeNow-timeThen).count()<<"\n";
                    }
                    break;

                    case CustomMsgTypes::ServerMessage:
                    {
                        //Server has responded to a ping request
                        uint32_t clientID;
                        msg>>clientID;
                        std::cout<<"Hello from ["<<clientID<<"]\n";
                    }
                    break;
                }
            }
        }
        else
        {
            std::cout<<"Server Down\n";
            bQuit=true;
        }
    }
    term_restore();

    return 0;
}