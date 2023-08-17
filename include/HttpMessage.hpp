#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <map>
#include <string>

#define ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DIGIT "0123456789"
#define TOKEN ("!#$%&'*+-.^_`|~" ALPHA DIGIT)


class HttpMessage {
public:
    typedef std::map<std::string, std::string> headers_t;
protected:
    headers_t _headers;
    int _major_version;
    int _minor_version;

    std::string _body;

    headers_t::iterator add_header(const std::string& name, const std::string& value);

    //helper functions for toString method
    virtual std::string toStringStart() const = 0;
    virtual std::string toStringHeaders() const;
    virtual std::string toStringBody() const;
public:
    // Constructors
    HttpMessage();
    HttpMessage(const HttpMessage& copy);

    virtual ~HttpMessage();

    HttpMessage& operator=(const HttpMessage& copy);

    // Getters/Setters
    int major_version() const;
    int minor_version() const;
    void version(int maj, int min);
    std::string version_string() const; // Returns version string (for example: "HTTP/1.1")

//    const std::string& header(const std::string& field_name) const;
    headers_t& headers();
    std::string& headers(std::string& field);
    const std::string& headers(const std::string& field) const;
    HttpMessage::headers_t::const_iterator headers(const std::string& field, const std::string& value);
    void removeHeader(const std::string& field);
    const std::string& body() const;
    std::string& body();

    // This will convert it back to a text based http message
    virtual std::string toString();
};

#endif
