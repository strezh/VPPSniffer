//Based on uss720.c

#include <linux/module.h>
#include <linux/socket.h>
#include <linux/parport.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/kref.h>

#define DRIVER_VERSION "0.0"
#define DRIVER_AUTHOR "A"
#define DRIVER_DESC "B"

#undef dbg
#define dbg printk

static unsigned char control_status;//orginal - reg[1]
static unsigned char unknown_status;//orginal - reg[2]
#define CN 1

static int change_mode(struct parport *pp, int m){
    printk("change_mode\n");
    return 0;
}

static int clear_epp_timeout(struct parport *pp){
    printk("clear_epp_timeout\n");
    return 0;
}

static void parport_uss720_write_data(struct parport *pp, unsigned char d){
    printk("parport_uss720_write_data\n");
}

static unsigned char parport_uss720_read_data(struct parport *pp){
    printk("parport_uss720_read_data\n");
    return 0;
}

static void parport_uss720_write_control(struct parport *pp, unsigned char d){
    printk("parport_uss720_write_control\n");
    d = (d & 0x0F) | (control_status & 0xF0);
    if(CN) return;
    control_status = d;
}

static unsigned char parport_uss720_read_control(struct parport *pp){
    printk("parport_uss720_read_control\n");
    return control_status & 0x0F;
}

static unsigned char parport_uss720_frob_control(struct parport *pp, unsigned char mask, unsigned char val){
    unsigned char d;
    printk("parport_uss720_frob_control\n");
    mask &= 0x0F;
    val &= 0x0F;
    d = (control_status & (~mask)) ^ val;
    if(CN) return 0;
    control_status = d;
    return d & 0x0F;
}

static unsigned char parport_uss720_read_status(struct parport *pp){
    printk("parport_uss720_read_status\n");
    return PARPORT_STATUS_ERROR;
}

static void parport_uss720_disable_irq(struct parport *pp){
    printk("parport_uss720_disable_irq\n");
}

static void parport_uss720_enable_irq(struct parport *pp){
    printk("parport_uss720_enable_irq\n");
}

static void parport_uss720_data_forward(struct parport *pp){
    printk("parport_uss720_data_forward\n");
    unsigned char d;
    d = control_status & ~0x20;
    if(CN) return;
    control_status = d;
}
static void parport_uss720_data_reverse(struct parport *pp){
    printk("parport_uss720_data_reverse\n");
    unsigned char d;
    d = control_status | 0x20;
    if(CN) return;
    control_status = d;
}

static void parport_uss720_init_state(struct pardevice *dev, struct parport_state *s){
    printk("parport_uss720_init_state: %d\n", dev->irq_func);
    s->u.pc.ctr = 0x0C | (dev->irq_func ? 0x10 : 0x00);
    s->u.pc.ecr = 0x24;
}

static void parport_uss720_save_state(struct parport *pp, struct parport_state *s){
    printk("parport_uss720_save_state\n");
    s->u.pc.ctr = control_status;
    s->u.pc.ecr = unknown_status;
}

static void parport_uss720_restore_state(struct parport *pp, struct parport_state *s){
    printk("parport_uss720_restore_state\n");
    control_status = s->u.pc.ctr;
    unknown_status = s->u.pc.ecr;
}
/*########################################################################################################*/
static size_t parport_uss720_epp_read_data(struct parport *pp, void *buf, size_t length, int flags){
    printk("parport_uss720_epp_read_data\n");
}

static size_t parport_uss720_epp_write_data(struct parport *pp, const void *buf, size_t length, int flags){
    printk("size_t parport_uss720_epp_write_data\n");
}

static size_t parport_uss720_epp_read_addr(struct parport *pp, void *buf, size_t length, int flags){
    printk("parport_uss720_epp_read_addr\n");
}

static size_t parport_uss720_epp_write_addr(struct parport *pp, const void *buf, size_t length, int flags){
    printk("parport_uss720_epp_write_addr\n");
}

static size_t parport_uss720_ecp_write_data(struct parport *pp, const void *buffer, size_t len, int flags){
    printk("parport_uss720_ecp_write_data\n");
}

static size_t parport_uss720_ecp_read_data(struct parport *pp, void *buffer, size_t len, int flags){
    printk("parport_uss720_ecp_read_data\n");
}

static size_t parport_uss720_ecp_write_addr(struct parport *pp, const void *buffer, size_t len, int flags){
    printk("parport_uss720_ecp_write_addr\n");
}

static size_t parport_uss720_write_compat(struct parport *pp, const void *buffer, size_t len, int flags){
    printk("parport_uss720_write_compat\n");
}
/*########################################################################################################*/


static struct parport_operations parport_uss720_ops = 
{
    .owner =        THIS_MODULE,
    .write_data =        parport_uss720_write_data,
    .read_data =        parport_uss720_read_data,

    .write_control =    parport_uss720_write_control,
    .read_control =        parport_uss720_read_control,
    .frob_control =        parport_uss720_frob_control,

    .read_status =        parport_uss720_read_status,

    .enable_irq =        parport_uss720_enable_irq,
    .disable_irq =        parport_uss720_disable_irq,

    .data_forward =        parport_uss720_data_forward,
    .data_reverse =        parport_uss720_data_reverse,

    .init_state =        parport_uss720_init_state,
    .save_state =        parport_uss720_save_state,
    .restore_state =    parport_uss720_restore_state,

    .epp_write_data =    parport_uss720_epp_write_data,
    .epp_read_data =    parport_uss720_epp_read_data,
    .epp_write_addr =    parport_uss720_epp_write_addr,
    .epp_read_addr =    parport_uss720_epp_read_addr,

    .ecp_write_data =    parport_uss720_ecp_write_data,
    .ecp_read_data =    parport_uss720_ecp_read_data,
    .ecp_write_addr =    parport_uss720_ecp_write_addr,

    .compat_write_data =    parport_uss720_write_compat,
    .nibble_read_data =    parport_ieee1284_read_nibble,
    .byte_read_data =    parport_ieee1284_read_byte,
};

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

static struct parport * pp = NULL;

static int __init uss720_init(void){
    pp = parport_register_port(0, PARPORT_IRQ_NONE, PARPORT_DMA_NONE, &parport_uss720_ops);
    printk("\n\nparport_register_port: %u\n", pp);
    if(!pp) return 0;
    pp->private_data = NULL;
    pp->modes = PARPORT_MODE_PCSPP | PARPORT_MODE_TRISTATE | PARPORT_MODE_EPP | PARPORT_MODE_ECP | PARPORT_MODE_COMPAT;
    parport_announce_port(pp);
    printk("uss720_init\n");
    return 0;
}

static void __exit uss720_cleanup(void){
    printk("uss720_cleanup\n");
    if(pp){
        parport_remove_port(pp);
        parport_put_port(pp);
    }
    printk("finish!\n");
}

module_init(uss720_init);
module_exit(uss720_cleanup);

/* --------------------------------------------------------------------- */
