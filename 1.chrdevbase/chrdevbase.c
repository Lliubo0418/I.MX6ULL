#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>



#define CHRDEVBASE_MAJOR 200             /*主设备号*/
#define CHRDEVBASE_NAME  "chrdevbase"    /* 设备名 */


static char readbuf[100];
static char writebuf[100];
static char kerneldata[]={"kernel data!"};

/*打开设备*/
static int chrdevbase_open(struct inode *inode, struct file *file){
    printk("chrdevbase open!\r\n");
    return 0;
}

static ssize_t chrdevbase_read(struct file *file, char __user *buf, size_t count, loff_t *ppos){
    int ret=0;
    /*向用户空间发送数据*/
    memcpy(readbuf,kerneldata,sizeof(kerneldata));
    ret=copy_to_user(buf,readbuf,count);
    if(ret==0){
        printk("kernel senddata ok!\r\n");
    }else{
        printk("kernel senddata failed!\r\n");
    }
    return 0;
}
static ssize_t chrdevbase_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
    int ret=0;
    /*从用户空间读数据*/
    ret=copy_from_user(writebuf,buf,count);
    if(ret==0){
        printk("kernel receivedata:%s\r\n",writebuf);
    
    }else{
        printk("kernel receivedata failed!\r\n");
    }
    return 0;
}
/*关闭设备*/
static int chrdevbase_release(struct inode *inode, struct file *file){
    printk("chrdevbase release!\r\n");
    return 0;
}


/*设备操作函数结构体*/
static struct file_operations chrdevbase_fops={
    .owner=THIS_MODULE,
    .open=chrdevbase_open,
    .read=chrdevbase_read,
    .write=chrdevbase_write,
    .release=chrdevbase_release
};

/*驱动函数初始化*/
static int __init chrdevbase_init(void){
    int ret=0;
    /*注册设备字符驱动*/
    ret=register_chrdev(CHRDEVBASE_MAJOR,CHRDEVBASE_NAME,&chrdevbase_fops);
    if(ret<0){
        printk("chrdevbase driver register failed!\r\n");
    }   
    printk("chrdevbase init\r\n");
    
    return 0;
}
/*设备注销*/
static void __exit chrdevbase_exit(void){
    
    /*注销设备*/
    unregister_chrdev(CHRDEVBASE_MAJOR,CHRDEVBASE_NAME);
    printk("chrdevbase exit()\r\n");
}

module_init(chrdevbase_init);
module_exit(chrdevbase_exit);


MODULE_AUTHOR("liubo");
MODULE_LICENSE("GPL");
