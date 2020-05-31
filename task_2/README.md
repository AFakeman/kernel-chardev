# Spinlocks vs. mutexes

[The task (in Russian)](https://docs.google.com/document/d/1Z6NF2su0c6WOTEC-BDS9Ic1lkRLtIDv_V3OqlDgfbzg/edit?hl=en)

This kernel module does a busy-loop with locking and unlocking using multiple
threads in two ways: with mutexes, and with spinlocks. For each task, 16 threads are spawned and each one does 512 increments of an integer. Each increment involves acquiring and releasing a lock.

## Usage

### Vagrant setup
* `vagrant up`
* `vagrant ssh -c "cd data; make && make profile"`

### Manually
```bash
sudo apt-get install -y linux-headers-$(uname -r);
make && make profile
```
## Results
Ran on a Intel(R) Core(TM) i7-7820HQ CPU @ 2.90GHz CPU, inside VirtualBox 6.1 in a VM with 1 CPU core allocated. Task time spent also inclues initialization of data structures.

* Average time of spinlock task: 370us
* Average time of mutex task: 354us
