# Lists all projects
PROJECTS=kernel libc boot initrd userland
# Lists all projects that need headers installed
HEADER_PROJECTS=kernel libc

# Root defaults to cwd; must be set properly if using make -C
export ROOT?=$(CURDIR)
# Sets directories for output based on defaults, and exports if needed
export SYSROOT=$(ROOT)/sysroot
export ISODIR?=$(ROOT)/isodir
export BUILDDIR?=$(ROOT)/build
export DESTDIR?=$(SYSROOT)

# Default host is given by script
DEFAULT_HOST!=./default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=./target-triplet-to-arch.sh $(HOST)

# Sets CC, AR, and OBJCOPY to respect host and use SYSROOT
export CC:=$(HOST)-gcc --sysroot=$(SYSROOT) -isystem=$(SYSROOT)/usr/include $(DEFINES)
export CXX:=$(HOST)-g++ --sysroot=$(SYSROOT) -isystem=$(SYSROOT)/usr/include $(DEFINES)
export AR:=$(HOST)-ar
export OBJCOPY:=$(HOST)-objcopy

.PHONY: all
all: os_2.iso os_2.img

# Makes iso - headers must be installed first, then each PROJECT
# Makes iso by creating a directory with kernel image and grub.cfg,
# then calling grub-mkrescue appropriately
os_2.iso: install-sources install-headers $(PROJECTS)
	mkdir -p $(ISODIR)/boot/grub
	mkdir -p $(ISODIR)/modules
	$(OBJCOPY) -S $(SYSROOT)/boot/boot_loader.o $(ISODIR)/boot/boot_loader.o
	$(OBJCOPY) -S $(SYSROOT)/boot/os_2.o $(ISODIR)/modules/os_2.o
	cp --preserve=timestamps $(SYSROOT)/boot/initrd.bin $(ISODIR)/modules/initrd.bin
	cp --preserve=timestamps $(ROOT)/grub.cfg $(ISODIR)/boot/grub
	grub-file --is-x86-multiboot2 $(ISODIR)/boot/boot_loader.o
	grub-mkrescue -o $@ $(ISODIR)

os_2.img: $(PROJECTS)
	sudo $(ROOT)/makeimg.sh

# Makes project by calling its Makefile
.PHONY: $(PROJECTS)
$(PROJECTS):
	$(MAKE) install -C $(ROOT)/$@

# Makes the kernel depend on libc
kernel: libc

# Makes the initrd depend on libc
initrd: libc

# Makes the userland depend on libc
userland: libc

# Cleans by removing all output directories and calling each project's clean
.PHONY: clean
clean:
	rm -rf $(ISODIR)
	rm -rf $(BUILDDIR)
	rm -f $(ROOT)/initrd/files/*.o
	rm -f $(ROOT)/*.dis
	rm -f $(ROOT)/debug.log
	rm -f $(ROOT)/os_2.iso

.PHONY: run
run:
	$(ROOT)/qemu.sh

.PHONY: debug
debug:
	$(ROOT)/qemu.sh --debug

.PHONY: install-sources
install-sources:
	mkdir -p $(BUILDDIR)
	cd $(ROOT); \
	for dir in $(PROJECTS); do \
	  find ./$$dir \( -name '*.c' -o -name '*.h' -o -name '*.S' -o -name '*.cpp' \) -exec cp --preserve=timestamps --parents -u \{\} $(BUILDDIR) \;; \
	done

# Installs headers by calling each project's install-headers
.PHONY: install-headers
install-headers:
	for dir in $(HEADER_PROJECTS); do \
	  $(MAKE) install-headers -C $(ROOT)/$$dir; \
	done

disassemble: all
	$(HOST)-objdump -D $(SYSROOT)/boot/boot_loader.o > $(ROOT)/boot_loader.dis
	$(HOST)-objdump -D $(SYSROOT)/boot/os_2.o > $(ROOT)/kernel.dis