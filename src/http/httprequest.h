#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <error.h>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.cc"

class HttpRequest
{
using string = std::string;

public:
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    enum HTTP_CODE
    {
        NO_REQEUST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_REQUEST,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNERL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest()
    {
        init();
    }

    ~HttpRequest() = default;

    void init();
    bool parse(Buffer& buff);

    string path() const;
    string& path();
    string method() const;
    string version() const;
    string getPost(const std::string& key) const;
    string getPost(const char* key) const;

    bool isKeepAlive() const;

private:
    bool parseRquestLine(const string& line);
    void parseHeader(const string& line);
    void parseBody(const string& line);

    void parsePath();
    void parsePost();
    void parseFromUrlencoded();

    static bool verifyUser(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE _state;
    string _method, _path, _version, _body;
    std::unordered_map<string, string> _header;
    std::unordered_map<string, string> _post;

    static const std::unordered_set<string> DEFAULT_HTML;
    static const std::unordered_map<string, int> DEFAULT_HTML_TAG;
    static int convertToHex(char ch);
};

#endif // !HTTP_REQUEST