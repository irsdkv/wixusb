CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

obj-m   := wixusb_module.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean: 
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order 
	@rm -f .*.*.cmd *~ *.*~ TODO.* 
	@rm -fR .tmp* 
	@rm -rf .tmp_versions 
disclean: clean 
	@rm *.ko *.symvers

