#include <netdb.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define FTP_PORT 2121  // 権限不要ポートで実行

// 固定ユーザー名とパスワード
const std::string FIXED_USER = "hiro";
const std::string FIXED_PASS = "hirofumi";

// パッシブモード用
int pasv_fd = -1;
int pasv_port = 0;

// サーバー自身のローカルIPを返す (LAN内で使用)
std::string getServerIP() {
    char host[256];
    if (gethostname(host, sizeof(host)) == 0) {
        struct hostent *he = gethostbyname(host);
        if (he) {
            struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
            if (addr_list[0]) {
                return std::string(inet_ntoa(*addr_list[0]));
            }
        }
    }
    // fallback
    return "127.0.0.1";
}

void handle_client(int client_fd) {
    send(client_fd, "220 Simple FTP Server\r\n", 24, 0);

    char buffer[1024];
    bool userOk = false;
    bool passOk = false;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        std::string cmd(buffer);
        cmd.erase(cmd.find_last_not_of("\r\n") + 1);

        if (cmd.rfind("USER", 0) == 0) {
            std::string user = cmd.substr(5);
            if (user == FIXED_USER) {
                userOk = true;
                send(client_fd, "331 Username OK, need password\r\n", 32, 0);
            } else {
                send(client_fd, "530 Invalid username\r\n", 23, 0);
            }

        } else if (cmd.rfind("PASS", 0) == 0) {
            std::string pass = cmd.substr(5);
            if (userOk && pass == FIXED_PASS) {
                passOk = true;
                send(client_fd, "230 Login successful\r\n", 23, 0);
            } else {
                send(client_fd, "530 Invalid password\r\n", 23, 0);
            }

        } else if (cmd.rfind("SYST", 0) == 0) {
            send(client_fd, "215 UNIX Type: L8\r\n", 20, 0);

        } else if (cmd.rfind("TYPE", 0) == 0) {
            send(client_fd, "200 Type set to I\r\n", 20, 0);

        } else if (cmd.rfind("PWD", 0) == 0) {
            send(client_fd, "257 \"/\" is the current directory\r\n", 34, 0);

        } else if (cmd.rfind("FEAT", 0) == 0) {
            send(client_fd, "211 End\r\n", 9, 0);

        } else if (cmd.rfind("PASV", 0) == 0) {
            if (pasv_fd != -1) close(pasv_fd);

            pasv_fd = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in pasv_addr{};
            pasv_addr.sin_family = AF_INET;
            pasv_addr.sin_addr.s_addr = INADDR_ANY;
            pasv_addr.sin_port = 0; // 自動割当て

            bind(pasv_fd, (sockaddr*)&pasv_addr, sizeof(pasv_addr));
            listen(pasv_fd, 1);

            socklen_t len = sizeof(pasv_addr);
            getsockname(pasv_fd, (sockaddr*)&pasv_addr, &len);
            pasv_port = ntohs(pasv_addr.sin_port);

            std::string server_ip = getServerIP();
            unsigned int ip[4];
            sscanf(server_ip.c_str(), "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
            int p1 = pasv_port / 256;
            int p2 = pasv_port % 256;

            std::string reply = "227 Entering Passive Mode (" +
                std::to_string(ip[0]) + "," + std::to_string(ip[1]) + "," +
                std::to_string(ip[2]) + "," + std::to_string(ip[3]) + "," +
                std::to_string(p1) + "," + std::to_string(p2) + ")\r\n";

            send(client_fd, reply.c_str(), reply.size(), 0);

        } else if (cmd.rfind("LIST", 0) == 0) {
            if (passOk && pasv_fd != -1) {
                send(client_fd, "150 Here comes the directory listing\r\n", 38, 0);

                int data_fd = accept(pasv_fd, nullptr, nullptr);
                if (data_fd >= 0) {
                    DIR *dir = opendir(".");
                    if (dir) {
                        struct dirent *entry;
                        while ((entry = readdir(dir)) != NULL) {
                            std::string line = entry->d_name;
                            line += "\r\n";
                            send(data_fd, line.c_str(), line.size(), 0);
                        }
                        closedir(dir);
                    }
                    close(data_fd);
                }
                close(pasv_fd);
                pasv_fd = -1;

                send(client_fd, "226 Directory send OK\r\n", 24, 0);
            } else {
                send(client_fd, "530 Please login with USER and PASS\r\n", 37, 0);
            }

        } else if (cmd.rfind("QUIT", 0) == 0) {
            send(client_fd, "221 Goodbye\r\n", 13, 0);
            break;

        } else {
            if (passOk) {
                send(client_fd, "502 Command not implemented\r\n", 29, 0);
            } else {
                send(client_fd, "530 Please login with USER and PASS\r\n", 37, 0);
            }
        }
    }

    if (pasv_fd != -1) close(pasv_fd);
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
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd >= 0) {
            std::thread t(handle_client, client_fd);
            t.detach();
        }
    }

    close(server_fd);
    return 0;
}
