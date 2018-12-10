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

#ifndef WINUSB_WRAP_H
#define WINUSB_WRAP_H

#include <stdbool.h>
#include <stdint.h>
#include "wixusb_driver_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int Sleep(int time);

int WinUsb_Connect(void);

bool CheckConnected(unsigned long deviceFd);

int WinUsb_GetDescriptor(int fd, uint8_t DescriptorType, uint8_t Index,
        uint16_t LanguageID, uint8_t * Buffer,
        uint32_t BufferLength,
        uint32_t * LengthTransferred);

int WinUsb_SetPipePolicy(int fd, uint8_t PipeID, uint32_t PolicyType,
        uint32_t ValueLength, void * Value);

/* WinUsb_ReadPipe*/
int WixUsb_ReadBulk(int InterfaceHandle, void * Buffer,
        uint32_t BufferLength, uint32_t * LengthTransferred);


/* WinUsb_WritePipe*/
int WixUsb_WriteBulk(int InterfaceHandle, PUCHAR Buffer,
        ULONG BufferLength, PULONG LengthTransferred);

BOOL WinUsb_ControlTransfer(int InterfaceHandle,
        WINUSB_SETUP_PACKET SetupPacket, PUCHAR Buffer, ULONG BufferLength,
        PULONG LengthTransferred, LPOVERLAPPED Overlapped);

#ifdef __cplusplus
}
#endif

#endif /* WINUSB_WRAP_H */
