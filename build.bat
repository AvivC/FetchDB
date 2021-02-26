g++ -fPIC -shared -o _httpserver.dll -Wl,--subsystem,windows -I. -Iwebserver\socket\src -I./../RibbonLang/src -Iwebserver\base64 *.cpp webserver\*.cpp webserver\socket\src\Socket.cpp webserver\base64\base64.cpp -g -Wall -Wno-unused -Wno-write-strings -lwsock32 -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic


copy _httpserver.dll C:\Ribbon\stdlib\_httpserver.dll
