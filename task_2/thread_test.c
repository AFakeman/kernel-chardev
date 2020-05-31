#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>

#define THREADS 16

const size_t ITERATIONS = 512;  // Why use non-power of two?

spinlock_t spl;
struct mutex mtx;  // Using dynamic init for uniformity
int guarded = 0;  // Value guarded by a lock

static int Major;		/* Major number assigned to our device driver */

static struct cdev mycdev;
static struct class *myclass = NULL;

#define SUCCESS 0
#define FAIL -1
#define DEVICE_NAME "thread_test"

/*
 *  Prototypes
 */
static int myinit(void);
static void myexit(void);
static void run_test(int thread_task(void*));
static void cleanup(int device_created);
static int device_open(struct inode *, struct file *);

struct thread_data {
    struct completion *start;
    struct completion *done;
};
static struct file_operations fops = {
	owner: THIS_MODULE, // https://stackoverflow.com/questions/51085225/prevent-removal-of-busy-kernel-module
	open: device_open,
};


/*
 * Post-increment operation using a lock
 */
inline static int guarded_inc_spl(void) {
    int ret;
    unsigned long flags;
    spin_lock_irqsave(&spl, flags);
    ret = guarded++;
    spin_unlock_irqrestore(&spl, flags);
    return ret;
}

static int thread_task_spl(void *data) {
    size_t i;
    struct thread_data *td = (struct thread_data *) data;
    wait_for_completion(td->start);
    for (i = 0; i < ITERATIONS; ++i) {
        guarded_inc_spl();
    }
    complete(td->done);
    return 0;
}

/*
 * Post-increment operation using a lock
 */
inline static int guarded_inc_mtx(void) {
    int ret;
    mutex_lock(&mtx);
    ret = guarded++;
    mutex_unlock(&mtx);
    return ret;
}

/*
 * Yes, we could use function pointers not to duplicate the thread_task,
 * but then we couldn't inline the function, and this may affect our time spent
 * waiting.
 */
static int thread_task_mtx(void *data) {
    size_t i;
    struct thread_data *td = (struct thread_data *) data;
    wait_for_completion(td->start);
    for (i = 0; i < ITERATIONS; ++i) {
        guarded_inc_spl();
    }
    complete(td->done);
    return 0;
}

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
    return 0;
}

static void run_test(int thread_task(void*)) {
    size_t i;
    struct completion comp[THREADS];
    struct completion start;
    struct thread_data tds[THREADS];
    guarded = 0;

    init_completion(&start);
    for (i = 0; i < THREADS; ++i) {
        init_completion(&comp[i]);
        tds[i].done = &comp[i];
        tds[i].start = &start;
    }

    for (i = 0; i < THREADS; ++i) {
        kthread_run(thread_task, &tds[i], "bench_sem_%lu", i);
    }
    complete_all(&start);

    for (i = 0; i < THREADS; ++i) {
        wait_for_completion(&comp[i]);
    }
    WARN_ON(guarded != ITERATIONS * THREADS);
}

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
    run_test(thread_task_spl);
    run_test(thread_task_mtx);
	return 0;
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

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");

