# Synapticon build of the Etherlab EtherCAT Master Linux kernel versions > 3

**Latest tested version is 5.19.0**

Newer kernel versions have changes in parts of the `linux/v5.19/source/include/linux/netdevice.h`
that has confilcts with the code for the IgH driver.

1. Confliting change in `netdevice.h` in the structure `net_device`, member `dev_addr` was constified for kernel **versions >= 5.17**. Link: https://elixir.bootlin.com/linux/v5.19/source/include/linux/netdevice.h#L2159

2. Conflicting change, identifier `netif_rx_ni` doesn't exist in kernel **versions >= 5.18**. Link: https://elixir.bootlin.com/linux/v5.17.15/source/include/linux/netdevice.h#L3672


# Install the IgH

Run the command:

```bash
sudo ./synapticon/install.sh
```


# Kernel module signature

If Linux is run in SafeBoot mode a kernel module must be provided in order for the IgH to be
uploaded in the kernel. Without the module signature the following error is reported, on starting
the ehercat service:


```
$ sudo /etc/init.d/ethercat start
"modprobe: ERROR: could not insert 'ec_master': Key was rejected by service"
```

## Signing kernel modules

The "howto" was taken from this blog page with an example of signing VirtualBox:


https://stegard.net/2016/10/virtualbox-secure-boot-ubuntu-fail/


Basically it boils to this (with changes for our case):


Here are the steps I did to enable IgH to work properly in Ubuntu with UEFI Secure Boot fully enabled*. The problem is the requirement that all kernel modules must be signed by a key trusted by the UEFI system, otherwise loading will fail. Ubuntu does not sign the third party  kernel modules, but rather gives the user the option to disable Secure Boot. I could do that, but then I would see an annoying “Booting in insecure mode” message every time the machine starts, and also the dual boot Windows 10 installation I have would not function.

Credit goes to the primary source of information I used to resolve this problem, which applies specifically to Fedora/Redhat:
http://gorka.eguileor.com/vbox-vmware-in-secureboot-linux-2016-update/

And a relevant Ask Ubuntu question:
http://askubuntu.com/questions/760671/could-not-load-vboxdrv-after-upgrade-to-ubuntu-16-04-and-i-want-to-keep-secur
Steps to make it work, specifically for Ubuntu/Debian

Install the package. If the installation detects that Secure Boot is enabled, you will be presented with the issue at hand and given the option to disable Secure Boot. Choose “No”.
Create a personal public/private RSA key pair which will be used to sign kernel modules. I chose to use the root account and the directory /root/module-signing/ to store all things related to signing kernel modules.

```bash
$ sudo -i
$ mkdir /root/module-signing
$ cd /root/module-signing
$ openssl req -new -x509 -newkey rsa:2048 -keyout MOK.priv -outform DER -out MOK.der -nodes -days 36500 -subj "/CN=YOUR_NAME/"
$ chmod 600 MOK.priv
```

Use the MOK (“Machine Owner Key”) utility to import the public key so that it can be trusted by the system. This is a two step process where the key is first imported, and then later must be enrolled when the machine is booted the next time. A simple password is good enough, as it is only for temporary use.

```
$ mokutil --import /root/module-signing/MOK.der
    input password:
    input password again:
```

Reboot the machine. When the bootloader starts, the MOK manager EFI utility should automatically start. It will ask for parts of the password supplied in step 3. Choose to “Enroll MOK”, then you should see the key imported in step 3. Complete the enrollment steps, then continue with the boot. The Linux kernel will log the keys that are loaded, and you should be able to see your own key with the command: dmesg|grep 'EFI: Loaded cert'
Using a signing utility shipped with the kernel build files, sign all the modules using the private MOK key generated in step 2. I put this in a small script `/root/module-signing/sign-igh-modules.sh`, so it can be easily run when new kernels are installed as part of regular updates:

```bash
#!/bin/bash

for modfile in $(dirname $(modinfo -n ec_master))/*.ko; do
echo "Signing $modfile"
/usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 \
                                /root/module-signing/MOK.priv \
                                /root/module-signing/MOK.der "$modfile"
done

for modfile in $(dirname $(modinfo -n ec_generic))/*.ko; do
echo "Signing $modfile"
/usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 \
                                /root/module-signing/MOK.priv \
                                /root/module-signing/MOK.der "$modfile"
done
```

```bash
$ chmod 700 /root/module-signing/sign-igh-modules.sh
```

Sign the modules

```bash
$ sudo /root/module-signing/sign-igh-modules.sh
```

Run the script from step 5 as root. You will need to run the signing script every time a new kernel
update is installed, since this will cause a rebuild of the third party modules. Use the script only
after the new kernel has been booted, since it relies on modinfo -n and uname -r to tell which
kernel version to sign for.

# Restart the kernel module

```bash
sudo /etc/init.d/ethercat restart
```