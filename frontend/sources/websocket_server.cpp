#include "websocket_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

namespace com {
namespace sony {
namespace imaging {
namespace remote {

static const char* WS_MAGIC_STRING = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

WebSocketServer::WebSocketServer(int port) 
    : port_(port), server_fd_(-1), running_(false) {
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        return false;
    }
    
    // Create socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        return false;
    }
    
    // Allow reuse of address
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    // Bind to port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);
    
    if (bind(server_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    // Listen
    if (listen(server_fd_, 5) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    running_ = true;
    server_thread_ = std::thread(&WebSocketServer::serverLoop, this);
    
    return true;
}

void WebSocketServer::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void WebSocketServer::registerCommand(const std::string& command, CommandHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    command_handlers_[command] = handler;
}

void WebSocketServer::serverLoop() {
    while (running_) {
        struct pollfd pfd;
        pfd.fd = server_fd_;
        pfd.events = POLLIN;
        
        int ret = poll(&pfd, 1, 1000); // 1 second timeout
        if (ret > 0 && (pfd.revents & POLLIN)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd >= 0) {
                // Handle client in a separate thread
                std::thread client_thread(&WebSocketServer::handleClient, this, client_fd);
                client_thread.detach();
            }
        }
    }
}

void WebSocketServer::handleClient(int client_fd) {
    // Perform WebSocket handshake
    if (!performWebSocketHandshake(client_fd)) {
        close(client_fd);
        return;
    }
    
    // Read WebSocket frames
    std::vector<uint8_t> buffer(4096);
    while (running_) {
        int n = recv(client_fd, buffer.data(), buffer.size(), 0);
        if (n <= 0) {
            break;
        }
        
        std::string message = decodeWebSocketFrame(std::vector<uint8_t>(buffer.begin(), buffer.begin() + n));
        if (!message.empty()) {
            std::string response = processCommand(message);
            auto encoded = encodeWebSocketFrame(response);
            send(client_fd, encoded.data(), encoded.size(), 0);
        }
    }
    
    close(client_fd);
}

std::string WebSocketServer::processCommand(const std::string& message) {
    // Parse JSON command
    // For simplicity, we'll use a basic format: "command:arg1,arg2,..."
    size_t colonPos = message.find(':');
    std::string command = (colonPos != std::string::npos) ? message.substr(0, colonPos) : message;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = command_handlers_.find(command);
    if (it != command_handlers_.end()) {
        return it->second(message);
    }
    
    return "{\"error\": \"Unknown command: " + command + "\"}";
}

std::string WebSocketServer::generateWebSocketAccept(const std::string& key) {
    std::string combined = key + WS_MAGIC_STRING;
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)combined.c_str(), combined.length(), hash);
    
    // Base64 encode
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    BIO_write(bio, hash, SHA_DIGEST_LENGTH);
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    return result;
}

bool WebSocketServer::performWebSocketHandshake(int client_fd) {
    char buffer[1024];
    int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        return false;
    }
    buffer[n] = '\0';
    
    std::string request(buffer);
    
    // Extract WebSocket key
    std::string wsKey;
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    if (keyPos != std::string::npos) {
        keyPos += 19; // Length of "Sec-WebSocket-Key: "
        size_t endPos = request.find("\r\n", keyPos);
        if (endPos != std::string::npos) {
            wsKey = request.substr(keyPos, endPos - keyPos);
        }
    }
    
    if (wsKey.empty()) {
        return false;
    }
    
    // Generate accept key
    std::string acceptKey = generateWebSocketAccept(wsKey);
    
    // Send handshake response
    std::stringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << acceptKey << "\r\n";
    response << "\r\n";
    
    std::string responseStr = response.str();
    send(client_fd, responseStr.c_str(), responseStr.length(), 0);
    
    return true;
}

std::string WebSocketServer::decodeWebSocketFrame(const std::vector<uint8_t>& data) {
    if (data.size() < 2) {
        return "";
    }
    
    bool fin = (data[0] & 0x80) != 0;
    uint8_t opcode = data[0] & 0x0F;
    (void)fin; // TODO: Handle fragmented messages
    (void)opcode; // TODO: Handle different opcodes
    bool masked = (data[1] & 0x80) != 0;
    uint64_t payloadLen = data[1] & 0x7F;
    
    size_t pos = 2;
    
    if (payloadLen == 126) {
        if (data.size() < pos + 2) return "";
        payloadLen = (data[pos] << 8) | data[pos + 1];
        pos += 2;
    } else if (payloadLen == 127) {
        if (data.size() < pos + 8) return "";
        payloadLen = 0;
        for (int i = 0; i < 8; i++) {
            payloadLen = (payloadLen << 8) | data[pos + i];
        }
        pos += 8;
    }
    
    std::vector<uint8_t> mask(4);
    if (masked) {
        if (data.size() < pos + 4) return "";
        std::copy(data.begin() + pos, data.begin() + pos + 4, mask.begin());
        pos += 4;
    }
    
    if (data.size() < pos + payloadLen) return "";
    
    std::string message;
    message.reserve(payloadLen);
    
    for (uint64_t i = 0; i < payloadLen; i++) {
        if (masked) {
            message += data[pos + i] ^ mask[i % 4];
        } else {
            message += data[pos + i];
        }
    }
    
    return message;
}

std::vector<uint8_t> WebSocketServer::encodeWebSocketFrame(const std::string& message) {
    std::vector<uint8_t> frame;
    
    // FIN = 1, opcode = 1 (text)
    frame.push_back(0x81);
    
    size_t len = message.length();
    if (len < 126) {
        frame.push_back(static_cast<uint8_t>(len));
    } else if (len < 65536) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    
    // Add payload
    frame.insert(frame.end(), message.begin(), message.end());
    
    return frame;
}

} // namespace remote
} // namespace imaging
} // namespace sony
} // namespace com