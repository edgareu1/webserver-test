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

Server::Server( ServerCfg& cfg, ServerConfig& gen_cfg ) :_cfg( cfg ), _gen_cfg( gen_cfg ) {}

Server::Server( const Server& copy ) :_cfg( copy._cfg ), _gen_cfg( copy._gen_cfg ) {}

Server::~Server() { }

Server& Server::operator=(const Server& copy)
{
    _cfg = copy._cfg;
    return *this;
}

// Helper functions
RouteCfg* find_route(const HttpRequest& req, std::vector<RouteCfg>& routes)
{
    std::pair<std::size_t, RouteCfg*> route_match(0, NULL);

    for(std::vector<RouteCfg>::iterator i = routes.begin(); i != routes.end(); ++i)
    {
        std::string::size_type p = req.target().find(i->route_path);
        if (p == 0 && i->route_path.length() > route_match.first)
            route_match = std::pair<int, RouteCfg*>(i->route_path.length(), &(*i));
    }

    return route_match.second;
}

HttpResponse    list_dir_res( HttpResponse& res, const HttpRequest& req, std::string path) {
        res.body().clear();
        
        std::ifstream   file( DIRLISTING );
        std::string     line_buff;

        if ( !file.is_open() ) {
            // TODO: return 500 error page
            res.set_status(500, "Internal Server Error");
            return res;
        }
        
        std::string                 items;
        std::vector<std::string>    dir_listing = list_dir( path );
        for ( size_t i = 0; i < dir_listing.size(); i++ )
            items.append( "<li><a href=\"" + req.target() + "/" + dir_listing[i] + "\">" + dir_listing[i] + "</a></li>" );

        // Replace all occurs of [DIR] & [ITEMS] with dir name and dir listing
        while( std::getline( file, line_buff) ) {
            replace_occurrence( line_buff, "[DIR]", req.target() );
            replace_occurrence( line_buff, "[ITEMS]", items);
            res.body().append( line_buff );
        }

        res.set_status( 200, "OK" );
        res.set_header( "Content-Type", "text/html" );
        file.close();
        return res;
}



// Will take a request and handle it, which includes calling cgi
HttpResponse Server::handleRequest(const HttpRequest& req)
{
    HttpResponse res;
    RouteCfg* route = find_route(req, _cfg.routes);
    std::string path;

    // Check if a valid route has been found
    if (!route) {
        // TODO: return 404 error page
        res.set_status(404, "File Not Found");
        return res;
    }

    path = get_path( req, route);

    if (::access(path.c_str(), F_OK) < 0)
    {
        // TODO: return 404 error page
        res.set_status(404, "File Not Found");
        return res;
    }
    if (::access(path.c_str(), O_RDONLY) < 0)
    {
        // TODO: return 403 error page
        res.set_status(403, "Forbidden");
        return res;
    }

    // Check if requested method is available
    if ( !(route->accepted_methods.empty()) ) {
        if ( !(is_accepted_method( route, req.method() ) ) )  {
            // TODO: return 405 error page
            res.set_status( 405, "Method Not Allowed" );
            return res;
        }
    }

    // TODO: check for CGI

    if (req.method() == "GET")
    {
    	switch ( index_path( req, route, path) ) {
	    case 1:
		return list_dir_res( res, req, path);
	    case 2:
		// TODO: return 404 error page
		res.set_status( 405, "Not Found" );
		return res;
	}

        std::ifstream file(path.c_str());
        std::string buff(BUFFER_SIZE, '\0');
        if (!file.is_open())
        {
            // TODO: return 500 error page
            res.set_status(500, "Internal Server Error");
            return res;
        }

        while(file.read(&buff[0], BUFFER_SIZE).gcount() > 0)
            res.body().append(buff, 0, file.gcount());

        std::ostringstream ss;
        ss << res.body().length();
        res.set_status(200, "OK");

        res.set_header("Content-Type", ServerConfig::getMimeType(path));
    }
    return res;
}

ServerCfg& Server::cfg() { return _cfg; }