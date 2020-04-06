#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>


/*  
 *  Prototypes
 */
static int myinit(void);
static void myexit(void);
static void cleanup(int device_created);

/* file operations */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define FAIL -1
#define DEVICE_NAME "chardev"
#define WRITE_BUF_SIZE 256

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */

static struct cdev mycdev;
static struct class *myclass = NULL;

static struct file_operations fops = {
	read: device_read,
	write: device_write,
	open: device_open,
	release: device_release
};

/*
 * This function is called when the module is loaded
 */
static int myinit(void)
{
    int device_created = 0;

    /* cat /proc/devices */
    if (alloc_chrdev_region(&Major, 0, 1, DEVICE_NAME "_proc") < 0)
        goto error;

    /* ls /sys/class */
    if ((myclass = class_create(THIS_MODULE, DEVICE_NAME "_sys")) == NULL)
        goto error;

    /* ls /dev/ */
    if (device_create(myclass, NULL, Major, NULL, DEVICE_NAME "_dev") == NULL)
        goto error;

    device_created = 1;

    cdev_init(&mycdev, &fops);
    if (cdev_add(&mycdev, Major, 1) == -1)
        goto error;
    return SUCCESS;
error:
    cleanup(device_created);
    return FAIL;
}


static void cleanup(int device_created)
{
    if (device_created) {
        device_destroy(myclass, Major);
        cdev_del(&mycdev);
    }
    if (myclass)
        class_destroy(myclass);
    if (Major != -1)
        unregister_chrdev_region(Major, 1);
}

/*
 * This function is called when the module is unloaded
 */
static void myexit(void)
{
    cleanup(1);
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
	return 0;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	printk(KERN_ALERT "Read not implemented yet.\n");
	return -EINVAL;
}

/*  
 * Called when a process writes to dev file: echo "hi" > /dev/hello 
 */
static ssize_t
device_write(struct file *filp, const char *input_buff, size_t input_len, loff_t * off)
{
	char buf[WRITE_BUF_SIZE];
	if (input_len > 255) {
		printk(KERN_ALERT "Write with input_len too big.\n");
		return -EINVAL;
	}
	if (copy_from_user(buf, input_buff, input_len) > 0) {
		printk(KERN_ALERT "Could not copy data from userspace.\n");
		return -EIO;
	}
	buf[input_len] = '\0';
	printk(KERN_ALERT "%s\n", buf);
	return input_len;
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");

