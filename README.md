# Chardev - example Linux Kernel Character Device Driver

## Functionality

Supported operations:
* `write` - writes message to kernel logs
* `read` - outputs count of write operations
* `open` - requires release before another open
* `release` - does not permit unloading of the module until file is released
