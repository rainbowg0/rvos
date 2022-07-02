/* 
  riscv is the name of architecture that the linker understands
  for any RISC-V target.
*/
OUTPUT_ARCH( "riscv" )
/*
  set entry point to a symbol called _entry inside of entry.S.
  which stores the address of _entry as the entry point where 
  CPU instructions should start executing.
*/
ENTRY( _entry )

/*
  'Memory' section will explain that have 'ram' that contains
  a section which is 'w' (writable), 'x' (executable),
  'a' (allocatable). use '!' to invert 'r' (read-only) and 
  'i' (initialized), so our memory is not ready-only, and
  it's not initialized at the beginning.

  'ORIGIN' set memory address at 0x8000_0000, which is the
  starting memory address of kernel.

  'LENGTH' tells the linker that we have 128MB of RAM.
*/
MEMORY 
{
  ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}

/*
  'PHDRS' is short for 'program headers', which specify three:
  text - CPU instructions (execuatable sections)
  data - Global, initialized variables
  bss - Global, uninitialized variables (all will be set to 0)

  PT_LOAD tells the linker that these sections will be loaded 
  from the file into memory
*/
PHDRS
{
  text PT_LOAD;
  data PT_LOAD;
  bss PT_LOAD;
}

/*
  Organize the memory based on which section it is in.
  - CPU instructions go into text.
  - global constants go into rodata.
  - global initialized variables go into data.
  - global uninitialized variables go into bss.
*/
SECTIONS
{
  /*
    Teh first part of RAM layout will be text section.
    Memory starts at 0x8000_0000.
  */
  .text : {
    /*
      PROVIDE allows to access a symbol called _text_start, from
      which can know where the text section starts in the 
      operating system.
      '.' tells the linker to set _test_start to the CURRENT location.
      current location moves as add things.
    */
    PROVIDE(_text_start = .);

    /*
      We are going to layout all text sections here, starting with 
      .text.init. The asterisk in front of the parentheses means to match
      the .text.init section of ANY object file. Otherwise, we can specify
      which object file should contain the .text.init section, for example,
      boot.o(.text.init) would specifically put the .text.init section of
      our bootloader here.

      Because we might want to change the name of our files, we'll leave it
      with a *.

      Inside the parentheses is the name of the section. I created my own
      called .text.init to make 100% sure that the _entry is put right at the
      beginning. The linker will lay this out in the order it receives it:

      .text.init first
      all .text sections next
      any .text.* sections last

      .text.* means to match anything after .text. If we didn't already specify
      .text.init, this would've matched here. The assembler and linker can place
      things in "special" text sections, so we match any we might come across here.
	  */
    *(.text.init) *(.text .text.*)

    /*
	    Again, with PROVIDE, we're providing a readable symbol called _text_end, which is
	    set to the memory address AFTER .text.init, .text, and .text.*'s have been added.
	  */
    PROVIDE(_text_end = .);

    /*
      The portion after the right brace is in an odd format. However, this is telling the
      linker what memory portion to put it in. We labeled our RAM, ram, with the constraints
      that it is writeable, allocatable, and executable. The linker will make sure with this
      that we can do all of those things.

      >ram - This just tells the linker script to put this entire section (.text) into the
            ram region of memory. To my knowledge, the '>' does not mean "greater than". Instead,
        it is a symbol to let the linker know we want to put this in ram.

      AT>ram - This sets the LMA (load memory address) region to the same thing. LMA is the final
              translation of a VMA (virtual memory address). With this linker script, we're loading
          everything into its physical location. We'll let the kernel copy and sort out the 
          virtual memory. That's why >ram and AT>ram are continually the same thing.

      :text  - This tells the linker script to put this into the :text program header. We've only
              defined three: text, data, and bss. In this case, we're telling the linker script
          to go into the text section.
	  */
  } >ram AT>ram :text

  /*
    The global pointer allows the linker to position global variables and constants into
    independent positions relative to the gp (global pointer) register. The globals start
    after the text sections and are only relevant to the rodata, data, and bss sections.
   */
   PROVIDE(_global_pointer = .);
   /*
    Most compilers create a rodata (read only data) section for global constants. However,
    we're going to place ours in the text section. We can actually put this in :data, but
    since the .text section is read-only, we can place it there.

    NOTE: This doesn't actually do anything, yet. The actual "protection" cannot be done
    at link time. Instead, when we program the memory management unit (MMU), we will be
    able to choose which bits (R=read, W=write, X=execute) we want each memory segment
    to be able to do.
   */
   .rodata : {
    PROVIDE(_rodata_start = .);
    *(.rodata .rodata.*)
    PROVIDE(_rodata_end = .);
	  /*
	    Again, we're placing the rodata section in the memory segment "ram" and we're putting
	    it in the :text program header. We don't have one for rodata anyway.
	  */
    } >ram AT>ram :text

    .data : {
    /*
      . = ALIGN(4096) tells the linker to align the current memory location (which is
      0x8000_0000 + text section + rodata section) to 4096 bytes. This is because our paging
      system's resolution is 4,096 bytes or 4 KiB.
    */
    . = ALIGN(4096);
    PROVIDE(_data_start = .);
    /*
      sdata and data are essentially the same thing. However, compilers usually use the
      sdata sections for shorter, quicker loading sections. So, usually critical data
      is loaded there. However, we're loading all of this in one fell swoop.
      So, we're looking to put all of the following sections under the umbrella .data:
      .sdata
      .sdata.[anything]
      .data
      .data.[anything]

      ...in that order.
    */
    *(.sdata .sdata.*) *(.data .data.*)
    PROVIDE(_data_end = .);
    } >ram AT>ram :data

   .bss : {
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*) *(.bss .bss.*)
    PROVIDE(_bss_end = .);
    } >ram AT>ram :bss

  /*
    The following will be helpful when we allocate the kernel stack (_stack) and
    determine where the heap begnis and ends (_heap_start and _heap_start + _heap_size)/
    When we do memory allocation, we can use these symbols.

    We use the symbols instead of hard-coding an address because this is a floating target.
    As we add code, the heap moves farther down the memory and gets shorter.

    _memory_start will be set to 0x8000_0000 here. We use ORIGIN(ram) so that it will take
    whatever we set the origin of ram to. Otherwise, we'd have to change it more than once
    if we ever stray away from 0x8000_0000 as our entry point.
  */
  PROVIDE(_memory_start = ORIGIN(ram));
  /*
    Our kernel stack starts at the end of the bss segment (_bss_end). However, we're allocating
    0x80000 bytes (524 KiB) to our kernel stack. This should be PLENTY of space. The reason
    we add the memory is because the stack grows from higher memory to lower memory (bottom to top).
    Therefore we set the stack at the very bottom of its allocated slot.
    When we go to allocate from the stack, we'll subtract the number of bytes we need.
  */
  PROVIDE(_stack_start = _bss_end);
  PROVIDE(_stack_end = _stack_start + 0x8000);
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));

  /* 
    Finally, our heap starts right after the kernel stack. This heap will be used mainly
    to dole out memory for user-space applications. However, in some circumstances, it will
    be used for kernel memory as well.

    We don't align here because we let the kernel determine how it wants to do this.
  */
  PROVIDE(_heap_start = _stack_end);
  PROVIDE(_heap_size = _memory_end - _heap_start);
}
