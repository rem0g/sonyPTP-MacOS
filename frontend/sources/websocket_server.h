#ifndef __WEBSOCKET_SERVER_H__
#define __WEBSOCKET_SERVER_H__

#include <string>
#include <functional>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include "socc_types.h"

namespace com {
namespace sony {
namespace imaging {
namespace remote {

class WebSocketServer {
public:
    typedef std::function<std::string(const std::string&)> CommandHandler;
    
    WebSocketServer(int port);
    ~WebSocketServer();
    
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    void registerCommand(const std::string& command, CommandHandler handler);
    
private:
    int port_;
    int server_fd_;
    bool running_;
    std::thread server_thread_;
    std::map<std::string, CommandHandler> command_handlers_;
    mutable std::mutex mutex_;
    
    void serverLoop();
    void handleClient(int client_fd);
    std::string processCommand(const std::string& message);
    
    // WebSocket specific
    std::string generateWebSocketAccept(const std::string& key);
    bool performWebSocketHandshake(int client_fd);
    std::string decodeWebSocketFrame(const std::vector<uint8_t>& data);
    std::vector<uint8_t> encodeWebSocketFrame(const std::string& message);
};

} // namespace remote
} // namespace imaging
} // namespace sony
} // namespace com

#endif // __WEBSOCKET_SERVER_H__