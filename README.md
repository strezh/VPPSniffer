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
sudo modprobe parport_pc
sudo modprobe ppdev
sudo rmmod lp

# build kernel modules
make
# insert modules
sudo make insert
```

## Change privileges for users

```sh
sudo chmod 666 /dev/parportsnif0 
sudo chmod 666 /dev/parport0
```

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
