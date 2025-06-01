#include <stdio.h>
#include <libusb-1.0/libusb.h>

int main() {
    libusb_device **devs;
    libusb_context *ctx = NULL;
    int r, i = 0;
    ssize_t cnt;

    r = libusb_init(&ctx);
    if (r < 0) {
        fprintf(stderr, "Init Error %d\n", r);
        return 1;
    }

    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        fprintf(stderr, "Get Device Error\n");
        return 1;
    }

    printf("USB Devices:\n");
    printf("Bus\tDevice\tVendor:Product\tDescription\n");
    printf("---\t------\t--------------\t-----------\n");

    for (i = 0; devs[i] != NULL; i++) {
        struct libusb_device_descriptor desc;
        r = libusb_get_device_descriptor(devs[i], &desc);
        if (r < 0) {
            fprintf(stderr, "Failed to get device descriptor\n");
            continue;
        }

        uint8_t bus = libusb_get_bus_number(devs[i]);
        uint8_t addr = libusb_get_device_address(devs[i]);

        printf("%03d\t%03d\t%04x:%04x\t", bus, addr, desc.idVendor, desc.idProduct);

        // Try to get product name
        libusb_device_handle *handle;
        r = libusb_open(devs[i], &handle);
        if (r == 0) {
            unsigned char string[256];
            if (desc.iProduct) {
                r = libusb_get_string_descriptor_ascii(handle, desc.iProduct, string, sizeof(string));
                if (r > 0) {
                    printf("%s", string);
                }
            }
            libusb_close(handle);
        }
        printf("\n");
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(ctx);
    return 0;
}