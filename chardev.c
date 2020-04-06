#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>


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
static int is_open;             /* A flag that specifies if the device is open somewhere */
static size_t operation_count;

static struct cdev mycdev;
static struct class *myclass = NULL;

static struct file_operations fops = {
	owner: THIS_MODULE,
	read: device_read,
	write: device_write,
	open: device_open,
	release: device_release
};

/*
 * We use private data in a file to store the data on how much of
 * string to be output was read. I think seq_file has something about this,
 * but the rest of the behavior is not very proc-like.
 * Not actually necessary because in out case only one instance can be open,
 * but in some cases may help in the future.
 */
struct private_data {
	char read_string[32];
	size_t offset;
};

/*
 * This function is called when the module is loaded
 */
static int myinit(void)
{
    int device_created = 0;

    is_open = 0;

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
	if (is_open) {
		return -EBUSY;
	}

	file->private_data = kzalloc(sizeof(struct private_data), GFP_KERNEL);

	is_open = 1;
	return 0;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	if (!is_open) {
		printk(KERN_ALERT "Releasing a closed device.\n");
		return -EIO;
	}
	is_open = 0;
	kfree(file->private_data);
	return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t buffer_length,/* length of the buffer     */
			   loff_t * offset)
{
	struct private_data *data = filp->private_data;
	char *copy_root = data->read_string + data->offset;
	size_t str_length, copy_length;
	if (data->offset == 0) {
		scnprintf(data->read_string, sizeof(data->read_string), "%lu\n", operation_count);
	}
	str_length = strlen(copy_root);
	if (str_length == data->offset) {
		return 0;
	}
	copy_length = str_length > buffer_length ? buffer_length : str_length;
	if (copy_to_user(buffer, copy_root, copy_length) != 0) {
		printk(KERN_ALERT "Could not copy data to userspace.\n");
		return -EIO;
	}
	data->offset += copy_length;
	return copy_length;
}

/*  
 * Called when a process writes to dev file: echo "hi" > /dev/hello 
 */
static ssize_t
device_write(struct file *filp, const char *input_buff, size_t input_len, loff_t * off)
{
	char buf[WRITE_BUF_SIZE];
	operation_count += 1;
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

