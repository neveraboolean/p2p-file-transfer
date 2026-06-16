# P2P LAN File Streaming Utility

A lightweight, zero-configuration peer-to-peer file transfer utility written in C++ using raw Linux sockets. The application enables seamless automatic device discovery over a shared local network interface and handles high-efficiency file streaming without high system memory overhead.

## Key Features

* **Zero-Config Auto-Discovery:** Utilizes **UDP Broadcast** capabilities to dynamically scan the local network, locate active receiver instances, and exchange identity handshakes without hardcoded IP configurations.
* **Hybrid Socket Architecture:** Seamlessly switches from a connectionless UDP handshake model to a reliable connection-oriented **TCP Socket stream** for targeted data transmission.
* **Memory-Optimized Streaming:** Implements data splitting into fixed **4KB memory chunks**, mitigating risk of system RAM exhaustion and safely enabling multi-gigabyte file transfers.
* **Custom Application Protocol:** Features a tailored application-layer protocol header to transmit file metadata dynamically and handle graceful cleanup in case of unexpected socket drops.

## 🛠️ Tech Stack & Concepts

* **Language:** C++
* **Networking APIs:** POSIX Sockets (`sys/socket.h`, `arpa/inet.h`, `netinet/in.h`)
* **Protocols:** UDP (Discovery/Handshake) + TCP (File Streaming)
* **Architecture:** Peer-to-Peer (P2P) / Client-Server Hybrid

---

## Project Structure

```text
p2p-file-transfer/
├── README.md          # Project documentation
├── .gitignore         # Prevents tracking compiled binaries
├── sender.cpp         # Source code for the file sender (Client)
└── receiver.cpp       # Source code for the file receiver (Server)
