#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
    string filename = "sample.jpg"; 
    
    ifstream in_file(filename, ios::binary);
    if (!in_file) {
        cerr << "Error: Local file " << filename << " does not exist." << endl;
        return 1;
    }

    in_file.seekg(0, ios::end);
    long long file_size = in_file.tellg();
    in_file.seekg(0, ios::beg);

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int broadcast_enable = 1;
    setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

    sockaddr_in broadcast_addr;
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(8888);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    cout << "Broadcasting network ping to find active receivers..." << endl;
    string ping_msg = "AIRDROP_PING";
    sendto(udp_fd, ping_msg.c_str(), ping_msg.length(), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

    char udp_buffer[1024];
    sockaddr_in receiver_addr; 
    socklen_t receiver_len = sizeof(receiver_addr);

    int received_bytes = recvfrom(udp_fd, udp_buffer, sizeof(udp_buffer) - 1, 0, (struct sockaddr*)&receiver_addr, &receiver_len);
    if (received_bytes <= 0) {
        cerr << "Error: No response from network receivers." << endl;
        close(udp_fd);
        return 1;
    }
    udp_buffer[received_bytes] = '\0';

    char receiver_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(receiver_addr.sin_addr), receiver_ip, INET_ADDRSTRLEN);

    if (strcmp(udp_buffer, "AIRDROP_PONG") != 0) {
        cerr << "Error: Handshake validation failed." << endl;
        close(udp_fd);
        return 1;
    }
    cout << "Receiver discovered successfully at IP: " << receiver_ip << endl;
    close(udp_fd);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_tcp_addr;
    server_tcp_addr.sin_family = AF_INET;
    server_tcp_addr.sin_port = htons(8080);
    server_tcp_addr.sin_addr = receiver_addr.sin_addr;

    cout << "Connecting to receiver over TCP..." << endl;
    if (connect(sock_fd, (struct sockaddr*)&server_tcp_addr, sizeof(server_tcp_addr)) < 0) {
        cerr << "Error: TCP connection failed." << endl;
        in_file.close();
        return 1;
    }

    char name_buffer[256];
    memset(name_buffer, 0, 256);
    strncpy(name_buffer, filename.c_str(), 255);
    
    if (send(sock_fd, name_buffer, 256, 0) < 0 || send(sock_fd, &file_size, sizeof(file_size), 0) < 0) {
        cerr << "Error: Failed to transmit metadata headers." << endl;
        in_file.close();
        close(sock_fd);
        return 1;
    }

    char buffer[4096];
    cout << "Streaming file bytes..." << endl;
    while (in_file.read(buffer, sizeof(buffer)) || in_file.gcount() > 0) {
        int bytes_to_send = in_file.gcount();
        int sent_bytes = send(sock_fd, buffer, bytes_to_send, 0);
        
        if (sent_bytes < 0) {
            cerr << "Error: Socket connection dropped mid-transfer." << endl;
            in_file.close();
            close(sock_fd);
            return 1;
        }
    }

    cout << "File sent successfully!" << endl;
    in_file.close();
    close(sock_fd);
    return 0;
}