obj-m := message_slot.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	sudo insmod message_slot.ko
	gcc -O3 -Wall -std=c11 message_sender.c -o message_sender.o
	gcc -O3 -Wall -std=c11 message_reader.c -o message_reader.o
	gcc -O3 -Wall -std=c11 slot_tester.c -o slot_tester.o
	./slot_tester.o /dev/message_slot
 
clean:
	sudo rmmod message_slot
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	

