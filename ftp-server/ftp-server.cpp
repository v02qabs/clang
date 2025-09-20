#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

#define FTP_PORT 2121  // 21 requires root privileges

void handle_client(int client_fd) {
    send(client_fd, "220 Simple FTP Server\r\n", 24, 0);

    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(client_fd, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) break;

        std::string cmd(buffer);
        if (cmd.find("USER") == 0) {
            send(client_fd, "331 Password required\r\n", 23, 0);
        } else if (cmd.find("PASS") == 0) {
            send(client_fd, "230 Login successful\r\n", 23, 0);
        } else if (cmd.find("QUIT") == 0) {
            send(client_fd, "221 Goodbye\r\n", 13, 0);
            break;
        } else {
            send(client_fd, "502 Command not implemented\r\n", 29, 0);
        }
    }

    close(client_fd);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(FTP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "FTP server listening on port " << FTP_PORT << "...\n";

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) {
            std::thread t(handle_client, client_fd);
            t.detach();
        }
    }

    close(server_fd);
    return 0;
}
