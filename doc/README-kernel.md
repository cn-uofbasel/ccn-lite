# CCN-lite for the Linux kernel

## Prerequisites

Follow the [UNIX installation instructions](README-unix.md) to set up the
CCN-lite sources and relevant environment variables.

## Installation

1.  Define the environment variable `$USE_KRNL` to compile CCN-lite in kernel mode:

    ```bash
    cd $CCNL_HOME/src
    export USE_KRNL=1
    make clean all
    ```

    Use the target `ccn-lite-lnxkernel` to compile only the module.

2.  Insert the module:
    ```bash
    sudo insmod ./ccn-lite-lnxkernel.ko e=eth0 v=trace
    ```

3.  Verify that the module was succesfully inserted into the kernel with:
    ```bash
    dmesg
    ```
    or alternatively
    ```bash
    tail /var/log/syslog
    ```
    Use option `-f` to continuously monitor the module's output:
    ```bash
    tail -f /var/log/syslog
    ```

## Usage

### Options

The module has several options when inserting. You can list them using `modinfo`:

```bash
modinfo ./ccn-lite-lnxkernel.ko
```

[//]: # (Add link to document, more information on key options)

The following describes options special to the kernel. For all other options, see the output of `ccn-lite-relay -h`.

* `k` - path to the public key used to verify messages sent from `ccn-lite-ctrl`
* `p` - path to the private key

### Control

You can control the module via `ccn-lite-ctrl` through the UNIX socket. For example to trigger a dump, use:

```bash
sudo $CCNL_HOME/bin/ccn-lite-ctrl debug dump | ccn-lite-ccnb2xml
```

Notice that you need to use `sudo` issuing the control command to access the UNIX socket. Type `ccn-lite-ctrl -h` to see all available commands.

### Removal

Use `rmmod` to remove the module from the kernel:
```bash
sudo rmmod ccn_lite_lnxkernel
```
