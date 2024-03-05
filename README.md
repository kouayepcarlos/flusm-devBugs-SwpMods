# USM

Userspace memory management project.

# Environment #

First, get USM's repository. You should've gotten it with this READMe.

To keep things simple, we'll all try to use the same environment. We tested many ones lately, and, while it does generally be doable under any situation, the hassle of even just the dependencies hunting, greatly depending on your hardware, is enough a reason not to try out that road ; scout's word.

We'll hence be using Ubuntu 22.04, in a VM (Qemu/KVM), coupled with the maximum of CPU cores available, and preferably 8 Gigs of RAM (at least 6), 2 of which'll be reserved for some contiguous memory we'll be managing (using DMA_CMA). We recommend, too, using as CPU architecture the one closer to your host's model (probably Broadwell-noTSX-IBRS).

The latter's checkable through :

```sh
virsh -c qemu:///system
capabilities
```

The first listed should be it.

# Dependencies #

## Runtime dependencies ##

 * userfaultfd -- the page faults passing to userspace mechanism, quite important for synchronization matters, otherwise disposable

## Build dependencies ##

The build dependencies are mainly classical ones (i.e. kernel building). Only one precaution needs to be taken with dwarves, which has to be inferior to the 1.24 version, hence why it is included for convenience. It should stay on 1.22 on Ubuntu 22.04 for quite a while, but we never know.

 * build-essential
 * bison
 * flex
 * libncurses-dev
 * libssl-dev
 * libelf-dev
 * dwarves
 * cmake
 * pkg-config
 
A baseline is gettable with :
```sh
sudo apt install build-essential bison flex libncurses-dev libssl-dev libelf-dev dwarves cmake pkg-config clang -y
```
 
## How to build ##

Estimated, with deployment, time : ~25 minutes (greatly dependent on your processors.. (number of CPUs passed to make with "-j")).

### USM's repository ###

First, get USM's repository. You should've gotten it with this READMe.

We need to first check dwarves' installed version :
```sh
pahole --version
```
***_Skip to kernel building if the value is less than or equal to 1.22._***
If it is any higher than 1.22, we'd need to uninstall it :
```sh
sudo apt remove pahole dwarves -y
sudo apt install libdw-dev libbpf-dev -y
```
We'd then go into the dwarves-1.22 dependency folder (in the repo.'s *Dependencies* folder) and do :
```sh
mkdir build
cd build
cmake -D__LIB=lib -DBUILD_SHARED_LIBS=OFF ..
sudo make install
```
#### Kernel building ####
Then we simply compile and install the kernel and its headers. We start with, in the Kernel folder :
```sh
cp /boot/config-$(uname -r) .config
```
We then activate some options linked to *CMA* (Contiguous Memory Allocator) :
```sh
echo -e "CONFIG_CMA=y\nCONFIG_DMA_CMA=y" >> .config
```
Once again, you need to have these in your config :
```sh
CONFIG_CMA=y
CONFIG_DMA_CMA=y
```
We'd then need to do this next command. It simply calls a script that modifies the config file, activating the options linked to the modules currently in use and verifying any depencies between options in the config file. It will hence prompt some questions about miscellaneous and linked options, to which we'll negatively answer to the maximum prompts possible (basically all except CMA related ones ; you should leave the CMA size prompt's value to its default).
```sh
make localmodconfig
```
The CMA size is configured by adding **cma=2G** (or any other amount, depending on your total RAM amount) to linux's arg.s :
```sh
sudo nano /etc/default/grub
```
Your *grub* file should now look like this (*GRUB_SAVEDEFAULT* helps in applying to all subsequent boots the last booted kernel (effectively putting into *saved* the last booted Kernel's number)) :
```sh
GRUB_DEFAULT=saved
GRUB_SAVEDEFAULT=true
#GRUB_TIMEOUT_STYLE=hidden
GRUB_TIMEOUT=5
GRUB_DISTRIBUTOR=`lsb_release -i -s 2> /dev/null || echo Debian`
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash"
GRUB_CMDLINE_LINUX="transparent_hugepage=never cgroup_disable=memory selinux=0 cma=2G"
```
Or :
```sh
GRUB_DEFAULT=saved
GRUB_SAVEDEFAULT=true
#GRUB_TIMEOUT_STYLE=hidden
GRUB_TIMEOUT=5
GRUB_DISTRIBUTOR=`lsb_release -i -s 2> /dev/null || echo Debian`
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash transparent_hugepage=never cgroup_disable=memory selinux=0 cma=2G"
GRUB_CMDLINE_LINUX=""
```
A further important precision about *CMA*'s amount is that you should always leave at least **4G** left when trying/choosing values.
Disabling key signing might as well be a good idea, to further mitigate possible related issues. It is still done by modifying _.config_, using provided scripts :
```sh
scripts/config --disable SYSTEM_TRUSTED_KEYS && scripts/config --disable SYSTEM_REVOCATION_KEYS && scripts/config --set-str CONFIG_SYSTEM_TRUSTED_KEYS "" && scripts/config --set-str CONFIG_SYSTEM_REVOCATION_KEYS ""
```
We then do the main part with (this first instruction's the longest in all this process) :
```sh
sudo make -j `getconf _NPROCESSORS_ONLN`
sudo make modules_install
sudo make install
sudo make headers_install INSTALL_HDR_PATH=/usr
```
`make install` already updates grub, but we can do so again, especially if you modified the grub after the latter, with :
```sh
sudo update-grub
```
We recommend copying/saving periodically your disk image, which'd help a lot whenever your environment gets corrupted. It is located, by default, at */var/lib/libvirt/images/name_of_your_disk_image*.

We further recommend disabling swapping, by either issuing a _swapoff_ command, adding `vm.swappiness = 0` to */etc/sysctl.conf*, or commenting out the line starting with `/swapfile` in */etc/fstab* ; you should do them all (!) :
```sh
sudo swapoff -a
sudo nano /etc/sysctl.conf
sudo nano /etc/fstab
```
After now rebooting in the just installed Kernel (the 5.18.9 one in the options list), the allocator and tagger can then be generated with, in the *Userspace* folder (usmAllocator's deprecated, use the API'ed version) :
```sh
cd Userspace
make
cd APIv
make
```
The latters need the module loaded to properly function. Open the *Module* folder, and modify **usmMemSize** to a value ***inferior*** to the value given at boot (in this case, as we had **CMA=2G**, we'd at most put ~1.9*1024*1024*1024 in there) ; then :
```sh
make
sudo insmod usm_lkmRM.ko
```
You could have issues with loading the module, such as *..invalid format..*. That'd simply mean, normally, that you didn't boot with the relevant kernel, which is, for instance, of *5.18.9* version for the moment. Furthermore, you should get rid of other linux headers and related, as you won't be needing them and could be a solution if the latter didn't work :
```sh
sudo apt remove linux-headers-*
```
Keep in mind that this all is under the assumption of a dedicated VM's usage. This could get rid of drivers you could need if on bare-metal, so proceed with caution in such case (tabulate and get rid of relevant thingies).
If you still have issues loading the module until this point, you mustn't have had the required options in your *config*, at kernel compilation. Redo it all and double check, after **make localmodconfig**, if you have them still enabled.
*..resource unavailable..* error means that the *CMA pool* couldn't be loaded at boot. The answer to that is to simply reduce the amount asked while considering what we previously mentioned, or just rebooting (could happen here and there.. not thoroughly investigated for the moment). The reserved *CMA pool* can be verified with :
```sh
sudo dmesg | grep cma
```
You can check whether the insertion/loading went well with (you'll see a little "Yo!" at the end) :
```sh
sudo dmesg
```
At this point, you might be interested in blocking any annoying update kernel related :
```sh
sudo apt-mark hold linux-image-generic
sudo apt-mark hold linux-headers-generic
```
# Synopsis #

Launch USM, with, after modifying the *config_file*'s (an example is, from the *Userspace/APIv* folder, _examples/project-2/cfg/alloc_policy_assignment_strategy.cfg_) ***memory*** argument to, the pool or less you *reserved* through *CMA*, and *taken* in the *Module* :
```sh
cd APIv
sudo ./project-2 config_file
```
A manual (*Manual.txt*) is available in _APIv_.
You'll have a *..CMA untakable..* if you didn't insert the module beforehand, as described earlier.
Then, after its complete initialization, launch any application with USM's tagger:
```sh
sudo LD_PRELOAD=./usmTagger ./application_wanted_managed_by_usm
```
##### Usage #####

You basically just need to include **usm.h** in your USM project, create your structures and anything wanted, define the mandatory interfaces (clear and complete example, **project-2** as launched in the synopsis, available in *APIv*, with usable comments), then compile and run your custom USM instance with your policies.
You will find the said mandatory or not definitions in each subcomponent's (we have two for now : the allocation and eviction/swapping ones) last lines : starting _usm_basic_alloc_policy_ for the allocation subcomponent, defined in *examples/project-2/alloc/src/policiesSet1.c*, and starting _swap_device_one_ops_ for the swapping subcomponent (that isn't yet completely verified but is working), located at *examples/project-2/evict/src/policiesSet1.c*.
