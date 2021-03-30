#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML
{
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture", 
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG 
{
    {"/register.html", 0}, 
    {"/login.html", 1},  
};

void HttpRequest::init()
{
    _method = _path = _version = _body = "";
    _state = REQUEST_LINE;
    _header.clear();
    _post.clear();
}

bool HttpRequest::parse(Buffer& buff)
{
    const char CRLF[] = "\r\n";
    if (buff.readableBytes() <= 0)
        return false;
    while (buff.readableBytes() && _state != FINISH)
    {
        const char* lineEnd = std::search(buff.peek(), buff.beginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.peek(), lineEnd);
        switch (_state)
        {
            case REQUEST_LINE:
                if (!parseRquestLine(line))
                {
                    return false;
                }
                parsePath();
                break;
            case HEADERS:
                parseHeader(line);
                if (buff.readableBytes() <= 2)
                {
                    _state = FINISH;
                }
                break;
            case BODY:
                parseBody(line);
                break;
            default:
                break;
        }
        if (lineEnd == buff.beginWrite())
            break;
        buff.retrieveUntil(lineEnd + 2);
    }
    return true;
}

std::string HttpRequest:: path() const
{
    return _path;
}

std::string& HttpRequest::path()
{
    return _path;
}

std::string HttpRequest:: method() const
{
    return _method;
}

std::string HttpRequest:: version() const
{
    return _version;
}

std::string HttpRequest:: getPost(const std::string& key) const
{
    assert(key != "");
    if (_post.count(key) == 1)
    {
        return _post.find(key)->second;
    }
    return "";
}

std::string HttpRequest:: getPost(const char* key) const
{
    assert(key != nullptr);
    if (_post.count(key) == 1)
    {
        return _post.find(key)->second;
    }
    return "";
}

bool HttpRequest::isKeepAlive() const
{
    if (_header.count("Connection") == 1)
    {
        return (_header.find("Connection")->second == "keep-alive") && (_version == "1.1");
    }
    return false;
}

void HttpRequest::parsePath()
{
    if (_path == "/")
        _path = "/index.html";
    else
    {
        for (auto &item : DEFAULT_HTML) 
        {
            if (item == _path){
                _path += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::parseRquestLine(const string& line)
{
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern))
    {
        _method = subMatch[1];
        _path = subMatch[2];
        _version = subMatch[3];
        _state = HEADERS;
        return true;
    }
    return false;
}

void HttpRequest::parseHeader(const string& line)
{
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern))
    {
        _header[subMatch[1]] = subMatch[2];
    }
    else
    {
        _state = BODY;
    }
}

void HttpRequest::parseBody(const string& line)
{
    _body = line;
    parsePost();
    _state = FINISH;
}

int HttpRequest::convertToHex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;   
    return ch;
}

void HttpRequest::parsePost()
{
    if (_method == "POST" && _header["Content-Type"] == "application/x-www-form-urlencoded")
    {
        parseFromUrlencoded();
        if (DEFAULT_HTML_TAG.count(_path))
        {
            int tag = DEFAULT_HTML_TAG.find(_path)->second;
            if (tag == 0 || tag == 1)
            {
                bool isLogin = (tag == 1);
                if (verifyUser(_post["username"], _post["password"], isLogin))
                {
                    _path = "/welcome.html";
                }
                else
                {
                    _path = "/error.html";
                }
            }

        }
    }   
}

void HttpRequest::parseFromUrlencoded()
{
    if (_body.size() == 0)
        return;
    string key, value;
    int num = 0;
    const int n = _body.size();
    int i = 0, j = 0;
    for (; i < n; ++i)
    {
        char ch = _body[i];
        switch(ch)
        {
            case '=':
                key = _body.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                _body[i] = ' ';
                break;
            case '%':
                num = convertToHex(_body[i + 1]) * 16 + convertToHex(_body[i + 2]);
                _body[i + 2] = num % 10 + '0';
                _body[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = _body.substr(j, i - j);
                j = i + 1;
                _post[key] = value;
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if (_post.count(key) == 0 && j < i)
    {
        value = _body.substr(j, i - j);
        _post[key] = value;
    }
}

bool HttpRequest::verifyUser(const string& name, const string& pwd, bool isLogin)
{
    if (name == "" || pwd == "")
        return false;
    MYSQL* sql;
    sqlConnRAII(&sql, sqlConnPool::instance());
    assert(sql);
    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin)
    {
        flag = true;
    }

    snprintf(order, 256, "SELECT username, password FROM user WHERE username = '%s' LIMIT 1", name.c_str());
    if (mysql_query(sql, order))
    {
        mysql_free_result(res);
        return false;
    }

    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_field(res);

    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        string password(row[1]);
        if (isLogin)
        {
            if (pwd == password)
            {
                flag = true;
            }
            else
            {
                flag = false;
            }
        }
        else
        {
            flag = false;
        }
    }
    mysql_free_result(res);

    if (!isLogin && flag == true)
    {
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), pwd.c_str());
        if (mysql_query(sql, order))
        {
            flag = false;
        }
        flag = true;
    }
    sqlConnPool::instance()->freeConn(sql);
    return flag;    
};
