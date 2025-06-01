#ifndef __SONY_DEVICE_FINDER_H__
#define __SONY_DEVICE_FINDER_H__

#include <vector>
#include <string>
#include <libusb-1.0/libusb.h>

namespace com {
namespace sony {
namespace imaging {
namespace remote {

#define SONY_VENDOR_ID 0x054C

struct SonyDevice {
    uint8_t bus;
    uint8_t address;
    uint16_t vendor_id;
    uint16_t product_id;
    std::string product_name;
    std::string serial_number;
};

class SonyDeviceFinder {
public:
    SonyDeviceFinder();
    ~SonyDeviceFinder();
    
    // Find all Sony cameras connected via USB
    std::vector<SonyDevice> findSonyCameras();
    
    // Find specific Sony FX30 camera
    std::vector<SonyDevice> findFX30Cameras();
    
    // Check if a device is a Sony PTP camera
    bool isSonyPTPDevice(libusb_device* device);
    
private:
    libusb_context* ctx_;
    
    // Known Sony camera product IDs (can be extended)
    static const uint16_t FX30_PRODUCT_ID = 0x0CDC;  // This needs to be verified
    
    bool getDeviceInfo(libusb_device* device, SonyDevice& info);
    std::string getStringDescriptor(libusb_device_handle* handle, uint8_t index);
};

} // namespace remote
} // namespace imaging
} // namespace sony
} // namespace com

#endif // __SONY_DEVICE_FINDER_H__