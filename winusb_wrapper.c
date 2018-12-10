/*
 * The MIT License
 *
 * Copyright 2017 Ildar Sadykov <irsdkv@gmail.com>.
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

#include "winusb_wrapper.h"
#include "errno.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#define WINUSB_FAIL         (FALSE)
#define WINUSB_SUCCESS      (TRUE)

extern int errno;

static int data_send_request(int fd, void * buff, int len) {
    int result = (int) write(fd, buff, len);
    if (result < 0)
        result = -errno;
    return result;
}

static int data_send(int fd, void *outBuff, int outLen) {
    return data_send_request(fd, outBuff, outLen);
}

static int data_send_cmd(int fd, void *outBuff, int outLen) {
    return data_send_request(fd, outBuff, outLen);
}

static int data_receive(int fd, char *buff, int buffLen) {
    int result = (int) read(fd, buff, buffLen);
    if (result < 0)
        result = -errno;
    return result;
}


int WinUsb_Connect(void) {
    int fd = -1;
    char namebuff[50];
    for (int i = 0; i < 10 && fd < 0; i++) {
        snprintf(namebuff, 50, WIXUSB_DEV_NAME "%d\0", i);
        fd = open(namebuff, O_RDWR);
        if (fd >= 0) {
            return fd;
        }
    }
    return fd;
}

bool CheckConnected(unsigned long deviceFd) {
    int result;

    result = ioctl(deviceFd, IOCTL_IS_CONNECTED);

    if (result < 0)
        return false;
    return true;
}

int Sleep(int time) {
    usleep(time * 1000);
}

int WinUsb_GetDescriptor(int fd, uint8_t DescriptorType, uint8_t Index,
        uint16_t LanguageID, uint8_t * Buffer,
        uint32_t BufferLength,
        uint32_t * LengthTransferred) {
    int result = 0;
    wixusb_get_desc_t desc_packet = {
        .desc_type = (USB_DESCRIPTOR_TYPES)DescriptorType,
        .desc_idx = Index,
    };

    result = ioctl(fd, IOCTL_GET_DESC, &desc_packet);

    if (result < 0) {
        return WINUSB_FAIL;
    }

    memcpy(Buffer, desc_packet.data, (result < BufferLength ? result : BufferLength));

    if (LengthTransferred != NULL)
        *LengthTransferred = result;

    return WINUSB_SUCCESS;
}

int WinUsb_SetPipePolicy(int fd, uint8_t PipeID, uint32_t PolicyType,
        uint32_t ValueLength, void * Value) {
    int result = 0;

    wixusb_set_pipe_policy_t pipe_policy;

    switch (PolicyType) {
        case SHORT_PACKET_TERMINATE:
        case PIPE_TRANSFER_TIMEOUT:
            pipe_policy.policy_type = (PIPE_POLICIES)PolicyType;
            pipe_policy.policy_value = *((uint32_t*) Value);
            break;
        default:
            return WINUSB_FAIL;
    }

    result = ioctl(fd, IOCTL_SET_PIPE_POL, &pipe_policy);

    if (result < 0)
        return WINUSB_FAIL;

    return WINUSB_SUCCESS;
}


int WixUsb_ReadBulk(int InterfaceHandle, void * Buffer,
        uint32_t BufferLength, uint32_t * LengthTransferred) {
    int result = 0;

    if (BufferLength > BULK_BUFF_LENGTH)
        return -1;

    result = data_receive(InterfaceHandle, (char*)Buffer, BufferLength);

    if (result < 0)
        return FALSE;

    if (LengthTransferred != NULL)
        *LengthTransferred = result;

    return TRUE;
}

int WixUsb_BulkTransfer(int InterfaceHandle, PUCHAR Buffer,
        ULONG BufferLength, PULONG LengthTransferred) {
    int result = 0;

    if (BufferLength > BULK_BUFF_LENGTH)
        return -1;

    result = data_send(InterfaceHandle, Buffer, BufferLength);

    if (result < 0)
        return FALSE;

    if (LengthTransferred != NULL)
        *LengthTransferred = result;

    return TRUE;
}


BOOL WinUsb_ControlTransfer(int InterfaceHandle,
        WINUSB_SETUP_PACKET SetupPacket, PUCHAR Buffer, ULONG BufferLength,
        PULONG LengthTransferred, LPOVERLAPPED Overlapped) {
    int result = 0;

    wixusb_ctrl_packet_t ctrl_packet = {
        .winusb_packet = SetupPacket,
    };

    if (BufferLength > CTRL_BUFF_LENGTH)
        return FALSE;


    if (SETUP_PACKET_IS_INPUT(SetupPacket.RequestType)) {
        result = ioctl(InterfaceHandle, IOCTL_RECV_CTRL, &ctrl_packet);
        if (result < 0)
            return result;
        memcpy(Buffer, ctrl_packet.data, result);
    } else {
        memcpy(ctrl_packet.data, Buffer, BufferLength);
        result = ioctl(InterfaceHandle, IOCTL_SEND_CTRL, &ctrl_packet);
        if (result < 0)
            return result;
    }

    if (LengthTransferred != NULL)
        *LengthTransferred = result;

    return TRUE;
}
