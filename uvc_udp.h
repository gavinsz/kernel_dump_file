/*
 * udp.h
 *
 *  Created on: 2021Äê11ÔÂ9ÈÕ
 *      Author: gavin
 */

#ifndef FUNCTION_UVC_UDP_H_
#define FUNCTION_UVC_UDP_H_

#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#define UDP_DATA_LEN 1024

struct pkt {
        uint32_t len;
        uint32_t seq;
        unsigned char data[UDP_DATA_LEN];
};
//__attribute__((packed));

void create_upd_sock(void);
int dump_data(u8 *data, size_t len);
int udp_sent_data(u8 *message, size_t len);

int file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size);
struct file* file_open(const char* path, int flags, int rights);
int write_file_init(void);
void start_write_thread(void *arg);

#endif /* FUNCTION_UVC_UDP_H_ */
