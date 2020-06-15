/* Linker script for xv6pp */

/* OUTPUT_ARCH(x86_64) */
ENTRY(_start)

kernelbase = 0x80000000; /* 2GiB */
multiboot_load = kernelbase;

SECTIONS {
	/* Link kernel at 1Mib offset in the last TB
   * of virtual address space. */
	. = kernelbase; /* 2GiB -- should match KERNBASE */

	/* load sections physically starting at 1Mib */
	.text : AT(multiboot_load) {
		KEEP(*(.multiboot_header));
		*(.text .text.*)
	}
	
	.rodata : {
		*(.rodata .rodata.*);
	}

	/* Define EndText symbol to current value of . */
	PROVIDE(etext = .);

	/* Align data section with 4K page boundary */
	. = ALIGN(0x1000);

	/* Conventionally, Unix linkers provide pseudo-symbols
	 * etext, edata, and end, at the end of the text, data, and bss.
	 * For the kernel mapping, we need the address at the multiboot_header
	 * of the data section, but that's not one of the conventional
	 * symbols, because the convention started before there was a
	 * read-only rodata section between text and data.
	 * Note: This implementation links rodata before the text section. */
	PROVIDE(data = .);

	/* The data segment */
	.data : {
		*(.data .data.*)
	}

	PROVIDE(edata = .); /* end of data segment */

	.bss : {
		*(.bss .bss.*)
	}

	PROVIDE(end = .); /* end of kernel */

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}

multiboot_load_end = multiboot_load + (edata - multiboot_header);
multiboot_bss_end = multiboot_load + (end - multiboot_header);
multiboot_entry = multiboot_load + (entry - multiboot_header);
bootgdt_addr = multiboot_load + (bootgdt - multiboot_header);
