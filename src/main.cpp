#include <iostream>
#include <cstdint>
#include <fstream>
#include <cerrno>
#include <string.h>
using namespace std;

uint32_t calculate_bytes(char* byte_start,int no_of_bytes){
	uint32_t result = 0;

	for(int i = 0;i<no_of_bytes;i++){
		result += (((unsigned int)(unsigned char)byte_start[i])<<(8*i));
	}

	return result;
}

int main(){

	ifstream file("/home/shivanshu_muppana/disk_proj/disk-backpup.img",ios::binary);

	if(!file){
		cerr << "Failed: " << strerror(errno) << endl;
		return 1;
	}

	file.seekg(1024);

	char bing[1024];

	file.read(bing,1024);

	//PRINTING SUPERBLOCK
	uint32_t total_inodes = calculate_bytes(bing,4);
	uint32_t total_blocks = calculate_bytes(bing+4,4);
	uint32_t no_superuser_blocks = calculate_bytes(bing+8,4);
	uint32_t total_unalloc_blocks = calculate_bytes(bing+12,4);
	uint32_t total_unalloc_inodes = calculate_bytes(bing+16,4);
	uint32_t superblock_no = calculate_bytes(bing+20,4);
	uint32_t block_size = 1024 << calculate_bytes(bing+24,4);
	uint32_t blocks_per_group = calculate_bytes(bing+32,4);
	uint32_t inodes_per_group = calculate_bytes(bing+40,4);
	uint32_t signature = calculate_bytes(bing+56,2);
	uint32_t filesystem_state = calculate_bytes(bing+58,2);
	uint32_t creator_os_id = calculate_bytes(bing+72,4);
	uint32_t first_non_reserved_inode = calculate_bytes(bing+84,4);

	cout << "Total inodes: " << total_inodes << endl;
	cout << "Total blocks: " << total_blocks << endl;
	cout << "Reserved blocks: " << no_superuser_blocks << endl;
	cout << "Free blocks: " << total_unalloc_blocks << endl;
	cout << "Free inodes: " << total_unalloc_inodes << endl;
	cout << "First data block: " << superblock_no << endl;
	cout << "Block size: " << block_size << " bytes" << endl;
	cout << "Blocks per group: " << blocks_per_group << endl;
	cout << "Inodes per group: " << inodes_per_group << endl;
	cout << "Signature: 0x" << hex << signature << dec << endl;
	cout << "Filesystem state: " << filesystem_state << endl;
	cout << "Creator OS ID: " << creator_os_id << endl;
	cout << "First non-reserved inode: " << first_non_reserved_inode << endl;


}
