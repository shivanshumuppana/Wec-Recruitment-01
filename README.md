# Developing a comprehensive program in C++ that can parse, read, modify, and manage custom-made ext2 filesystem images in binary format concurrently.

## Goals
- Read Core Structures: Parse the filesystem image to extract and display the contents of the superblock and block group descriptors.
- Traverse Directories: Starting from the root directory, your program must be able to recursively traverse all subdirectories, printing the complete layout of the filesystem.
- Read File Contents: For any given file in the filesystem, your program should be able to locate its data blocks and display its full contents.
- Update Existing Files: Implement functionality to append data to or overwrite the contents of existing files, handling block allocation as needed.

