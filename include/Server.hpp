#ifndef SERVER_HPP
#define SERVER_HPP

#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#define BUFFER_SIZE 4096

class Server {
    private:
        ServerCfg   _cfg;

        Server();

    public:
        // Will take a server config to set up this server, and a general config for stuff like CGI and mime types
        Server(const ServerCfg& cfg);
        Server(const Server& copy);

        ~Server();

        Server& operator=(const Server& copy);

        // Will take a request and handle it, which includes calling CGI
        HttpResponse    handleRequest(const HttpRequest& req);

        ServerCfg&  cfg();
};

#endif
