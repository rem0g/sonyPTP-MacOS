#include "sony_device_finder.h"
#include <sstream>
#include <iomanip>

namespace com {
namespace sony {
namespace imaging {
namespace remote {

SonyDeviceFinder::SonyDeviceFinder() : ctx_(nullptr) {
    libusb_init(&ctx_);
}

SonyDeviceFinder::~SonyDeviceFinder() {
    if (ctx_) {
        libusb_exit(ctx_);
    }
}

std::vector<SonyDevice> SonyDeviceFinder::findSonyCameras() {
    std::vector<SonyDevice> devices;
    libusb_device** devs;
    
    ssize_t cnt = libusb_get_device_list(ctx_, &devs);
    if (cnt < 0) {
        return devices;
    }
    
    for (ssize_t i = 0; i < cnt; i++) {
        SonyDevice info;
        if (getDeviceInfo(devs[i], info)) {
            if (info.vendor_id == SONY_VENDOR_ID && isSonyPTPDevice(devs[i])) {
                devices.push_back(info);
            }
        }
    }
    
    libusb_free_device_list(devs, 1);
    return devices;
}

std::vector<SonyDevice> SonyDeviceFinder::findFX30Cameras() {
    std::vector<SonyDevice> allSony = findSonyCameras();
    std::vector<SonyDevice> fx30s;
    
    for (const auto& device : allSony) {
        // Check if product name contains "FX30" or has specific product ID
        if (device.product_name.find("FX30") != std::string::npos ||
            device.product_id == FX30_PRODUCT_ID) {
            fx30s.push_back(device);
        }
    }
    
    return fx30s;
}

bool SonyDeviceFinder::isSonyPTPDevice(libusb_device* device) {
    struct libusb_config_descriptor* config;
    
    if (libusb_get_active_config_descriptor(device, &config) < 0) {
        return false;
    }
    
    bool isPTP = false;
    
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface* interface = &config->interface[i];
        for (int j = 0; j < interface->num_altsetting; j++) {
            const struct libusb_interface_descriptor* altsetting = &interface->altsetting[j];
            if (altsetting->bInterfaceClass == LIBUSB_CLASS_PTP) {
                isPTP = true;
                break;
            }
        }
        if (isPTP) break;
    }
    
    libusb_free_config_descriptor(config);
    return isPTP;
}

bool SonyDeviceFinder::getDeviceInfo(libusb_device* device, SonyDevice& info) {
    struct libusb_device_descriptor desc;
    
    if (libusb_get_device_descriptor(device, &desc) < 0) {
        return false;
    }
    
    info.bus = libusb_get_bus_number(device);
    info.address = libusb_get_device_address(device);
    info.vendor_id = desc.idVendor;
    info.product_id = desc.idProduct;
    
    // Try to get product name and serial number
    libusb_device_handle* handle;
    if (libusb_open(device, &handle) == 0) {
        info.product_name = getStringDescriptor(handle, desc.iProduct);
        info.serial_number = getStringDescriptor(handle, desc.iSerialNumber);
        libusb_close(handle);
    }
    
    return true;
}

std::string SonyDeviceFinder::getStringDescriptor(libusb_device_handle* handle, uint8_t index) {
    if (index == 0) {
        return "";
    }
    
    unsigned char buffer[256];
    int ret = libusb_get_string_descriptor_ascii(handle, index, buffer, sizeof(buffer));
    
    if (ret > 0) {
        return std::string(reinterpret_cast<char*>(buffer));
    }
    
    return "";
}

} // namespace remote
} // namespace imaging
} // namespace sony
} // namespace com