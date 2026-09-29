#include <iostream>
#include <cstdint>
#include <fstream>
#include <cerrno>
#include <string.h>
#include <cmath>
using namespace std;

struct Superblock {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t reserved_blocks;
    uint32_t total_unalloc_blocks;
    uint32_t total_unalloc_inodes;
    uint32_t first_data_block;
    uint32_t block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint16_t signature;
    uint16_t filesystem_state;
    uint32_t creator_os_id;
    uint32_t first_non_reserved_inode;
};

uint32_t calculate_bytes(char* byte_start,int no_of_bytes){
	uint32_t result = 0;

	for(int i = 0;i<no_of_bytes;i++){
		result += (((unsigned int)(unsigned char)byte_start[i])<<(8*i));
	}

	return result;
}

void display_group_descriptor(char* byte_start,int group_no){

	uint32_t block_bitmap = calculate_bytes(byte_start, 4);
	uint32_t inode_bitmap = calculate_bytes(byte_start + 4, 4);
	uint32_t inode_table = calculate_bytes(byte_start + 8, 4);
	uint32_t free_blocks = calculate_bytes(byte_start + 12, 2);
	uint32_t free_inodes = calculate_bytes(byte_start + 14, 2);
	uint32_t used_dirs = calculate_bytes(byte_start + 16, 2);

	cout << "Block Group " << group_no << endl;
	cout << "Block bitmap: " << block_bitmap << endl;
	cout << "Inode bitmap: " << inode_bitmap << endl;
	cout << "Inode table: " << inode_table << endl;
	cout << "Free blocks: " << free_blocks << endl;
	cout << "Free inodes: " << free_inodes << endl;
	cout << "Used directories: " << used_dirs << endl;
	cout << endl;

}

void display_group_table(char* byte_start,Superblock sb){

	cout << "Group Table: " << endl;
	int no_of_groups = ceil((float)sb.total_blocks/sb.blocks_per_group);
	for(int i = 0;i<no_of_groups;i++){
		display_group_descriptor(byte_start+(32*i),i);
	}

}

Superblock read_superblock(char* byte_start){

	Superblock sb;

	sb.total_inodes = calculate_bytes(byte_start,4);
	sb.total_blocks = calculate_bytes(byte_start+4,4);
	sb.reserved_blocks = calculate_bytes(byte_start+8,4);
	sb.total_unalloc_blocks = calculate_bytes(byte_start+12,4);
	sb.total_unalloc_inodes = calculate_bytes(byte_start+16,4);
	sb.first_data_block = calculate_bytes(byte_start+20,4);
	sb.block_size = 1024 << calculate_bytes(byte_start+24,4);
	sb.blocks_per_group = calculate_bytes(byte_start+32,4);
	sb.inodes_per_group = calculate_bytes(byte_start+40,4);
	sb.signature = calculate_bytes(byte_start+56,2);
	sb.filesystem_state = calculate_bytes(byte_start+58,2);
	sb.creator_os_id = calculate_bytes(byte_start+72,4);
	sb.first_non_reserved_inode = calculate_bytes(byte_start+84,4);


	cout << endl << "Superblock Content: " << endl;
	cout << "Total inodes: " << sb.total_inodes << endl;
	cout << "Total blocks: " << sb.total_blocks << endl;
	cout << "Reserved blocks: " << sb.reserved_blocks << endl;
	cout << "Free blocks: " << sb.total_unalloc_blocks << endl;
	cout << "Free inodes: " << sb.total_unalloc_inodes << endl;
	cout << "First data block: " << sb.first_data_block << endl;
	cout << "Block size: " << sb.block_size << " bytes" << endl;
	cout << "Blocks per group: " << sb.blocks_per_group << endl;
	cout << "Inodes per group: " << sb.inodes_per_group << endl;
	cout << "Signature: 0x" << hex << sb.signature << dec << endl;
	cout << "Filesystem state: " << sb.filesystem_state << endl;
	cout << "Creator OS ID: " << sb.creator_os_id << endl;
	cout << "First non-reserved inode: " << sb.first_non_reserved_inode << endl;
	cout << endl;
	return sb;
}

int main(){

	ifstream file("/home/shivanshu_muppana/disk_proj/disk-backpup.img",ios::binary);

	//if error
	if(!file){
		cerr << "Failed: " << strerror(errno) << endl;
		return 1;
	}

	//print superblock
	file.seekg(1024);
	char bing[1024];
	file.read(bing,1024);

	Superblock sb;
	sb = read_superblock(bing);
	cout << endl;

	//print group descriptor table
	file.seekg(2048);
	char group_table[1024];
	file.read(group_table,1024);
	display_group_table(group_table,sb);

}
