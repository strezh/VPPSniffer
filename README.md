# VPPSniffer

## Overview

A simple Linux 3.X kernel modules for debugging and sniffing parallel port (LPT)applications without real physical port

It also works with VirtualBox

Tested in Linux Debian 8.0 (Jessie) with Linux kernel 3.16.0

## Contents

- parportvirt: virtual parallel port sources kernel module
- parportsnif: parallel port sniffer kernel module for linux 3.X
- install_port: simple "make and install" script
- set_vbox_settings: simple VirualBox configuring script

## Installation

### Simple

```sh
./install_port
```

### Manual

```sh
# insert and remove standard modules (if yuo don't have physical LPT)
~$ sudo modprobe parport_pc
~$ sudo modprobe ppdev
~$ sudo rmmod lp

# build kernel modules
~$ make
# insert modules
sudo make insert

#change privileges for users
~$ sudo chmod 666 /dev/parportsnif0 
~$ sudo chmod 666 /dev/parport0
```

## Using
You can test correct installation and runnig modules by this command:
```sh
~$ ls /dev/parport*
```
Correct responce must be like this:
```sh
/dev/parport0  /dev/parportsnif0
```
Device `/dev/parport0` is parallel port (virtual or real if exist)
Device `/dev/parportsnif0` is fake parallel port for sniffering
All information from sniffer logged to `/proc/parportlog` file

## Testing

Start write sniffer log file to dump.ols
```sh
~$ cat /proc/parportlog > dump.ols
```

Run test program
```sh
~$ ./sniftest/sniftest
```

Press `Ctrl+C` to break logging and open dump file dump.ols
```
;Rate: 1000000
;Channels: 32
0000000055@100099
00000000AA@200170
0000000055@300247
00000000AA@400317
0000000000@500387
# [1433923145.442004659] 0 CLOSE
```

You can open dump.ols with [Open Logic Sniffer](http://dangerousprototypes.com/docs/Open_Bench_Logic_Sniffer)

## VirtualBox configuration
Configuration LPT1 port (0x378 address) for Windows Virtual Machine in VirtualBox

### Manual
```sh
VBoxManage setextradata <VM_NAME> "VBoxInternal/Devices/parallel/0/LUN#0/Config/DevicePath" /dev/parportsnif0
VBoxManage setextradata <VM_NAME> "VBoxInternal/Devices/parallel/0/LUN#0/Driver" HostParallel
VBoxManage setextradata <VM_NAME> "VBoxInternal/Devices/parallel/0/Config/IOBase" 0x378
VBoxManage setextradata <VM_NAME> "VBoxInternal/Devices/parallel/0/Config/IRQ" 7
```
where `<VM_NAME>` - name of Virtual Machine
### Simple
```sh
set_vbox_settings <VM_NAME>
```
where `<VM_NAME>` - name of Virtual Machine

## Links

- [Parallel port sniffer (for kernel 2.X)](http://www.alfersoft.com.ar/blog/2012/02/23/parallel-port-sniffer-for-linux/)
- [Parallel port sniffer (for kernel 3.X)](https://github.com/jwfang/eldd)
- Virtual driver for parport(LPT) on [linuxquestions.org forum](http://www.linuxquestions.org/questions/programming-9/virtual-driver-for-parport-lpt-809059/)
- [VirtualBox Parallel port configuration](http://blog.my1matrix.org/2013/04/parallel-port-on-virtualbox.html)
