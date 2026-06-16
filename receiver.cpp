#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cstdio> 

using namespace std;

int main() {
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        cerr << "Error: Unable to create UDP socket." << endl;
        return 1;
    }
    
    int opt = 1;
    setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in udp_addr;
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(8888);

    if (bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        cerr << "Error: UDP binding process failed." << endl;
        close(udp_fd);
        return 1;
    }

    cout << "Receiver online and listening for discovery pings..." << endl;

    char udp_buffer[1024];
    sockaddr_in sender_udp_addr;
    socklen_t sender_len = sizeof(sender_udp_addr);

    int received_bytes = recvfrom(udp_fd, udp_buffer, sizeof(udp_buffer) - 1, 0, 
                                  (struct sockaddr*)&sender_udp_addr, &sender_len);
    if (received_bytes <= 0) {
        cerr << "Error: Failed to catch discovery package." << endl;
        close(udp_fd);
        return 1;
    }
    udp_buffer[received_bytes] = '\0';

    if (strcmp(udp_buffer, "AIRDROP_PING") == 0) {
        cout << "Sender detected. Issuing network handshake reply..." << endl;
        string reply = "AIRDROP_PONG";
        sendto(udp_fd, reply.c_str(), reply.length(), 0, 
               (struct sockaddr*)&sender_udp_addr, sender_len);
    }
    close(udp_fd);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in tcp_addr;
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        cerr << "Error: TCP socket binding failed." << endl;
        close(server_fd);
        return 1;
    }
    listen(server_fd, 1);

    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) {
        cerr << "Error: Failed to accept connection." << endl;
        close(server_fd);
        return 1;
    }

    char file_name[256];
    long long file_size = 0;

    if (recv(client_fd, file_name, 256, 0) <= 0 || recv(client_fd, &file_size, sizeof(file_size), 0) <= 0) {
        cerr << "Error: Metadata parsing failed." << endl;
        close(client_fd);
        close(server_fd);
        return 1;
    }

    string secure_filename = "received_" + string(file_name);
    cout << "Downloading file: " << secure_filename << " (" << file_size << " bytes)" << endl;

    ofstream out_file(secure_filename, ios::binary);
    if (!out_file) {
        cerr << "Error: Cannot write file to disk." << endl;
        close(client_fd);
        close(server_fd);
        return 1;
    }

    char buffer[4096];
    long long total_bytes_received = 0;
    int bytes_read = 0;
    bool transfer_success = true;

    while (total_bytes_received < file_size) {
        long long remaining = file_size - total_bytes_received;
        int chunk_to_read = (remaining < 4096) ? remaining : 4096;

        bytes_read = recv(client_fd, buffer, chunk_to_read, 0);
        
        if (bytes_read < 0) {
            cerr << "Error: System socket error during download stream." << endl;
            transfer_success = false;
            break;
        } 
        if (bytes_read == 0) {
            cerr << "Error: Remote sender disconnected abruptly." << endl;
            transfer_success = false;
            break;
        }

        out_file.write(buffer, bytes_read);
        total_bytes_received += bytes_read;

        int progress = (total_bytes_received * 100) / file_size;
        cout << "\rTransfer progress: " << progress << "%" << flush;
    }

    out_file.close();

    if (!transfer_success) {
        cout << "Cleaning up broken/incomplete files..." << endl;
        remove(secure_filename.c_str()); 
    } else {
        cout << "\nSuccess! Local file saved as: " << secure_filename << endl;
    }

    close(client_fd);
    close(server_fd);
    return 0;
}