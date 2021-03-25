#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::CONTENT_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::HTTP_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpResponse::FILE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() : 
    _code(-1),
    _isKeepAlive(false),
    _mmFile(nullptr),
    _mmFileStat()
{
}

HttpResponse::~HttpResponse()
{
    unmapFile();
}

void HttpResponse::init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code)
{
    assert(srcDir != "");
    if (_mmFile)
    {
        unmapFile();
    }
    _code = code;
    _isKeepAlive = isKeepAlive;
    _srcDir = srcDir;
    _mmFile = nullptr;
    _mmFileStat = {0};
}

void HttpResponse::response(Buffer& buffer)
{
    if (stat((_srcDir + _path).data(), &_mmFileStat) < 0 || S_ISDIR(_mmFileStat.st_mode))
    {
        _code = 404;
    }
    else if (!(_mmFileStat.st_mode & S_IROTH))
    {
        _code = 403;
    }
    else if (_code == -1)
    {
        _code = 202;
    }
    errorHtml();
    addStateLine(buffer);
    addHeader(buffer);
    addContent(buffer);
}

char* HttpResponse::file()
{
    return _mmFile;
}

size_t HttpResponse::fileLen() const
{
    return _mmFileStat.st_size;
}

void HttpResponse::errorHtml()
{
    if (HTTP_STATUS.count(_code) == 1)
    {
        _path = HTTP_STATUS.find(_code)->second;
        stat((_srcDir + _path).data(), &_mmFileStat);
    }
}

void HttpResponse::addStateLine(Buffer& buffer)
{
    string status;
    if (HTTP_STATUS.count(_code) == 1)
    {
        status = HTTP_STATUS.find(_code)->second;
    }
    else
    {
        _code = 400;
        status = HTTP_STATUS.find(400)->second;
    }
    buffer.append("HTTP/1.1 " + std::to_string(_code) + " " + status + "\r\n");
}

void HttpResponse::addHeader(Buffer& buffer)
{
    buffer.append("Connection:");
    if (_isKeepAlive)
    {
        buffer.append("keep_alive\r\n");
        buffer.append("keep-alive: max = 6, timeout=120\r\n");
    }
    else
    {
        buffer.append("close\r\n");
    }
    buffer.append("Content-type: " + getFileType() + "\r\n");
}

void HttpResponse::addContent(Buffer& buffer)
{
    int srcFd = open((_srcDir + _path).data(), O_RDONLY);
    if (srcFd < 0)
    {
        errorContent(buffer, "File Not Found");
        return;
    }

    int* mmRet = (int*) mmap(0, _mmFileStat.st_size, PORT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1)
    {
        errorContent(buffer, "File Not Found");
        return;
    }
    _mmFile = (char*)mmRet;
    close(srcFd);
    buffer.append("Content-length: " + std::to_string(_mmFileStat.st_size) + "\r\n\r\n");
}

void HttpResponse::unmapFile()
{
    if (_mmFile)
    {
        munmap(_mmFile, _mmFileStat.st_size);
        _mmFile = nullptr;
    }
}

std::string HttpResponse::getFileType()
{
    std::string::size_type idx = _path.find_last_of('.');
    if (idx == std::string::npos)
    {
        return "text/plain";
    }
    std::string type = _path.substr(idx);
    if (CONTENT_TYPE.count(type) == 1)
    {
        return CONTENT_TYPE.find(type)->second;
    }
    return "text/plain";
}

void HttpResponse::errorContent(Buffer& buffer, std::string message)
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (HTTP_STATUS.count(_code) == 1)
    {
        status = HTTP_STATUS.find(_code)->second;
    }
    else
    {
        status = "Bad Request";
    }
    body += std::to_string(_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServer 1.0</em></body></html>";
    buffer.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buffer.append(body);
}