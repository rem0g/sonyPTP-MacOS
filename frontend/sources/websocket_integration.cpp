#include "websocket_integration.h"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace com {
namespace sony {
namespace imaging {
namespace remote {

WebSocketIntegration::WebSocketIntegration(int port, int busn, int devn)
    : server_(std::make_unique<WebSocketServer>(port)),
      command_(std::make_unique<Command>(busn, devn)),
      ptp_(nullptr),
      busn_(busn), devn_(devn) {
}

WebSocketIntegration::~WebSocketIntegration() {
    stop();
}

bool WebSocketIntegration::start() {
    // Register command handlers
    server_->registerCommand("open", [this](const std::string& msg) { return handleOpen(msg); });
    server_->registerCommand("close", [this](const std::string& msg) { return handleClose(msg); });
    server_->registerCommand("send", [this](const std::string& msg) { return handleSend(msg); });
    server_->registerCommand("recv", [this](const std::string& msg) { return handleRecv(msg); });
    server_->registerCommand("wait", [this](const std::string& msg) { return handleWait(msg); });
    server_->registerCommand("auth", [this](const std::string& msg) { return handleAuth(msg); });
    server_->registerCommand("getall", [this](const std::string& msg) { return handleGetAll(msg); });
    server_->registerCommand("get", [this](const std::string& msg) { return handleGet(msg); });
    server_->registerCommand("getobject", [this](const std::string& msg) { return handleGetObject(msg); });
    server_->registerCommand("getliveview", [this](const std::string& msg) { return handleGetLiveView(msg); });
    server_->registerCommand("reset", [this](const std::string& msg) { return handleReset(msg); });
    server_->registerCommand("clear", [this](const std::string& msg) { return handleClearHalt(msg); });
    
    return server_->start();
}

void WebSocketIntegration::stop() {
    server_->stop();
}

std::string WebSocketIntegration::handleOpen(const std::string& message) {
    if (!ptp_) {
        ptp_ = new socc_ptp(busn_, devn_);
    }
    
    int result = ptp_->connect();
    if (result == 0) {
        return successToJson("Device opened successfully");
    }
    return errorToJson("Failed to open device");
}

std::string WebSocketIntegration::handleClose(const std::string& message) {
    if (ptp_) {
        ptp_->disconnect();
        delete ptp_;
        ptp_ = nullptr;
    }
    return successToJson("Device closed");
}

std::string WebSocketIntegration::handleSend(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    PTPTransaction transaction = parseTransaction(message);
    int result = command_->send(ptp_, &transaction);
    if (result == 0) {
        return transactionToJson(transaction);
    }
    return errorToJson("Send command failed");
}

std::string WebSocketIntegration::handleRecv(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    PTPTransaction transaction = parseTransaction(message);
    int result = command_->recv(ptp_, &transaction);
    if (result == 0) {
        return transactionToJson(transaction);
    }
    return errorToJson("Receive command failed");
}

std::string WebSocketIntegration::handleWait(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    int result = command_->wait(ptp_);
    if (result == 0) {
        return successToJson("Event received");
    }
    return errorToJson("Wait command failed");
}

std::string WebSocketIntegration::handleAuth(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    int result = command_->auth(ptp_);
    if (result == 0) {
        return successToJson("Authentication successful");
    }
    return errorToJson("Authentication failed");
}

std::string WebSocketIntegration::handleGetAll(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    int result = command_->getall(ptp_);
    if (result == 0) {
        return successToJson("Get all properties successful");
    }
    return errorToJson("Get all properties failed");
}

std::string WebSocketIntegration::handleGet(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    // Parse device property code from message
    size_t colonPos = message.find(':');
    if (colonPos == std::string::npos) {
        return errorToJson("Invalid get command format");
    }
    
    std::string params = message.substr(colonPos + 1);
    uint16_t propertyCode = std::stoi(params, nullptr, 0);
    
    int result = command_->get(ptp_, propertyCode);
    if (result == 0) {
        return successToJson("Get property successful");
    }
    return errorToJson("Get property failed");
}

std::string WebSocketIntegration::handleGetObject(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    // Parse object handle from message
    size_t colonPos = message.find(':');
    if (colonPos == std::string::npos) {
        return errorToJson("Invalid getobject command format");
    }
    
    std::string params = message.substr(colonPos + 1);
    uint32_t handle = std::stoul(params, nullptr, 0);
    
    int result = command_->getobject(ptp_, handle);
    if (result == 0) {
        return successToJson("Get object successful");
    }
    return errorToJson("Get object failed");
}

std::string WebSocketIntegration::handleGetLiveView(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    int result = command_->getliveview(ptp_);
    if (result == 0) {
        return successToJson("Get live view successful");
    }
    return errorToJson("Get live view failed");
}

std::string WebSocketIntegration::handleReset(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    command_->reset(ptp_);
    return successToJson("Device reset");
}

std::string WebSocketIntegration::handleClearHalt(const std::string& message) {
    if (!ptp_) {
        return errorToJson("Device not connected");
    }
    
    command_->clear_halt(ptp_);
    return successToJson("Clear halt successful");
}

PTPTransaction WebSocketIntegration::parseTransaction(const std::string& params) {
    PTPTransaction transaction;
    memset(&transaction, 0, sizeof(transaction));
    
    // Parse parameters from format: "command:op=0x1234,p1=0x5678,p2=0x9ABC,data=123,size=4"
    size_t colonPos = params.find(':');
    if (colonPos == std::string::npos) {
        return transaction;
    }
    
    std::string paramStr = params.substr(colonPos + 1);
    std::istringstream iss(paramStr);
    std::string param;
    
    while (std::getline(iss, param, ',')) {
        size_t eqPos = param.find('=');
        if (eqPos != std::string::npos) {
            std::string key = param.substr(0, eqPos);
            std::string value = param.substr(eqPos + 1);
            
            if (key == "op") {
                transaction.code = std::stoi(value, nullptr, 0);
            } else if (key == "p1") {
                transaction.params[0] = std::stoul(value, nullptr, 0);
                if (transaction.nparam < 1) transaction.nparam = 1;
            } else if (key == "p2") {
                transaction.params[1] = std::stoul(value, nullptr, 0);
                if (transaction.nparam < 2) transaction.nparam = 2;
            } else if (key == "p3") {
                transaction.params[2] = std::stoul(value, nullptr, 0);
                if (transaction.nparam < 3) transaction.nparam = 3;
            } else if (key == "p4") {
                transaction.params[3] = std::stoul(value, nullptr, 0);
                if (transaction.nparam < 4) transaction.nparam = 4;
            } else if (key == "p5") {
                transaction.params[4] = std::stoul(value, nullptr, 0);
                if (transaction.nparam < 5) transaction.nparam = 5;
            } else if (key == "data") {
                transaction.data.send = std::stoul(value, nullptr, 0);
            } else if (key == "size") {
                transaction.size = std::stoul(value, nullptr, 0);
            }
        }
    }
    
    return transaction;
}

std::string WebSocketIntegration::transactionToJson(const PTPTransaction& transaction) {
    std::stringstream json;
    json << "{";
    json << "\"code\": \"0x" << std::hex << transaction.code << "\",";
    json << "\"nparam\": " << std::dec << transaction.nparam << ",";
    json << "\"params\": [";
    for (int i = 0; i < transaction.nparam; i++) {
        if (i > 0) json << ",";
        json << "\"0x" << std::hex << transaction.params[i] << "\"";
    }
    json << "],";
    json << "\"size\": " << std::dec << transaction.size << ",";
    json << "\"data\": \"0x" << std::hex << transaction.data.send << "\"";
    json << "}";
    return json.str();
}

std::string WebSocketIntegration::errorToJson(const std::string& error) {
    return "{\"error\": \"" + error + "\"}";
}

std::string WebSocketIntegration::successToJson(const std::string& result) {
    if (result.empty()) {
        return "{\"success\": true}";
    }
    return "{\"success\": true, \"result\": \"" + result + "\"}";
}

} // namespace remote
} // namespace imaging
} // namespace sony
} // namespace com