/*
 * Parallel port sniffer kernel module
 *
 * Copyright (c) 2012, Fernando Gabriel Vicente (www.alfersoft.com.ar - fvicente@gmail.com)
 * All rights reserved.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Thanks to Rodrigo Zechin Rosauro!!!
 */

#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/parport.h>
#include <linux/ppdev.h>

//#include <linux/smp_lock.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>

#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>	/* Necessary because we use proc fs */
#include <linux/seq_file.h>	/* for seq_file */
#include <linux/file.h>
#include <asm/uaccess.h>	/* for get_user and put_user */

#define SUCCESS 0
#define BUF_LEN 80


#define CHRDEV      "parportsnif"
#define VPP_MAJOR   367
#define MAX_DEVICES 10
#define MAX_LINES   10000
#define PORT_NUM    0

#define SAMPLE_RATE     1000000
#define SAMPLE_RATE_STR "1000000"

DECLARE_WAIT_QUEUE_HEAD(readwait);

struct vpp_st {
    struct file *pfd;
    unsigned int port;
    int bufsz;
    struct timespec start;
    unsigned char chanvalues[3];
    unsigned char lastvalues[3];
};

struct vlogline_st {
    struct vlogline_st *next;
    char *data;
    int size;
    int offset;
};

struct vlog_st {
    struct vlogline_st *lines;
    int totlines;
} vlog;

static struct class *vpp_class;

static ssize_t log_write(const char *buffer, size_t length)
{
    struct vlogline_st  *line, *newline;

	//lock_kernel();
    line = vlog.lines;
    while(line && line->next)
        line = line->next;
    newline = (struct vlogline_st *)kzalloc(sizeof(struct vlogline_st), GFP_KERNEL);
    if (!newline) {
    	//unlock_kernel();
        return -ENOMEM;
    }
    newline->data = (char *)kzalloc(length+1, GFP_KERNEL);
    if (!newline->data) {
    	//unlock_kernel();
        kfree(newline);
        return -ENOMEM;
    }
    newline->size = length;
    memcpy(newline->data, buffer, length);
    if(line)
        line->next = newline;
    else
        vlog.lines = newline;
    vlog.totlines++;
    while (vlog.totlines > MAX_LINES && vlog.lines) {
        line = vlog.lines;
        vlog.lines = line->next;
        kfree(line->data);
        kfree(line);
        vlog.totlines--;
    }
	//unlock_kernel();
    wake_up_interruptible(&readwait);
    return length;
}

static void vpp_log_ols(struct vpp_st *vpp)
{
    char buf[900];
    int sz;
    struct timespec ts;
    long long udelta;
    uint32_t samplenum;

    if (memcmp(vpp->lastvalues, vpp->chanvalues, sizeof(vpp->lastvalues)) == 0) {
        // values did not change - nothing to do
        return;
    }

    memset(&ts, 0, sizeof(ts));
    getnstimeofday(&ts);
    
    // microseconds since "start"
    udelta = ((ts.tv_sec - vpp->start.tv_sec) * (long long)1000000);
    udelta += ((ts.tv_nsec - vpp->start.tv_nsec) / 1000);
    // number of the sample
    // /* todo - find a better way */ samplenum = do_div(udelta * 1000000, SAMPLE_RATE);
    samplenum = udelta;
    //0000000f@0
    sz = snprintf(buf, sizeof(buf), "0000%.2X%.2X%.2X@%u\n", vpp->chanvalues[2], vpp->chanvalues[1], vpp->chanvalues[0], samplenum);
    log_write(buf, sz);
    memcpy(vpp->lastvalues, vpp->chanvalues, sizeof(vpp->lastvalues));
}

static void vpp_log(struct vpp_st *vpp, const char *fmt, ...)
{
    char buf[900];
    va_list args;
    int sz;
    struct timespec ts;

    memset(&ts, 0, sizeof(ts));
    getnstimeofday(&ts);

    // current time
    sz = snprintf(buf, sizeof(buf), "# [%u.%u] ", (unsigned int)ts.tv_sec, (unsigned int)ts.tv_nsec);
    
    // log message
    va_start(args, fmt);
    sz += vsnprintf(buf+sz, sizeof(buf)-sz, fmt, args);
    va_end(args);
    buf[sz++] = '\n';
    buf[sz] = '\0';
    log_write(buf, sz);
}

/* 
 * This is called whenever a process attempts to open the device file 
 */
static int device_open(struct inode *inode, struct file *file)
{
    struct vpp_st *vpp = NULL;
    char port[64];
    int err = 0;

	vpp = kzalloc (sizeof(struct vpp_st), GFP_KERNEL);
	if (!vpp) {
		return -ENOMEM;
    }
    vpp->bufsz = 512;
    vpp->port = iminor(inode);
    snprintf(port, sizeof(port)-1, "/dev/parport%d", vpp->port);
    /* open the real parallel port */
    vpp->pfd = filp_open(port, O_RDWR, 0);
    if (IS_ERR(vpp->pfd)) {
        err = PTR_ERR(vpp->pfd);
	    printk(KERN_ALERT "parportsnif: failed to open %s", port);
        return err;
    }
    file->private_data = vpp;
    /* write the OLS log header */
    log_write(";Rate: "SAMPLE_RATE_STR"\n", strlen(";Rate: "SAMPLE_RATE_STR"\n"));
    log_write(";Channels: 32\n", strlen(";Channels: 32\n"));
    /* get the starting timestamp */
    memset(&vpp->start, 0, sizeof(vpp->start));
    getnstimeofday(&vpp->start);
    /* log the initial reading */
    vpp_log_ols(vpp);
	try_module_get(THIS_MODULE);
    printk(KERN_INFO "parportsnif: successfully opened %s\n", port);
    //vpp_log(vpp, "%d %s", vpp->port, "OPEN");
	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    struct vpp_st *vpp = (struct vpp_st *) file->private_data;

    if (vpp->pfd) {
	    filp_close(vpp->pfd, NULL);
    }
    vpp->pfd = NULL;
	module_put(THIS_MODULE);
    printk(KERN_INFO "parportsnif: successfully closed /dev/parport%d\n", vpp->port);
    vpp_log(vpp, "%d %s", vpp->port, "CLOSE");
    kfree(vpp);
	return SUCCESS;
}

loff_t device_llseek(struct file *file, loff_t offset, int p)
{
    struct vpp_st *vpp = (struct vpp_st *) file->private_data;
    vpp_log(vpp, "%d %s", vpp->port, "SEEK");
    return(vpp->pfd->f_op->llseek(vpp->pfd, offset, p));
}

unsigned int device_poll(struct file *file, struct poll_table_struct *ps)
{
    struct vpp_st *vpp = (struct vpp_st *) file->private_data;
    vpp_log(vpp, "%d %s", vpp->port, "POLL");
    return(vpp->pfd->f_op->poll(vpp->pfd, ps));
}

/* 
 * This function is called whenever a process which has already opened the
 * device file attempts to read from it.
 */
static ssize_t device_read(struct file *file, char __user * buffer, size_t length, loff_t * offset)
{
    struct vpp_st *vpp = (struct vpp_st *) file->private_data;
    int i;
    char *buf;
    unsigned char ch;
    ssize_t bytes_read;

    printk(KERN_ALERT "%s\n", "parportsnif: starting read");
    buf = (char *)kmalloc((length*3)+1, GFP_KERNEL);
    if (!buf) {
        printk(KERN_ALERT "%s\n", "parportsnif: Failed to allocate read buffer");
        return 0;
    }
    *buf = '\0';
    bytes_read = vpp->pfd->f_op->read(vpp->pfd, buffer, length, offset);
    for (i = 0; i < bytes_read; i++, buffer++) {
        get_user(ch, buffer);
        snprintf(&buf[i*3], ((length-i)*3)+1, " %.2X", (unsigned int)ch);
    }
    vpp_log(vpp, "%d READ %d %s", vpp->port, bytes_read, buf);
    kfree(buf);
    return(bytes_read);
}

/* 
 * This function is called when somebody tries to
 * write into our device file. 
 */
static ssize_t
device_write(struct file *file,
	     const char __user * buffer, size_t length, loff_t * offset)
{
    struct vpp_st *vpp = (struct vpp_st *) file->private_data;
    int i;
    char *buf;
    unsigned char ch;
    ssize_t bytes_written;

    printk(KERN_ALERT "%s\n", "parportsnif: starting write");
    buf = (char *)kmalloc((length*3)+1, GFP_KERNEL);
    if (!buf) {
        printk(KERN_ALERT "%s\n", "parportsnif: Failed to allocate write buffer");
        return 0;
    }
    *buf = '\0';
    bytes_written = vpp->pfd->f_op->write(vpp->pfd, buffer, length, offset);
    for (i = 0; i < bytes_written; i++, buffer++) {
        get_user(ch, buffer);
        snprintf(&buf[i*3], ((length-i)*3)+1, " %.2X", (unsigned int)ch);
    }
    vpp_log(vpp, "%d WRITE %d %s", vpp->port, bytes_written, buf);
    kfree(buf);
	return(bytes_written);
}

#define LOGKFOP(el)     printk(KERN_INFO "vpp->pfd->f_op->%s = %p\n", #el, vpp->pfd->f_op->el)
#define CASE(a) case a: snprintf(buf, sizeof(buf)-1, "%s", #a)
/* 
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */
long device_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct vpp_st *vpp = (struct vpp_st *) file->private_data;
	int has_data=0;
	unsigned char ch;
    char buf[64];
    long ret;

	/* 
	 * Switch according to the ioctl called 
	 */
    switch(cmd) {
    CASE(PPSETMODE);
        break;
    CASE(PPRSTATUS);
        break;
    CASE(PPRCONTROL);
        break;
    CASE(PPWCONTROL);
        get_user(ch, (char *)arg);
        has_data = 1;
        vpp->chanvalues[2] = ch;
        break;
    CASE(PPFCONTROL);
        break;
    CASE(PPRDATA);
        break;
    CASE(PPWDATA);
        get_user(ch, (char *)arg);
        has_data = 1;
        vpp->chanvalues[0] = ch;
        break;
    CASE(PPCLAIM);
        break;
    CASE(PPRELEASE);
        break;
    CASE(PPYIELD);
        break;
    CASE(PPEXCL);
        break;
    CASE(PPDATADIR);
        break;
    CASE(PPNEGOT);
        break;
    CASE(PPWCTLONIRQ);
        break;
    CASE(PPCLRIRQ);
        break;
    CASE(PPSETPHASE);
        break;
    CASE(PPGETTIME);
        break;
    CASE(PPSETTIME);
        break;
    CASE(PPGETMODES);
        break;
    CASE(PPGETMODE);
        break;
    CASE(PPGETPHASE);
        break;
    CASE(PPGETFLAGS);
        break;
    CASE(PPSETFLAGS);
        break;
    CASE(PP_FASTWRITE);
        break;
    CASE(PP_FASTREAD);
        break;
    CASE(PP_W91284PIC);
        break;
    default:
        snprintf(buf, sizeof(buf)-1, "UNKNOWN %u", cmd);
        break;
    }
    ret = vpp->pfd->f_op->unlocked_ioctl(vpp->pfd, cmd, arg);
    if(cmd == PPRSTATUS) {
        get_user(ch, (char *)arg);
        has_data = 1;
        vpp->chanvalues[1] = ch;
    }
    if(has_data) {
        vpp_log_ols(vpp);
    }
    //if(has_data) {
    //    vpp_log(vpp, "%d IOCTL %s %.2X", vpp->port, buf, (unsigned int)ch);
    //} else {
    //    vpp_log(vpp, "%d IOCTL %s", vpp->port, buf);
    //}
    return(ret);
}

/* log file operations */

/**
 * This function is called when the /proc file is open.
 *
 */
static int log_open(struct inode *inode, struct file *file)
{
	return SUCCESS;
}

static int log_release(struct inode *inode, struct file *file)
{
	return SUCCESS;
}

static ssize_t log_read(struct file *file,
			   char __user * buffer, size_t length, loff_t * offset)
{
    struct vlogline_st  *line = NULL;
    size_t              sz;

    if (!buffer || length <= 0)
        return -EINVAL;

    if (!vlog.lines) {
        if (file->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
        wait_event_interruptible(readwait, vlog.lines != NULL);
    }
	//lock_kernel();
    line = vlog.lines;
    if (!line) {
        //unlock_kernel();
        return 0;
    }
    sz = ((line->size-line->offset) > length) ? length : (line->size-line->offset);
    copy_to_user(buffer, line->data+line->offset, sz);
    line->offset += sz;
    if (line->offset >= line->size) {
        vlog.lines = line->next;
        kfree(line->data);
        kfree(line);
        vlog.totlines--;
    }
	//unlock_kernel();
    return sz;
}

static unsigned int log_poll(struct file *file, poll_table * wait)
{
    struct vlog_st      *vlog = (struct vlog_st *) file->private_data;
    unsigned int mask;

    poll_wait(file, &readwait, wait);
    mask = 0;
    if (vlog->lines != NULL)
        mask |= POLLIN | POLLRDNORM;
    return mask;
}


/* Module Declarations */

/* 
 * This structure will hold the functions to be called
 * when a process does something to the device we
 * created. Since a pointer to this structure is kept in
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions. 
 */
struct file_operations vpp_fops = {
    .owner = THIS_MODULE,
    .llseek = device_llseek,
	.read = device_read,
	.write = device_write,
    .poll = device_poll,
	.unlocked_ioctl = device_unlocked_ioctl,
	.open = device_open,
	.release = device_release,	/* a.k.a. close */
};

struct file_operations vlog_fops = {
	.owner   = THIS_MODULE,
	.open    = log_open,
	.read    = log_read,
    .poll    = log_poll,
	.release = log_release
};

static void vpp_attach(struct parport *port)
{
    struct device *device;
	int err = 0;

	device = device_create(vpp_class, port->dev, MKDEV(VPP_MAJOR, port->number),
		      NULL, "parportsnif%d", port->number);
    if (IS_ERR(device)) {
        err = PTR_ERR(device);
        printk(KERN_WARNING CHRDEV "Error %d while trying to create %s%d", err, CHRDEV, port->number);
    }
}

static void vpp_detach(struct parport *port)
{
	device_destroy(vpp_class, MKDEV(VPP_MAJOR, port->number));
}

static struct parport_driver vpp_driver = {
	.name		= CHRDEV,
	.attach		= vpp_attach,
	.detach		= vpp_detach,
};

static int __init vpp_init (void)
{
	int err = 0;
	struct proc_dir_entry *entry;

    memset(&vlog, 0, sizeof(vlog));

	if (register_chrdev (VPP_MAJOR, CHRDEV, &vpp_fops)) {
		printk (KERN_WARNING CHRDEV ": unable to get major %d\n",
			VPP_MAJOR);
		return -EIO;
	}
	vpp_class = class_create(THIS_MODULE, CHRDEV);
	if (IS_ERR(vpp_class)) {
		printk (KERN_WARNING CHRDEV ": error creating class parportsnif\n");
		err = PTR_ERR(vpp_class);
		goto out_chrdev;
	}
	if (parport_register_driver(&vpp_driver)) {
		printk (KERN_WARNING CHRDEV ": unable to register with parport\n");
		goto out_class;
	}

    /* register log file in /proc */
	//entry = create_proc_entry("parportlog", 0, NULL);
	entry = proc_create("parportlog", 0444, NULL, &vlog_fops);
	//if (entry) {
	//	entry->proc_fops = &vlog_fops;
	//}
	if (!entry) {
		err = PTR_ERR(entry);
        goto out_class;
    }

	goto out;

out_class:
	class_destroy(vpp_class);
out_chrdev:
	unregister_chrdev(VPP_MAJOR, CHRDEV);
out:
	return err;
}

static void __exit vpp_cleanup (void)
{
	/* Clean up all parport stuff */
    remove_proc_entry("parportlog", NULL);
	parport_unregister_driver(&vpp_driver);
	class_destroy(vpp_class);
	unregister_chrdev (VPP_MAJOR, CHRDEV);
}

module_init(vpp_init);
module_exit(vpp_cleanup);

MODULE_LICENSE("GPL");
MODULE_ALIAS_CHARDEV_MAJOR(VPP_MAJOR);

