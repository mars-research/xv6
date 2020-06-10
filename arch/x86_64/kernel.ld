/* Linker script for xv6pp */

/* OUTPUT_ARCH(x86_64) */
ENTRY(entry)

multiboot_load = 0x100000; /* 1MiB */

SECTIONS {
	/* Link kernel at 1Mib offset in the last TB
   * of virtual address space. */
	. = 0xFFFFFFFF80100000; /* 2GiB + 1MiB */
  begin = .;

	/* load sections physically starting at 1Mib */
	.rodata : AT(multiboot_load) {
		KEEP(*(.multiboot_header));
		*(.rodata .rodata.*);
	}

	/* . = ALIGN(0x1000); */
	.text : {
		*(.text .text.*)
	}

	/* Define EndText symbol to current value of . */
	PROVIDE(etext = .);

	/* Align data section with 4K page boundary */
	. = ALIGN(0x1000);

	/* Conventionally, Unix linkers provide pseudo-symbols
	 * etext, edata, and end, at the end of the text, data, and bss.
	 * For the kernel mapping, we need the address at the beginning
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

multiboot_load_end = multiboot_load + (edata - begin);
multiboot_bss_end = multiboot_load + (end - begin);
multiboot_entry = multiboot_load + (entry - begin);
