/*
 * udp.c
 *
 *  Created on: 2021Äê11ÔÂ9ÈÕ
 *      Author: gavin
 */

#include <linux/types.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/ptrace.h>
#include "queue.h"
#include "uvc_udp.h"

struct list_head time_list_head;

static struct socket *sock;
static struct msghdr msg;
static struct iovec iov;

static struct pkt *udp_pkt = NULL;
static struct file *fp = NULL;
struct queue v_queue;

/*
typedef struct time_node {
        int diff;
        struct list_head list;
}time_node_t;

u64 get_systime_us(void)
{
    struct timeval tv;
    do_gettimeofday(&tv);
    return ((u64)tv.tv_sec * 1000000) + tv.tv_usec;
}

static void dump_time_list(void)
{
    struct list_head *p;
    time_node_t *time_node, *cur, *q;
    list_for_each(p, &time_list_head){
        time_node = list_entry(p, time_node_t, list);
        printk("interval=%d", time_node->diff);
    }

    list_for_each_entry_safe(cur, q , &time_list_head,list)
    {
        list_del(&(cur->list));
        kfree(cur);
    }
}
*/

void create_upd_sock(void)
{
//#define INADDR_SEND INADDR_LOOPBACK   /* 127.0.0.1 */
//#define INADDR_SEND 0xac11b2be      /* 172.17.178.190 */
//#define INADDR_SEND 0xac11b299      /*172.17.178.153*/
#define INADDR_SEND 0xc0a80f57        /*192.168.15.87*/

#define MESSAGE_SIZE 1024

    static bool sock_init;
    static struct sockaddr_in sin;

    int error;

    if (sock_init == false)
    {
        /* Creating socket */
        error = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
        if (error < 0 ){
            printk(KERN_ERR "create socket failed, error %d\n",error);
            sock = NULL;
            return;
        }
        /* Connecting the socket */
        sin.sin_family = AF_INET;
        sin.sin_port = htons(1764);
        sin.sin_addr.s_addr = htonl(INADDR_SEND);
        error = sock->ops->connect(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr), 0);
        if (error<0){
            printk(KERN_ERR "connect socket failed, error %d\n",error);
            return;
        }

        /* Preparing message header */
        msg.msg_flags = 0;
        msg.msg_name = &sin;
        msg.msg_namelen  = sizeof(struct sockaddr_in);
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        //msg.msg_iter.iov = &iov;
        msg.msg_control = NULL;
        sock_init = true;
    }
}

int udp_sent_data(u8 *message, size_t len)
{
    mm_segment_t old_fs;
    int error = 0;

    iov.iov_base = message;
    iov.iov_len = len;
    iov_iter_init(&msg.msg_iter, WRITE, &iov, 1, len);
    if (NULL == sock)
        return 0;

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    error = sock_sendmsg(sock, &msg);
    set_fs(old_fs);

    return error;
}

int udp_sent_pkt(u8 *data, size_t len)
{
    static unsigned int seq = 0;
    unsigned int pos = 0;
    size_t data_len = 0;
    struct element *e;

    //dump_stack();
    if (NULL == udp_pkt) {
        udp_pkt = kmalloc(sizeof(struct pkt), GFP_KERNEL);
        if (NULL == udp_pkt) {
            printk(KERN_ERR "kmalloc for snd_buff failed\n");
            return -2;
        }
    }

    /*
    udp_pkt->len = htonl(len);
    udp_pkt->seq = htonl(seq);
    memcpy(udp_pkt->data, data, len);
    udp_sent_data((u8*)udp_pkt, sizeof(struct pkt) - UDP_DATA_LEN + len);
    seq++;
    */

    while (len > 0) {
        if (len <= UDP_DATA_LEN) {
            data_len = len;
        } else {
            data_len = UDP_DATA_LEN;
        }
        udp_pkt->len = htonl(data_len);
        udp_pkt->seq = htonl(seq);
        memcpy(udp_pkt->data, data + pos, data_len);

        e = kmalloc(sizeof(struct element), GFP_KERNEL);
        if (NULL == e) {
            printk(KERN_ERR "kmalloc for element failed\n");
            return 0;
        }
        memcpy(e->pk.data, data + pos, data_len);
        e->pk.len = data_len;

        //printk(KERN_ERR "enqueue queue=%p|e=%p\n", &v_queue, e);
        enqueue(e, &v_queue);

        pos += data_len;
        len -= data_len;
        //udp_sent_data((u8*)udp_pkt, sizeof(struct pkt) - UDP_DATA_LEN + data_len);
        seq++;

    }

    //printk(KERN_ERR "pos=%u\n", pos);
    return 0;
}

struct file* file_open(const char* path, int flags, int rights)
{
    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

int file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    //set_fs(get_ds());
    set_fs(KERNEL_DS);

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}


static int write_thread(void *arg)
{
    static unsigned long long offset = 0;
    struct element *e = NULL;
    struct queue *q = (struct queue*)arg;

    printk(KERN_ERR "write thread started\n");

    while (1) {
        e = dequeue(q);
        if (NULL != e) {
            //printk(KERN_ERR "dequeue queue=%p|e=%p\n", q, e);
            file_write(fp, offset, e->pk.data, e->pk.len);
            offset += (unsigned long long)e->pk.len;
            kfree(e);
        } else {
            usleep_range(5*1000, 5*1000+10); //sleep 5ms
        }
    }

    return 0;
}

void start_write_thread(void *arg)
{
    kthread_run(write_thread, arg, "write_thread");
}

int write_file_init(void)
{
    fp = filp_open("/data/local/tmp/uvc_buffer_prepare.h264", O_RDWR|O_CREAT, 0644);
    if (NULL == fp) {
        printk(KERN_ERR "filp_open /data/kernel.h264 failed\n");
        return -1;
    }

    queue_create(&v_queue);
    start_write_thread((void*)&v_queue);

    return 0;
}

