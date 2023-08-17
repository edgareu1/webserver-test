#include "Server.hpp"
#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerUtils.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>


#define VALIDPATH 0
#define AUTOINDEX 1
#define INVALIDPATH 2


// BEGIN: Helper Functions Prototypes
RouteCfg*       find_route(const HttpRequest& req, std::vector<RouteCfg>& routes);
HttpResponse    list_dir_res(const HttpRequest& req, std::string path, HttpResponse& res, ServerCfg& _cfg, RouteCfg* route);
HttpResponse    response_get(const HttpRequest& req, std::string path, HttpResponse& res, ServerCfg& _cfg, RouteCfg* route);
HttpResponse    response_head(const HttpRequest& req, std::string path, HttpResponse& res, ServerCfg& _cfg, RouteCfg* route);
HttpResponse    response_error(const HttpRequest& req, HttpResponse& res, ServerCfg& _cfg, RouteCfg* route, const int statusCode);
// END: Helper Functions Prototypes


// BEGIN: Canonical Form Functions
Server::Server(const ServerCfg& cfg) :_cfg(cfg) {}

Server::Server(const Server& copy) :_cfg(copy._cfg) {}

Server::~Server() {}

Server& Server::operator=(const Server& copy)
{
    _cfg = copy._cfg;
    return *this;
}
// END: Canonical Form Functions


// BEGIN: Class Functions
// Will take a request and handle it, which includes calling CGI
HttpResponse    Server::handleRequest(const HttpRequest& req)
{
    HttpResponse    res;
    RouteCfg*       route = find_route(req, _cfg.routes);
    std::string     path;

    // Check if a valid route has been found
    if (!route) {
        // TODO: return 404 error page
        res.set_status(404);
        return res;
    }

    path = get_path(req, route);

    // Check if file exists
    if (!is_directory(path) && ::access(path.c_str(), F_OK) < 0)
        return response_error(req, res, _cfg, route, 404);

    // Check if we have access to file
    else if (::access(path.c_str(), O_RDONLY) < 0)
        return response_error(req, res, _cfg, route, 403);

    // Check if requested method is available
    else if (!route->accepted_methods.empty() && !is_accepted_method(route, req.method()))
        return response_error(req, res, _cfg, route, 405);

    // Handle GET method
    else if (req.method() == "GET")
        return response_get(req, path, res, _cfg, route);

    // Handle HEAD method
    else if (req.method() == "HEAD")
        return response_head(req, path, res, _cfg, route);

    // TODO: check for CGI

    return res;
}

ServerCfg&  Server::cfg()
{
    return _cfg;
}
// END: Class Functions


// BEGIN: Helper Functions
RouteCfg*   find_route(const HttpRequest& req, std::vector<RouteCfg>& routes)
{
    std::pair<std::size_t, RouteCfg*>   route_match(0, NULL);

    for(std::vector<RouteCfg>::iterator i = routes.begin(); i != routes.end(); ++i)
    {
        std::string::size_type  p = req.target().find(i->route_path);

        if (p == 0 && i->route_path.length() > route_match.first)
            route_match = std::pair<int, RouteCfg*>(i->route_path.length(), &(*i));
    }

    return route_match.second;
}

HttpResponse    list_dir_res(const HttpRequest& req, std::string path, HttpResponse& res, ServerCfg& _cfg, RouteCfg* route)
{
    res.body().clear();

    std::ifstream   file(DIRLISTING);
    std::string     line_buff;

    if (!file.is_open())
        return response_error(req, res, _cfg, route, 500);

    std::string                 items;
    std::vector<std::string>    dir_listing = list_dir(path);

    for (size_t i = 0; i < dir_listing.size(); i++)
        items.append("<li><a href=\"" + remove_slash_dups(req.target() + "/") + dir_listing[i] + "\">" + dir_listing[i] + "</a></li>");

    // Replace all occurs of [DIR] & [ITEMS] with dir name and dir listing
    while(std::getline(file, line_buff))
    {
        replace_occurrence(line_buff, "[DIR]", req.target());
        replace_occurrence(line_buff, "[ITEMS]", items);
        res.body().append(line_buff);
    }

    res.set_status(200);
    res.set_header("Content-Type", "text/html");
    file.close();

    return res;
}

HttpResponse    response_get(const HttpRequest& req, std::string path, HttpResponse& res, ServerCfg& _cfg, RouteCfg* route)
{
    switch (index_path(req, route, path)) {
        case AUTOINDEX:
            return list_dir_res(req, path, res, _cfg, route);

        case INVALIDPATH:
            return response_error(req, res, _cfg, route, 404);
    }

    std::ifstream   file(path.c_str());
    std::string     buff(BUFFER_SIZE, '\0');

    if (!file.is_open())
        return response_error(req, res, _cfg, route, 500);

    while(file.read(&buff[0], BUFFER_SIZE).gcount() > 0)
        res.body().append(buff, 0, file.gcount());

    std::ostringstream  ss;

    ss << res.body().length();
    res.set_status(200);
    res.set_header("Content-Type", ServerConfig::getMimeType(path));

    return res;
}

HttpResponse    response_head(const HttpRequest& req, std::string path, HttpResponse& res, ServerCfg& _cfg, RouteCfg* route)
{
    switch (index_path(req, route, path)) {
        case VALIDPATH:
            res.set_status(200);
            res.set_header("Content-type", ServerConfig::getMimeType(path));
            return res;

        case AUTOINDEX:
            res.set_status(200);
            res.set_header("Content-type", ServerConfig::getMimeType(DIRLISTING));
            return res;

        case INVALIDPATH:
            return response_error(req, res, _cfg, route, 404);
    }

    return res;
}

HttpResponse    response_error(const HttpRequest& req, HttpResponse& res, ServerCfg& _cfg, RouteCfg* route, const int statusCode)
{
    std::map<short, std::string>::const_iterator    it = _cfg.error_pages.find(statusCode);
    std::string     path = get_path(it->second, route);
    std::ifstream   file(path.c_str());
    std::string     buff(BUFFER_SIZE, '\0');

    if (it != _cfg.error_pages.end())
    {
        if (is_file(path) && file.is_open())
        {
            while(file.read(&buff[0], BUFFER_SIZE).gcount() > 0)
                res.body().append(buff, 0, file.gcount());

            std::ostringstream  ss;
            ss << res.body().length();

            res.set_status(200);
            res.set_header("Content-Type", ServerConfig::getMimeType(path));
        }
        else if (statusCode == 500)
            res.set_status(500);
        else
            return response_error(req, res, _cfg, route, 500);
    }
    else
        res.set_status(statusCode);

    return res;
}
// END: Helper Functions
