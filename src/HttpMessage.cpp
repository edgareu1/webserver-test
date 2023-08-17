#include "HttpMessage.hpp"
#include <sstream>
#include <exception>

HttpMessage::headers_t::iterator HttpMessage::add_header(const std::string& name, const std::string& value)
{
    if (_headers[name].empty())
        _headers[name] = value;
    else
        _headers[name] += ", " + value;
    return _headers.find(name);
}

// Constructors
HttpMessage::HttpMessage() : _major_version(1), _minor_version(1) { }
HttpMessage::HttpMessage(const HttpMessage& copy) { *this = copy; }

HttpMessage::~HttpMessage() { }

HttpMessage& HttpMessage::operator=(const HttpMessage& copy)
{
    _major_version = copy._major_version;
    _minor_version = copy._minor_version;

    _headers = copy._headers;
    _body = copy._body;
    return *this;
}

// Getters/Setters
int HttpMessage::major_version() const { return _major_version; }
int HttpMessage::minor_version() const { return _minor_version; }
std::string HttpMessage::version_string() const
{
    std::stringstream result;
    result << "HTTP/" << _major_version << "." << _minor_version;
    return result.str();
}

HttpMessage::headers_t& HttpMessage::headers() { return _headers; }

std::string& HttpMessage::headers(std::string& field_name)
{
    return _headers.at(field_name);
}

const std::string& HttpMessage::headers(const std::string& field_name) const
{
    return _headers.at(field_name);
}

HttpMessage::headers_t::const_iterator HttpMessage::headers(const std::string& field, const std::string& value)
{
    if (_headers[field].empty())
        _headers[field] = value;
    else
        _headers[field] += ", " + value;
    return _headers.find(field);
}

void HttpMessage::removeHeader(const std::string& field) { _headers.erase(field); }

const std::string& HttpMessage::body() const { return _body; }
std::string& HttpMessage::body() { return _body; }

// This will convert it back to a text based http message
std::string HttpMessage::toStringHeaders() const
{
    std::stringstream result;
    for (HttpMessage::headers_t::const_iterator i = _headers.begin(); i != _headers.end(); ++i)
        result << i->first << ": " << i->second << "\r\n";
    return result.str();
}
std::string HttpMessage::toStringBody() const { return _body;}

std::string HttpMessage::toString()
{
    std::stringstream result;

    result << toStringStart() << toStringHeaders() << "\r\n" << toStringBody();

    return result.str();
}
