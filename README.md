# Ext2 FileSystem Parser

## Overview
Developing a comprehensive program in C++ that can parse, read, modify, and manage custom-made ext2 filesystem images in binary format concurrently.

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

## Goals
- Read Core Structures: Parse the filesystem image to extract and display the contents of the superblock and block group descriptors.
- Traverse Directories: Starting from the root directory, your program must be able to recursively traverse all subdirectories, printing the complete layout of the filesystem.
- Read File Contents: For any given file in the filesystem, your program should be able to locate its data blocks and display its full contents.
- Update Existing Files: Implement functionality to append data to or overwrite the contents of existing files, handling block allocation as needed.

## References
-Ref 1: https://wiki.osdev.org/Ext2#Locating_the_Superblock

