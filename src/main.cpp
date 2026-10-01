#include <iostream>
#include <cstdint>
#include <fstream>
#include <cerrno>
#include <string.h>
#include <cmath>
using namespace std;

struct Superblock{
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t reserved_blocks;
    uint32_t total_unalloc_blocks;
    uint32_t total_unalloc_inodes;
    uint32_t first_data_block;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint16_t signature;
    uint16_t filesystem_state;
    uint32_t creator_os_id;
    uint32_t first_non_reserved_inode;
};

struct group_descriptor{
	uint32_t block_bitmap;
	uint32_t inode_bitmap;
	uint32_t inode_table;
	uint32_t free_blocks;
	uint32_t free_inodes;
	uint32_t used_dirs;
};

struct inode{
    uint16_t mode;
    uint32_t size;
    uint32_t blocks;
    uint32_t block_pointers[15];
};

uint32_t calculate_bytes(char* byte_start,int no_of_bytes){
	uint32_t result = 0;

	for(int i = 0;i<no_of_bytes;i++){
		result += (((unsigned int)(unsigned char)byte_start[i])<<(8*i));
	}

	return result;
}

group_descriptor read_group_descriptor(char* byte_start){

	group_descriptor gd;

	gd.block_bitmap = calculate_bytes(byte_start, 4);
	gd.inode_bitmap = calculate_bytes(byte_start + 4, 4);
	gd.inode_table = calculate_bytes(byte_start + 8, 4);
	gd.free_blocks = calculate_bytes(byte_start + 12, 2);
	gd.free_inodes = calculate_bytes(byte_start + 14, 2);
	gd.used_dirs = calculate_bytes(byte_start + 16, 2);

	return gd;
}

void display_group_descriptor(group_descriptor gd,int group_no){
	cout << "Block Group " << group_no << endl;
	cout << "Block bitmap: " << gd.block_bitmap << endl;
	cout << "Inode bitmap: " << gd.inode_bitmap << endl;
	cout << "Inode table: " << gd.inode_table << endl;
	cout << "Free blocks: " << gd.free_blocks << endl;
	cout << "Free inodes: " << gd.free_inodes << endl;
	cout << "Used directories: " << gd.used_dirs << endl;
	cout << endl;
}

void display_group_table(char* byte_start,Superblock sb){

	cout << "Group Table: " << endl;
	int no_of_groups = ceil((float)sb.total_blocks/sb.blocks_per_group);
	for(int i = 0;i<no_of_groups;i++){
		display_group_descriptor(read_group_descriptor(byte_start+(32*i)),i);
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
	sb.inode_size = calculate_bytes(byte_start+88,2);
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
	cout << "Inode size: " << sb.inode_size << " bytes" << endl;
	cout << "Blocks per group: " << sb.blocks_per_group << endl;
	cout << "Inodes per group: " << sb.inodes_per_group << endl;
	cout << "Signature: 0x" << hex << sb.signature << dec << endl;
	cout << "Filesystem state: " << sb.filesystem_state << endl;
	cout << "Creator OS ID: " << sb.creator_os_id << endl;
	cout << "First non-reserved inode: " << sb.first_non_reserved_inode << endl;
	cout << endl;
	return sb;
}

int display_inode(int inode_no,Superblock sb){

	ifstream file("/home/shivanshu_muppana/disk_proj/disk-backpup.img",ios::binary);

	//if error
	if(!file){
		cerr << "Failed: " << strerror(errno) << endl;
		return 1;
	}

	file.seekg(2048);
	char group_table[1024];
	file.read(group_table,1024);

	int block_group = ((inode_no-1)/sb.inodes_per_group);
	group_descriptor gd = read_group_descriptor(group_table+(32*block_group));

	int index = ((inode_no-1)%sb.inodes_per_group);
	uint32_t byte_shift = (index*(sb.inode_size)) + (gd.inode_table)*(sb.block_size);

	file.seekg(byte_shift);
	char inode_buffer[sb.inode_size];
	file.read(inode_buffer,sb.inode_size);

	inode root;
	root.mode = calculate_bytes(inode_buffer, 2);
	root.size = calculate_bytes(inode_buffer + 4, 4);
	root.blocks = calculate_bytes(inode_buffer + 28, 4);

	for (int i = 0;i<15;i++) {
    		root.block_pointers[i] = calculate_bytes(inode_buffer + 40 + i * 4, 4);
	}

	for(int i = 0;i<12;i++){
		if(root.block_pointers[i]==0){
			break;
		}
		uint32_t offset = 0;
		file.seekg(root.block_pointers[i]*sb.block_size);
		char direc_buffer[sb.block_size];
		file.read(direc_buffer,sb.block_size);

		while(offset<sb.block_size){

			int name_length = calculate_bytes(direc_buffer+6+offset,1);
			int entry_length = calculate_bytes(direc_buffer+4+offset,2);
			int type = calculate_bytes(direc_buffer+7+offset,1);
			int subdirec_inode_no = calculate_bytes(direc_buffer+0+offset,4);
			char name[name_length+1];

			cout << "Inode: " << subdirec_inode_no << endl;
			cout <<	"Total size of entry: " << entry_length << endl;
			cout << "Name length: " << name_length << endl;
			cout << "Type: " << type << endl;
			for(int j = 0;j<name_length;j++){
				name[j] = calculate_bytes(direc_buffer+8+offset+j,1);
			}
			name[name_length] = '\0';
			cout << "Name: " << name << endl;
			cout << endl;

			/*
			bool flag = false;
			if((strcmp(name,"..") && strcmp(name,".")) && (type==2)){
				cout << "Entering subdirectory: " << endl;
				cout << "Due to: " << name << endl;
				cout << "Having inode no: " << subdirec_inode_no << endl;
				cout << "With parent inode no: " << inode_no << endl;
				flag = true;
				display_inode(subdirec_inode_no,sb);
			}
			if(flag){
				cout << "Exiting subdir" << endl;
			}
			*/

			offset += entry_length;
		}
	}

	return 0;
}


int read_core_structures(ifstream& file){

	//print superblock
	file.seekg(1024);
	char bing[1024];
	file.read(bing,1024);

	Superblock sb;
	sb = read_superblock(bing);

	//print group descriptor table
	file.seekg(2048);
	char group_table[1024];
	file.read(group_table,1024);

	display_group_table(group_table,sb);
	display_inode(2,sb);
	return 0;
}

int traverse_directories(){



	return 0;
}

int main(){

	ifstream file("/home/shivanshu_muppana/disk_proj/disk-backpup.img",ios::binary);

	//if error
	if(!file){
		cerr << "Failed: " << strerror(errno) << endl;
		return 1;
	}

	read_core_structures(file);

	return 0;
}

