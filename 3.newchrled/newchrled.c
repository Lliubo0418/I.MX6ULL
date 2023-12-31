#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define NEWCHRLED_CNT   1        /*设备号个数*/
#define NEWCHRLED_NAME  "newchrled"       /*名字*/
#define LEDOFF          0
#define LEDON           1


/*寄存器物理地址*/
#define CCM_CCGR1_BASE    (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE (0X020E02F4)
#define GPIO1_DR_BASE (0X0209C000)
#define GPIO1_GDIR_BASE (0X0209C004)

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;


/*newchrled 设备结构体*/
struct newchrled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
};

struct newchrled_dev newchrled;

/*led打开/关闭*/
void led_switch(u8 sta){
    u32 val =0;
    if(sta==LEDON){
        val=readl(GPIO1_DR);
        val&=~(1<<3);
        writel(val,GPIO1_DR);
    }else if(sta==LEDOFF){
        val=readl(GPIO1_DR);
        val|=(1<<3);
        writel(val,GPIO1_DR);
    }
}

/*打开设备*/
static int led_open(struct inode *inode, struct file *file){
    file->private_data =&newchrled;               /*设置私有数据*/
    //printk("led open!\r\n");
    return 0;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos){

    return 0;
}
static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
    int ret=0;
    
    unsigned char databuf[1];
    unsigned char ledstat;
    ret=copy_from_user(databuf,buf,count);
    if(ret<0){
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat=databuf[0];

    if(ledstat==LEDON){
        led_switch(LEDON);
    }else if(ledstat==LEDOFF){
        led_switch(LEDOFF);
    }
    return 0;
}
/*关闭设备*/
static int led_release(struct inode *inode, struct file *file){
    printk("led release!\r\n");
    return 0;
}


/*设备操作函数结构体*/
static struct file_operations newchrled_fops={
    .owner=THIS_MODULE,
    .open=led_open,
    .read=led_read,
    .write=led_write,
    .release=led_release
};

/*驱动函数初始化*/
static int __init led_init(void){
    u32 val =0;

    /*初始化LED*/
    /*1、寄存器地址映射*/
    IMX6U_CCM_CCGR1=ioremap(CCM_CCGR1_BASE,4);
    SW_MUX_GPIO1_IO03=ioremap(SW_MUX_GPIO1_IO03_BASE,4);
    SW_PAD_GPIO1_IO03=ioremap(SW_PAD_GPIO1_IO03_BASE,4);
    GPIO1_DR=ioremap(GPIO1_DR_BASE,4);
    GPIO1_GDIR=ioremap(GPIO1_GDIR_BASE,4);

    /*使能GPIO1时钟*/
    val=readl(IMX6U_CCM_CCGR1);
    val&=~(3<<26);        /*清除之前的设置*/
    val|=(3<<26);         /*设置新值*/
    writel(val,IMX6U_CCM_CCGR1);

    /* 3、设置 GPIO1_IO03 的复用功能，将其复用为GPIO1_IO03，最后设置 IO 属性。*/
    writel(5,SW_MUX_GPIO1_IO03);

    /* 寄存器 SW_PAD_GPIO1_IO03 设置 IO 属性 */
    writel(0x10B0,SW_PAD_GPIO1_IO03);

    /* 4、设置 GPIO1_IO03 为输出功能 */
    val=readl(GPIO1_GDIR);
    val&=~(1<<3);
    val|=(1<<3);
    writel(val,GPIO1_GDIR);

    /*默认关闭LED灯*/
    val=readl(GPIO1_DR);
    val|=(1<<3);
    writel(val,GPIO1_DR);

    /*注册字符设备驱动*/
    /*1、创建设备号*/
    if(newchrled.major){                /*定义了设备号*/
        newchrled.devid=MKDEV(newchrled.major,0);
        register_chrdev_region(newchrled.devid,NEWCHRLED_CNT,NEWCHRLED_NAME);
    }else{                              /*没有定义设备号*/
        alloc_chrdev_region(&newchrled.devid,0,NEWCHRLED_CNT,NEWCHRLED_NAME);      /*申请设备号*/
        newchrled.major =MAJOR(newchrled.devid);                                   /*获取主设备号*/
        newchrled.minor =MINOR(newchrled.devid);                                   /*获取次设备号*/
    }
    printk("newchrled major=%d,minor=%d\r\n",newchrled.major,newchrled.minor);

    /*2、初始化cdev*/
    newchrled.cdev.owner=THIS_MODULE;
    cdev_init(&newchrled.cdev,&newchrled_fops);

    /*3、添加一个cdev*/
    cdev_add(&newchrled.cdev,newchrled.devid,NEWCHRLED_CNT);

    /*4、创建类*/
    newchrled.class=class_create(THIS_MODULE,NEWCHRLED_NAME);
    if(IS_ERR(newchrled.class)){
        return PTR_ERR(newchrled.class);
    }

    /*5、创建设备*/
    newchrled.device=device_create(newchrled.class,NULL,newchrled.devid,NULL,NEWCHRLED_NAME);
    if(IS_ERR(newchrled.device)){
        return PTR_ERR(newchrled.device);
    }

    return 0;
}
/*设备注销*/
static void __exit led_exit(void){
    /*取消映射*/
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    /*注销设备*/
    cdev_del(&newchrled.cdev);
    unregister_chrdev_region(newchrled.devid,NEWCHRLED_CNT);

    device_destroy(newchrled.class,newchrled.devid);
    class_destroy(newchrled.class);

    //unregister_chrdev(CHRDEVBASE_MAJOR,CHRDEVBASE_NAME);
    //printk("chrdevbase exit()\r\n");
}

module_init(led_init);
module_exit(led_exit);


MODULE_AUTHOR("liubo");
MODULE_LICENSE("GPL");







