# Ext2 FileSystem Parser

## Overview
Developing a comprehensive program in C++ that can parse, read, modify, and manage custom-made ext2 filesystem images in binary format concurrently.

## Task 1: Read Core Structures

### 1. Reading the superblock
The ext2 superblock begins at a byte offset of 1024 bytes, and has a length of 1024 bytes. (ref.1)

The program does the program flow:
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
- Blocks per group
- Inodes per group
- Filesystem signature
- Filesystem state
- Creator OS ID
- First non reserved inode
<img width="610" height="438" alt="image" src="https://github.com/user-attachments/assets/1198934b-40a8-4d14-bc96-6c6fdd6bf386" />

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
<img width="570" height="914" alt="image" src="https://github.com/user-attachments/assets/1d43afa8-0c10-4e32-af40-5632ea60f6d1" />

## Goals
- Read Core Structures: Parse the filesystem image to extract and display the contents of the superblock and block group descriptors.
- Traverse Directories: Starting from the root directory, your program must be able to recursively traverse all subdirectories, printing the complete layout of the filesystem.
- Read File Contents: For any given file in the filesystem, your program should be able to locate its data blocks and display its full contents.
- Update Existing Files: Implement functionality to append data to or overwrite the contents of existing files, handling block allocation as needed.

# References
-Ref 1: https://wiki.osdev.org/Ext2#Locating_the_Superblock

