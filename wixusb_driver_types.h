/*
 * The MIT License
 *
 * Copyright 2017 Ildar Sadykov.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef WIXUSB_DRIVER_TYPES_H
#define WIXUSB_DRIVER_TYPES_H

#ifndef __KERNEL__
#include <stdint.h>
#else
#include <linux/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define _Out_bytecap_(size)
#define _In_
#define _Out_opt_ 
#define _Out_
#define _Inout_
#define TRUE                    true
#define FALSE                   false
#define CHAR                    char
#define LONG                    long
#define DEVICE_DATA             int
#define HRESULT                 int32_t
#define ULONG                   uint32_t
#define UCHAR                   uint8_t
#define PDEVICE_DATA            DEVICE_DATA *
#define BOOL                    LONG
#define PBOOL                   BOOL *
#define LPTSTR                  ULONG
#define USHORT                  uint16_t
#define FAILED(x)               (x < 0)
#define BYTE                    uint8_t 
#define PBYTE                   BYTE *
#define PUCHAR                  UCHAR *
#define PULONG                  ULONG *
#define LPOVERLAPPED            PULONG
#define ERROR_SEM_TIMEOUT       0x79
#define UINT32                  uint32_t
#define WORD                    uint16_t

#define EP_SIZE                 0x40

#define WIXUSB_IO_BUFFSIZE        256

#define CTRL_BUFF_LENGTH        128
#define DESC_BUFF_LENGTH        128
#define BULK_BUFF_LENGTH        4096

#define SETUP_PACKET_IS_INPUT(bmRequestType)  ((bmRequestType & (1 << 7) ? 1 : 0))

    
typedef struct _WINUSB_SETUP_PACKET {
    UCHAR RequestType;
    UCHAR Request;
    USHORT Value;
    USHORT Index;
    USHORT Length;
} WINUSB_SETUP_PACKET, *PWINUSB_SETUP_PACKET;

typedef enum  {
    USB_DEVICE_DESCRIPTOR_TYPE = 0x01, // Стандартный дескриптор устройства
    USB_CONFIGURATION_DESCRIPTOR_TYPE = 0x02, // Дескриптор конфигурации
    USB_STRING_DESCRIPTOR_TYPE = 0x03, // Дескриптор строки
    USB_INTERFACE_DESCRIPTOR_TYPE = 0x04, // Дескриптор интерфейса
    USB_ENDPOINT_DESCRIPTOR_TYPE = 0x05, // Дескриптор конечной точки
    DEVICE_QUALIFIER = 0x06, // Уточняющий дескриптор устройства
    OTHER_SPEED_CONFIGURATION = 0x07, // Дескриптор дополнительной конфигурации
    INTERFACE_POWER = 0x08, // Дескриптор управления питанием интерфейса
    OTG = 0x09, // Дескриптор OTG
    DEBUG = 0x0A, // Отладочный дескриптор
    INTERFACE_ASSOCIATION = 0x0B, // Дополнительный дескриптор интерфейса	
}USB_DESCRIPTOR_TYPES;

typedef enum {
    SHORT_PACKET_TERMINATE = 0x01,
    PIPE_TRANSFER_TIMEOUT = 0x03,
} PIPE_POLICIES;

typedef struct {
    WINUSB_SETUP_PACKET winusb_packet;
    uint8_t data[CTRL_BUFF_LENGTH];
}wixusb_ctrl_packet_t;

typedef struct {
    USB_DESCRIPTOR_TYPES desc_type;
    uint8_t desc_idx;
    uint8_t data[DESC_BUFF_LENGTH];
}wixusb_get_desc_t;

typedef struct {
    PIPE_POLICIES policy_type;
    uint32_t policy_value;
}wixusb_set_pipe_policy_t;

typedef struct {
    uint16_t vid;
    uint16_t pid;
}wixusb_vid_pid_t;

typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;


#ifdef __cplusplus
}
#endif

#endif /* WIXUSB_DRIVER_TYPES_H */

