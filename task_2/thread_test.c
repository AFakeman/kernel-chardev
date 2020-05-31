#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>

#define THREADS 16

const size_t ITERATIONS = 512;  // Why use non-power of two?

spinlock_t spl;
struct mutex mtx;  // Using dynamic init for uniformity
int guarded = 0;  // Value guarded by a lock

/*
 *  Prototypes
 */
static int myinit(void);
static void myexit(void);
static void run_test(int thread_task(void*));

struct thread_data {
    struct completion *start;
    struct completion *done;
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
    run_test(thread_task_spl);
    run_test(thread_task_mtx);
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
 * This function is called when the module is unloaded
 */
static void myexit(void)
{
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");

