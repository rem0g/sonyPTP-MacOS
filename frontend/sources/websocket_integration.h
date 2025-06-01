#ifndef __WEBSOCKET_INTEGRATION_H__
#define __WEBSOCKET_INTEGRATION_H__

#include "websocket_server.h"
#include "command.h"
#include <memory>

namespace com {
namespace sony {
namespace imaging {
namespace remote {

class WebSocketIntegration {
public:
    WebSocketIntegration(int port, int busn = 0, int devn = 0);
    ~WebSocketIntegration();
    
    bool start();
    void stop();
    
private:
    std::unique_ptr<WebSocketServer> server_;
    std::unique_ptr<Command> command_;
    socc_ptp* ptp_;
    int busn_;
    int devn_;
    
    // Command handlers
    std::string handleOpen(const std::string& message);
    std::string handleClose(const std::string& message);
    std::string handleSend(const std::string& message);
    std::string handleRecv(const std::string& message);
    std::string handleWait(const std::string& message);
    std::string handleAuth(const std::string& message);
    std::string handleGetAll(const std::string& message);
    std::string handleGet(const std::string& message);
    std::string handleGetObject(const std::string& message);
    std::string handleGetLiveView(const std::string& message);
    std::string handleReset(const std::string& message);
    std::string handleClearHalt(const std::string& message);
    
    // Helper functions
    PTPTransaction parseTransaction(const std::string& params);
    std::string transactionToJson(const PTPTransaction& transaction);
    std::string errorToJson(const std::string& error);
    std::string successToJson(const std::string& result = "");
};

} // namespace remote
} // namespace imaging
} // namespace sony
} // namespace com

#endif // __WEBSOCKET_INTEGRATION_H__