cmd_/home/student/OS/hw3/MessageSlot/message_slot.ko := ld -r -m elf_x86_64  -z max-page-size=0x200000 -T ./scripts/module-common.lds  --build-id  -o /home/student/OS/hw3/MessageSlot/message_slot.ko /home/student/OS/hw3/MessageSlot/message_slot.o /home/student/OS/hw3/MessageSlot/message_slot.mod.o ;  true
