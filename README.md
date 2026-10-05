# Ext2 FileSystem Parser

## Overview
Developing a comprehensive program in C++ that can parse, read, modify, and manage custom-made ext2 filesystem images in binary format.

## Task 1: Read Core Structures

### 1. Reading the superblock
The ext2 superblock begins at a byte offset of 1024 bytes, and has a length of 1024 bytes. (ref.1)

The program has the following program flow:
- Opens the filesystem image in binary mode.
- Seeks to byte offset 1024
- Reads the 1024-byte superblock into a buffer.
- Extracts the individual fields in bytes
- Converts from little-endian byte sequences into integer values
- Stores parsed values in a 'Superblock' structure.

Used calculate_bytes() to convert little-endian byte sequences into integer values.
<img width="1182" height="268" alt="image" src="https://github.com/user-attachments/assets/24c25c39-76f7-46bd-8cfc-8a02329f768c" />

### 2. Superblock information
Parsed the following fields:
- Total inodes
- Total blocks
- Total unallocated blocks
- Total unallocated inodes
- First data block
- Block size
- Inode size
- Blocks per group
- Inodes per group
- Filesystem signature
- Filesystem state
- Creator OS ID
- First non reserved inode

### 3. Block group descriptor table
The block group descriptor table is located immediately after the superblock. (ref.1)
The number of block groups is calculated using: 

                No. of block groups = ceil(total no of blocks/blocks per group)

Each group descriptor is 32 bytes.

The program reads the group descriptor table and iterates over each descriptor, extracting:
- Block bitmap location
- Inode bitmap location
- Inode table location
- Number of free blocks
- Number of free inodes
- Number of used directories

### Output
<img width="536" height="958" alt="image" src="https://github.com/user-attachments/assets/91c7ece3-6a53-4c3b-b482-bbd59272d7d4" />

## Task 2: Traverse Directories
The directory traversal has the following program flow:
- Read the inode corresponding to a directory, using an `inode` structure, containing the following:
    - mode
    - size
    - blocks
    - an array of block pointers (major component for task 2)
- Extract its block pointers.
- Process the directory data blocks pointed to by the direct pointers.
- Parse directory entries in each data block to obtain:
    - Inode number
    - Record length
    - Filename length
    - File type
    - File name
- Recursively traverse directory entries whose File type is `Directory`.
- Recursively handle indirect block pointers.
- Use indentation based on the recursion depth to display filesystem hierarchy.

### Directory Entry Handling
- Entries `.` , `..` are not printed or recursively traversed.
- Entries with inode number 0 are skipped.
- Record length of 0 stops the processing of the current directory block.
- Variable length file names are handled with filename length field.
  
### Block Pointer Handling
Supports all ext2 inode block pointer levels (ref.1):
- 12 direct block pointers
- Single indirect pointer
- Double indirect pointer
- Triple indirect pointer

Recursively handles indirect block pointers by following the indirect block pointers to their blocks, until actual data blocks containing directory entries are obtained.
- Uses the following implementation:
    - traverse_directory() : reads an inode and determines its data blocks.
    - process_directory_block() : parses directory entries within a particular data block.
    - process_indirect_block() : recursively handles indirect block pointers.

### Output
Output snippet:

<img width="452" height="1568" alt="image" src="https://github.com/user-attachments/assets/02d9957b-45a1-4bf0-b24e-0308cc61c498" />

## Task 3: Locate & Read files in the current working directory
Navigates the filesystem using a current working directory.

### Current working directory 
- Maintained using `current_inode` and `current_path`.
- Root directory starts at inode 2.
- The following commands are supported:
    - cd <directory> : changes directory
    - cd .. : move to parent directory (if exists)
    - cd . : stay in the current directory
    - pwd : print working directory

### Directory Searching
A generic search mechanism was implemented to locate files and directories within the current directory.
It follows the following program flow:
- `search()` obtains the inode of the current directory and searches all of its data blocks.
- `search_indirect_block()` recursively handles indirect block pointers in union with `search_block()` to parse through all directory entries of current directory. 

### Reading File Contents
Reading a file has the following syntax :

                  read <filename>

It has the following program flow:
- Search the current directory for requested file.
- Once located, file's inode is read.
- Physical data blocks are obtained from the inode's direct or indirect block pointeres.
- The contents of each data block are read from the image.
- Only the number of bytes specified by the inode's file size are printed.

### Output
<img width="582" height="84" alt="image" src="https://github.com/user-attachments/assets/228cf364-5607-4ae2-a840-30c449eadb97" />

## Task 4: Updating existing files
Modifies the contents of existing regular files with the following syntax.

                  write <filename> <input_file>
                  append <filename> <input_file>

### Overwriting a file
It has the following program flow:
- Locates the file's inode.
- Determines the physical data blocks currently allocated to the file.
- Reads the contents of the input file.
- Calculates the number of blocks required for the new file size.
- Compares the required number of blocks with the currently allocated blocks.
- Handles the following cases:
    - Same number of blocks : Existing blocks are reused
    - More blocks required : Additional blocks are allocated
    - Fewer blocks required : Excess data blocks are freed.
- The file's data blocks are then overwritten with the new contents and the inode's fields are updated.

### Appending to a file
It has the following porgram flow:
- Determines the amount of unused space remaining in the file's final data block.
- If the appended data fits within existing blocks, no new blocks are allocated.
- If the appended data exceeds the remaining space:
    - The remaining space in the current final block is filled.
    - Additional blocks are allocated.
    - The remaining data is written to the newly allocated blocks.
    - The inode's fields are updated.

### Block Allocation
Allocation process is as follows: 
- Determines the block group associated with the file's inode.
- Reads the group's block bitmap.
- Searches for free blocks.
- Marks selected blocks as allocated in an in-memory allocation state.

### Block Deallocation (Freeing)
When an overwrite results in a fewer amount of blocks being used, unused blocks are identified and marked free in the in-memory bitmap.
However, the code currently has no functionality to deallocate metadata blocks for indirect block pointers, leading to block leaks. This will be improved upon in the next commit.

### Committing Allocation changes
After the file data and inode fields have been updated, the in-memory allocation state is committed to the filesystem image.
The following structures are updated:
- Block bitmap
- Block group descriptor
- Superblock free block count

### Output
The following files were made for testing task 4 implementation
`python3 -c "open('big.txt','wb').write(b'A'*1100)"`
`python3 -c "open('appendbig.txt','wb').write(b'B'*1100)"`
The output reflects a clear change in the blocks allocated in the image based on the input.

<img width="472" height="1432" alt="image" src="https://github.com/user-attachments/assets/f889f7eb-06be-4dd0-8342-c2defa0d23cc" />

## Goals
- Read Core Structures: Parse the filesystem image to extract and display the contents of the superblock and block group descriptors.
- Traverse Directories: Starting from the root directory, program must be able to recursively traverse all subdirectories, printing the complete layout of the filesystem.
- Read File Contents: For any given file in the filesystem, program should be able to locate its data blocks and display its full contents.
- Update Existing Files: Implement functionality to append data to or overwrite the contents of existing files, handling block allocation as needed.

## References
- Ref 1: https://wiki.osdev.org/Ext2#Locating_the_Superblock | Task related information
- Operating Systems: Three Easy Pieces | Theory for operating systems
- Missing Semester: Playlist on youtube | Linux & Git commands
