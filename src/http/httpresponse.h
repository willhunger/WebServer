#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "../buffer/buffer.h"

class HttpResponse
{
using string = std::string;
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);

    void response(Buffer& buffer);

    void unmapFile();

    char* file();

    size_t fileLen() const;

    void errorContent(Buffer& buffer, std::string message);

    int code() const
    {
        return _code;
    }
private:
    void addStateLine(Buffer& buffer);
    void addHeader(Buffer& buffer);
    void addContent(Buffer& buffer);

    void errorHtml();
    std::string getFileType();

    int _code;
    bool _isKeepAlive;

    string _path;
    string _srcDir;
    
    char* _mmFile;
    struct stat _mmFileStat;

    static const std::unordered_map<std::string, std::string> CONTENT_TYPE;
    static const std::unordered_map<int, std::string> HTTP_STATUS;
    static const std::unordered_map<int, std::string> FILE_PATH;  
};


#endif // !HTTP_RESPONSE_H