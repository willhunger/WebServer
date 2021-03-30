#include "server/webserver.h"
int main()
{
    WebServer server(
        64398, 3, 60000, true,
        3306, "root", "1213", "webserver",
        12, 4
    );
    server.start();
}
