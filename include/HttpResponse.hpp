#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "HttpMessage.hpp"
#include <string>

class HttpResponse : public HttpMessage
{
private:
    int         _status_code;
    std::string _status_message;

    static const std::map<short, std::string>   _status_code_map;

protected:
    virtual std::string toStringStart() const;

public:
    HttpResponse();
    HttpResponse(const HttpResponse& copy);

    ~HttpResponse();

    HttpResponse&   operator= (const HttpResponse& copy);

    HttpResponse&   set_version(int major, int minor);

    HttpResponse&       set_header(const std::string&, const std::string& value);
    std::string&        get_header(std::string& name);
    HttpResponse&       remove_header(const std::string& name);

    HttpResponse&   set_status(int code);
    HttpResponse&   set_status(int code, const std::string& message);

    std::string&        body();
    const std::string&  body() const;

    int                 get_status() const;
    const std::string&  get_status_message() const;

    virtual std::string toString();
};
#endif
