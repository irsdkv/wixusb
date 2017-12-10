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

#ifndef WIXUSB_IOCTL_H
#define WIXUSB_IOCTL_H

#include "wixusb_driver_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIXUSB_IOC_MAGIC      ('M' + 0)

typedef struct {
    char length;
    char data[EP_SIZE];
}wixusb_intrpt_packet;

#define IOCTL_SEND_CTRL            _IOW( WIXUSB_IOC_MAGIC, 0, wixusb_ctrl_packet_t )
#define IOCTL_RECV_CTRL            _IOWR( WIXUSB_IOC_MAGIC, 1, wixusb_ctrl_packet_t )
#define IOCTL_GET_DESC             _IOWR( WIXUSB_IOC_MAGIC, 2, wixusb_get_desc_t )
#define IOCTL_SET_PIPE_POL         _IOW( WIXUSB_IOC_MAGIC, 3, wixusb_set_pipe_policy_t )
#define IOCTL_GET_VID_PID          _IOR( WIXUSB_IOC_MAGIC, 5, wixusb_vid_pid_t )
#define IOCTL_IS_CONNECTED         _IO( WIXUSB_IOC_MAGIC, 6)
#define IOCTL_WRITE_INT            _IOW( WIXUSB_IOC_MAGIC, 7, wixusb_intrpt_packet )


#ifdef __cplusplus
}
#endif

#endif /* WIXUSB_IOCTL_H */

