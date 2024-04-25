# About XV6
xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern x86-based multiprocessor using ANSI C.

## ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.828/, which
provides pointers to on-line resources for v6.

xv6 borrows code from the following sources:
    JOS (asm.h, elf.h, mmu.h, bootasm.S, ide.c, console.c, and others)
    Plan 9 (entryother.S, mp.h, mp.c, lapic.c)
    FreeBSD (ioapic.c)
    NetBSD (console.c)

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by Silas
Boyd-Wickizer, Anton Burtsev, Cody Cutler, Mike CAT, Tej Chajed, eyalz800,
Nelson Elhage, Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, Peter
Froehlich, Yakir Goaron,Shivam Handa, Bryan Henry, Jim Huang, Alexander
Kapshuk, Anders Kaseorg, kehao95, Wolfgang Keller, Eddie Kohler, Austin
Liew, Imbar Marinescu, Yandong Mao, Matan Shabtay, Hitoshi Mitake, Carmi
Merimovich, Mark Morrissey, mtasm, Joel Nider, Greg Price, Ayan Shafqat,
Eldar Sehayek, Yongming Shen, Cam Tenny, tyfkda, Rafael Ubal, Warren
Toomey, Stephen Tu, Pablo Ventura, Xi Wang, Keiichi Watanabe, Nicolas
Wolovick, wxdao, Grant Wu, Jindong Zhang, Icenowy Zheng, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2018 Frans Kaashoek, Robert Morris, and Russ Cox.

## ERROR REPORTS

We switched our focus to xv6 on RISC-V; see the mit-pdos/xv6-riscv.git
repository on github.com.

## BUILDING AND RUNNING XV6

To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
"make". On non-x86 or non-ELF machines (like OS X, even on x86), you
will need to install a cross-compiler gcc suite capable of producing
x86 ELF binaries (see https://pdos.csail.mit.edu/6.828/).
Then run "make TOOLPREFIX=i386-jos-elf-". Now install the QEMU PC
simulator and run "make qemu".

# XV6 Shared Memory

## Objective
The goal of this project is to modify the xv6 OS to support memory sharing between processes, limited to processes that are in a parent-child relationship. 
The parent process should report to the operating system the memory that is shared, and all immediate children of that process can receive access from the operating system
in their address space to that very memory located in the parent process. 
In addition to these changes, several user programs that use these system changes are implemented.
This new functionality does not disrupt the current operation of the system. Processes can be started and finished normally, and that all occupied memory is properly freed.

## System Calls
Two new system calls are implemented: <code>share_data</code> and <code>get_data</code>. Both system calls work with a new shared structure that has the following attributes:
* <b>String name</b>, maximum 10 characters
* <b>Pointer to start of shared space</b>
* <b>Size of shared space</b>

### <code> int share_data(char *name, void *addr, int size);</code>
This system call is called from the parent process to register the shared structure. When declaring a shared structure, specify a unique string name for the structure 
(if the given string is longer than 10 characters, take the first 10), the address where the structure is located, and the size of the
structure. <code>proc</code> structure is modified so that it stores up to 10 shared structures. Unused shared structures are marked with size 0 in the newly created process. 
In case of successful writing of a new shared structure, this system call returns its serial number within the process (0-9). Possible errors for this system call are:
* -1: bad parameter passed.
* -2: there already exists a shared structure with the given name.
* -3: there are already 10 shared structures in the current process.

### <code> int get_data(char *name, void **addr);</code>
This system call is called from the child process to access the shared structure previously declared by the parent using the <code>share_data</code> system call. 
The first parameter is the string name of the structure we want to access, and the second parameter is a pointer (passed by reference) that points to the shared space 
after the system call is executed. The child process already has a prepared shared structure from which it just reads the value for the <code>addr</code> pointer. 
In case of successful execution of this system call, the return value is 0. Possible errors for this system call are:
* -1: bad parameter passed.
* -2: there is no shared structure with the given name.

## System Changes 
Three primary changes in the system are made: <code>fork()</code>, <code>exec()</code> and process shutdown. 
### <code>fork()</code>
This system call ensures that the newly created process inherits all shared structures from its parent. Since the memory image is identical in the child, there is no need to 
change anything within the structures themselves or the side structures of the child. Until it does <code>exec()</code>, the child process cannot access the parent's shared objects, 
but only has access to its own copy. In addition, a pointer to the parent's page directory is stored in the child's <code>proc</code> structure using a new attribute specifically 
intended for this. 
### <code> exec() </code> 
The regular process size in virtual memory is limited to 1GB, and starting from 1GB shared objects are being mapped. This system call is modified so that the newly started process 
in its virtual space, starting from 1GB, maps all the pages listed in the shared structures inherited from the parent. 
These mappings point to the same frames as in the parent directory. After the mapping is done, the shared structures are being modified, so that the virtual addresses 
are now valid taking into account the new mapping. 
### Process shutdown 
It is necessary that both the parent and all child processes can be successfully completed, as well as that all occupied memory is freed when they are shut down.

## User programs
Total of three user programs are implemented - one parent (<code>dalle</code>), which starts the other two (<code>coMMa</code> and <code>liSa</code>) as part of its work.

### <code>dalle</code>
This user program runs an interactive system for analyzing text files. It receives the path to the file as an argument, or if there is none, the default value is "../home/README". 
The program prepares the following structures to share with their children:
* <b>Path to the file we are processing</b> - <code>char *</code>
* <b>The counter to which sentence we reached within the file</b> - <code>int</code>
* <b>The longest word in the current sentence</b> - <code>char *</code>
* <b>Size of the longest word in the entire text up to the current sentence</b> (including it) - <code>int</code>
* <b>The shortest word in the current sentence</b> - <code>char *</code>
* <b>Size of the shortest word in the entire text up to the current sentence</b> (with it) - <code>int</code>
* <b>The longest word in the entire text up to the current sentence</b> (including it) - <code>char *</code>
* <b>Command indicator</b> - <code>int</code>
* <b> The shortest word in the entire text up to the current sentence</b> (including it) - <code>char *</code>
After creating nine shared structures for this data, two new processes are started as children: <code>coMMa</code> and <code>liSa</code>.
After starting the children, <code>dalle</code> just waits for the completion of both processes and then terminates itself.

### <code>coMMa</code>
This user program is tasked with accepting commands from the user in a loop. Possible commands that the user can enter are:
* latest - prints which sentence we are processing in order, as well as the longest and shortest word in that sentence
* <code>global extrema</code> - prints the largest and smallest word found so far and their size
* <code>pause</code> - pauses text processing.
* <code>resume</code> - resumes text processing.
* <code>end</code> - notifies the process that it should shut down, and shuts down itself.
The <code>global extrema</code> and <code>latest</code> commands directly read from shared structures in order to write the specified data.
The <code>pause</code>, <code>resume</code>, and <code>end</code> commands send signals to the <code>liSa</code> user program using a shared command indicator.

### <code>liSa</code>
This user program is tasked with processing a text file. Upon opening the file, it finds the shortest and longest word in the current sentence (as well as globally) 
and places them in the appropriate shared structure, then increments the shared counter of the current sentence and calls <code>sleep(150)</code> to simulate data analysis. 
After processing each sentence, this process should check if <code>coMMa</code> specified a command. In case a pause command is specified, 
<code>sleep(1)</code> is executed, and then the program check for the command again. Only when <code>resume</code> or <code>end</code> command is specified, the program 
continues to work or ends.
