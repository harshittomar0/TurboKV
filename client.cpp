#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

using namespace std;

using Buffer = vector<uint8_t>;

void write_u32(Buffer &buf, uint32_t x) {
    buf.insert(buf.end(), (uint8_t*)&x, (uint8_t*)&x + 4);
}

void write_str(Buffer &buf, const string &s) {
    write_u32(buf, s.size());
    buf.insert(buf.end(), s.begin(), s.end());
}

// serialize command
Buffer build_request(vector<string> &cmd) {
    Buffer buf;
    write_u32(buf, cmd.size());
    for (auto &s : cmd) {
        write_str(buf, s);
    }

    Buffer final;
    write_u32(final, buf.size());
    final.insert(final.end(), buf.begin(), buf.end());
    return final;
}

// read exact n bytes
bool read_full(int fd, uint8_t *buf, size_t n) {
    size_t got = 0;
    while (got < n) {
        int r = read(fd, buf + got, n - got);
        if (r <= 0) return false;
        got += r;
    }
    return true;
}

// parse response (simple)
void parse_response(Buffer &buf) {
    uint8_t type = buf[0];

    if (type == 0) {
        cout << "(nil)\n";
    } else if (type == 2) { // string
        uint32_t len;
        memcpy(&len, &buf[1], 4);
        string s((char*)&buf[5], len);
        cout << s << "\n";
    } else if (type == 3) { // int
        int64_t x;
        memcpy(&x, &buf[1], 8);
        cout << x << "\n";
    } else if (type == 4) { // double
        double d;
        memcpy(&d, &buf[1], 8);
        cout << d << "\n";
    } else if (type == 1) { // error
        uint32_t code, len;
        memcpy(&code, &buf[1], 4);
        memcpy(&len, &buf[5], 4);
        string msg((char*)&buf[9], len);
        cout << "ERR: " << msg << "\n";
    } else if (type == 5) { // array
        uint32_t n;
        memcpy(&n, &buf[1], 4);
        cout << "[ ";
        size_t i = 5;
        while (i < buf.size()) {
            uint8_t t = buf[i++];
            if (t == 2) {
                uint32_t len;
                memcpy(&len, &buf[i], 4);
                i += 4;
                string s((char*)&buf[i], len);
                i += len;
                cout << s << " ";
            } else if (t == 4) {
                double d;
                memcpy(&d, &buf[i], 8);
                i += 8;
                cout << d << " ";
            }
        }
        cout << "]\n";
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    cout << "Connected to KV store\n";

    while (true) {
        cout << "kv> ";
        string line;
        getline(cin, line);

        if (line == "exit") break;

        // split input
        vector<string> cmd;
        string cur;
        for (char c : line) {
            if (c == ' ') {
                if (!cur.empty()) cmd.push_back(cur), cur.clear();
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) cmd.push_back(cur);

        if (cmd.empty()) continue;

        Buffer req = build_request(cmd);
        write(sock, req.data(), req.size());

        // read response header
        uint32_t len;
        if (!read_full(sock, (uint8_t*)&len, 4)) break;

        Buffer resp(len);
        if (!read_full(sock, resp.data(), len)) break;

        parse_response(resp);
    }

    close(sock);
}