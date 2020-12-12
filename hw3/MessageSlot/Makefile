obj-m := message_slot.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -O3 -Wall -std=c11 message_sender.c 
	gcc -O3 -Wall -std=c11 message_reader.c
 
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

py:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	sudo insmod message_slot.ko
	gcc -O3 -Wall -std=c11 message_sender.c message_slot.h -o message_sender.o
	gcc -O3 -Wall -std=c11 message_reader.c message_slot.h -o message_reader.o
	python3 ./tester.py


pyclean:
	sudo rmmod message_slot
	$(MAKE) -C $(KDIR) M=$(PWD) clean


mymake:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	sudo insmod message_slot.ko
	sudo mknod /dev/message_slot c 240 0
	sudo chmod o+rw /dev/message_slot
	gcc -O3 -Wall -std=c11 message_sender.c -o message_sender.o
	gcc -O3 -Wall -std=c11 message_reader.c -o message_reader.o
	gcc -O3 -Wall -std=c11 slot_tester.c -o slot_tester.o
	./slot_tester.o /dev/message_slot

myclean:
	sudo rmmod message_slot
	sudo rm /dev/message_slot
	$(MAKE) -C $(KDIR) M=$(PWD) clean
