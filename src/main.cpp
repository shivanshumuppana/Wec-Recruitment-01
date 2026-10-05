#include <iostream>
#include <cstdint>
#include <fstream>
#include <cerrno>
#include <string.h>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;


/* Structures and function signatures */
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

void process_directory_block(uint32_t block_number,Superblock sb,fstream& file,int depth);
int traverse_directory(uint32_t inode_no,Superblock sb,fstream& file,int depth);
uint32_t search_block(uint32_t block_number,Superblock sb,fstream& file,string key,int desired_type);
uint32_t search_indirect_block(uint32_t block_no,Superblock sb,fstream& file,int level,string key,int desired_type);
uint32_t search(uint32_t inode_no,Superblock sb,fstream& file,string key,int desired_type);
void fill_indirect(uint32_t &block_no,int level,vector<uint32_t>& new_blocks,int& j,Superblock sb, fstream& file,uint32_t inode_no);
vector<uint32_t> allocate_blocks(int no_of_blocks,uint32_t inode_no,Superblock sb,fstream& file);
vector<uint32_t> get_data_blocks(inode target,Superblock sb,fstream& file);
void write_inode(uint32_t inode_no,vector<uint32_t> final_data_blocks,uint32_t new_size,Superblock sb,fstream& file);
void write_data(vector<char>& data,vector<uint32_t>& data_blocks,Superblock sb,fstream& file);

/* Basic functions */
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

Superblock process_superblock(char* byte_start){

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

	return sb;
}

void display_superblock(Superblock sb){
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
}

inode process_inode(uint32_t inode_no,Superblock sb,fstream& file){

	file.seekg(2048);
	char group_table[sb.block_size];
	file.read(group_table,sb.block_size);

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

	return root;

}

/* Traversing */
void process_directory_block(uint32_t block_number,Superblock sb,fstream& file,int depth){

	if(block_number==0){
		return;
	}
	uint32_t offset = 0;
	file.seekg(block_number*sb.block_size);
	char direc_buffer[sb.block_size];
	file.read(direc_buffer,sb.block_size);

	while(offset<sb.block_size){

		int name_length = calculate_bytes(direc_buffer+6+offset,1);
		int entry_length = calculate_bytes(direc_buffer+4+offset,2);
		int type = calculate_bytes(direc_buffer+7+offset,1);
		int subdirec_inode_no = calculate_bytes(direc_buffer+0+offset,4);
		if(entry_length < 8){
			break;
		}
		char name[name_length+1];

		//handling unused entries and the double/single dot entries
		//if(entry_length==0){
		//	break;
		//}
		if(subdirec_inode_no==0){
			offset += entry_length;
			continue;
		}

		for(int j = 0;j<name_length;j++){
			name[j] = calculate_bytes(direc_buffer+8+offset+j,1);
		}
		name[name_length] = '\0';

		if (strcmp(name,"..")==0 || strcmp(name,".")==0){
			offset += entry_length;
			continue;
		}

		// Printing inode
		for(int j = 0;j<depth;j++){
			cout << "	";
		}
		cout << "Inode: " << subdirec_inode_no << endl;
		// Printing Name
		for(int j = 0;j<depth;j++){
			cout << "	";
		}
		cout << "Name: " << name << endl;

		// Printing type
		for(int j = 0;j<depth;j++){
			cout << "	";
		}
		if(type==0){
			cout << "Type: Unknown" << endl;
		}
		else if(type==1){
			cout << "Type: File" << endl;
		}
		else if(type==2){
			cout << "Type: Directory" << endl;
		}
		cout << endl;


		// Recursive call
		if(type==2){
			traverse_directory(subdirec_inode_no,sb,file,depth+1);
		}

		offset += entry_length;
	}

	return;

}

void process_indirect_block(uint32_t block_no,Superblock sb,fstream& file,int depth,int level){

	if(block_no==0){
		return;
	}

	file.seekg(block_no*sb.block_size);
	char buffer[sb.block_size];
	file.read(buffer,sb.block_size);

	for(int i = 0;i<(sb.block_size/4);i++){
		uint32_t indirect_block_no = calculate_bytes(buffer+(4*i),4);
		if(level == 1){
			process_directory_block(indirect_block_no,sb,file,depth);
		}
		else{
			process_indirect_block(indirect_block_no,sb,file,depth,level-1);
		}
	}

}

int traverse_directory(uint32_t inode_no,Superblock sb,fstream& file,int depth){

	inode root = process_inode(inode_no,sb,file);

	//direct pointers
	for(int i = 0;i<12;i++){
		process_directory_block(root.block_pointers[i],sb,file,depth);
	}

	/* indirect pointers */
	//single indirect pointer
	if(root.block_pointers[12]!=0){
		process_indirect_block(root.block_pointers[12],sb,file,depth,1);
	}
	//double indirect pointer
	if(root.block_pointers[13]!=0){
		process_indirect_block(root.block_pointers[13],sb,file,depth,2);
	}
	//triple indirect pointer
	if(root.block_pointers[14]!=0){
		process_indirect_block(root.block_pointers[14],sb,file,depth,3);
	}

	return 0;
}


int read_core_structures(Superblock sb,fstream& file){

	//print superblock
	display_superblock(sb);

	//print group descriptor table
	file.seekg(2048);
	char group_table[sb.block_size];
	file.read(group_table,sb.block_size);

	display_group_table(group_table,sb);

	return 0;
}


/* Locating & Reading a particular file's content */
uint32_t search_block(uint32_t block_number,Superblock sb,fstream& file,string key,int desired_type){

    if(block_number == 0){
        return 0;
    }

    uint32_t offset = 0;

    file.seekg(block_number * sb.block_size);

    char direc_buffer[sb.block_size];

    file.read(direc_buffer, sb.block_size);

    while(offset < sb.block_size){

        int name_length = calculate_bytes(direc_buffer + 6 + offset, 1);

        int entry_length = calculate_bytes(direc_buffer + 4 + offset, 2);

        int type = calculate_bytes(direc_buffer + 7 + offset, 1);

        uint32_t inode_no = calculate_bytes(direc_buffer + offset, 4);

	// corrupt/zero rec_len would loop forever
        if(entry_length < 8 || offset + entry_length > sb.block_size){
            break;
        }

        if(type != desired_type || inode_no == 0){
            offset += entry_length;
            continue;
        }

        string name(direc_buffer + 8 + offset,name_length);

        if(name == key){
            return inode_no;
        }

        offset += entry_length;
    }

    return 0;
}

uint32_t search(uint32_t cwd_inode_no,Superblock sb,fstream& file,string key,int desired_type){

    inode cwd_inode = process_inode(cwd_inode_no, sb, file);

    uint32_t result;

    // Direct blocks
    for(int i = 0; i < 12; i++){

        result = search_block(cwd_inode.block_pointers[i],sb,file,key,desired_type);

        if(result != 0){
            return result;
        }
    }

    // Single indirect
    if(cwd_inode.block_pointers[12] != 0){

        result = search_indirect_block(cwd_inode.block_pointers[12],sb,file,1,key,desired_type);

        if(result != 0){
            return result;
        }
    }

    // Double indirect
    if(cwd_inode.block_pointers[13] != 0){

        result = search_indirect_block(cwd_inode.block_pointers[13],sb,file,2,key,desired_type);

        if(result != 0){
            return result;
        }
    }

    // Triple indirect
    if(cwd_inode.block_pointers[14] != 0){

        result = search_indirect_block(cwd_inode.block_pointers[14],sb,file,3,key,desired_type);

        if(result != 0){
            return result;
        }
    }

    return 0;
}

uint32_t search_indirect_block(uint32_t block_no,Superblock sb,fstream& file,int level,string key,int desired_type){

    if(block_no == 0){
        return 0;
    }

    file.seekg(block_no * sb.block_size);

    char buffer[sb.block_size];

    file.read(buffer, sb.block_size);

    for(int i = 0; i<(sb.block_size/4); i++){

        uint32_t next_block = calculate_bytes(buffer + 4 * i, 4);

        if(next_block == 0){
            continue;
        }

        uint32_t result;

        if(level == 1){
            result = search_block(next_block,sb,file,key,desired_type);

        }
        else{

            result = search_indirect_block(next_block,sb,file,level - 1,key,desired_type);
        }

        if(result != 0){
            return result;
        }
    }

    return 0;
}

uint32_t get_indirect_block(uint32_t block_no,Superblock sb,fstream& file,uint32_t entry_capacity,int level,uint32_t index){

	if(block_no==0){
		return 0;
	}

	uint32_t span = 1;
	for(int i = 1;i<level;i++){
		span *= entry_capacity;
	}

	file.seekg(block_no*sb.block_size);
	char buffer[sb.block_size];
	file.read(buffer,sb.block_size);

	uint32_t next = calculate_bytes(buffer+4*(index/span),4);

	if(level==1){
		return next;
	}
	return get_indirect_block(next,sb,file,entry_capacity,level-1,index%span);
}

vector<uint32_t> get_data_blocks(inode target,Superblock sb,fstream& file){
	vector<uint32_t> data_blocks;

	int no_of_blocks = ceil(((float)target.size)/(sb.block_size));
	uint32_t entry_capacity = sb.block_size/4;

	for(int i = 0;i<no_of_blocks;i++){
		uint32_t block_no = 0;
		if(i<12){
			block_no = target.block_pointers[i];
		}
		else if(i<12+entry_capacity){
			int index = i-12;
			block_no = get_indirect_block(target.block_pointers[12],sb,file,entry_capacity,1,index);
		}
		else if(i<12+entry_capacity+(entry_capacity*entry_capacity)){
			int index = i - (12 + entry_capacity);
			block_no = get_indirect_block(target.block_pointers[13],sb,file,entry_capacity,2,index);
		}
		else{
			int index = i - (12+entry_capacity+entry_capacity*entry_capacity);
			block_no = get_indirect_block(target.block_pointers[14],sb,file,entry_capacity,3,index);
		}

		if(block_no!=0){
			data_blocks.push_back(block_no);
		}
	}

	return data_blocks;
}

void print_file(uint32_t cwd_inode_no,Superblock sb,fstream& file,string key){
	uint32_t key_inode_no = search(cwd_inode_no,sb,file,key,1);
	if(!key_inode_no){
		cout << "File not found!" << endl;
		return;
	}

	//go to file and process its blocks
	inode key_inode = process_inode(key_inode_no,sb,file);
	vector<uint32_t> data_blocks = get_data_blocks(key_inode,sb,file);

	for(uint32_t block_no : data_blocks){
		//write min(block_size, size - i*block_size) bytes (implementation of reading the content of file is yet to be done)
	}

}

/* Update existing files */
int count_indirect_blocks(int data_blocks,Superblock sb){
	int entry_capacity = sb.block_size/4;
	int extra_blocks = 0;

	if(data_blocks<=12){
		return 0;
	}

	int remaining = data_blocks - 12;
	extra_blocks++;

	if(remaining<=6){
		return extra_blocks;
	}

	remaining -= entry_capacity;
	extra_blocks++;

	extra_blocks += ceil((float)remaining/entry_capacity);

	if(remaining<=entry_capacity*entry_capacity){
		return extra_blocks;
	}

	remaining -= entry_capacity*entry_capacity;
	extra_blocks++;

	int level2 = ceil((float)remaining/(entry_capacity*entry_capacity));
	extra_blocks += level2;

	extra_blocks += ceil((float)remaining/entry_capacity);

	return extra_blocks;
}

void write_inode(uint32_t inode_no,vector<uint32_t> final_data_blocks,uint32_t new_size,Superblock sb,fstream& file){
	int block_group = (inode_no-1)/sb.inodes_per_group;
	int index = (inode_no-1)%sb.inodes_per_group;

	file.seekg(2048);
	char group_table[sb.block_size];
	file.read(group_table,sb.block_size);
	group_descriptor gd = read_group_descriptor(group_table+(32*block_group));
	uint32_t byte_offset = gd.inode_table*sb.block_size + index*sb.inode_size;

	char inode_buffer[sb.inode_size];
	file.seekg(byte_offset);
	file.read(inode_buffer,sb.inode_size);

	//direct pointer
	int j = 0;
	for(int i = 0;i<12 && j<final_data_blocks.size();i++){
		uint32_t current_block = calculate_bytes(inode_buffer+40+i*4,4);
		if(current_block==0){
			uint32_t new_block = final_data_blocks[j++];

			for(int k = 0;k<4;k++){
				inode_buffer[40+i*4+k] = (new_block>>(8*k)) & 0xFF;
			}
		}
	}

	//single indirect
	if(j<final_data_blocks.size()){
		uint32_t block_no = calculate_bytes(inode_buffer+40+12*4,4);

		fill_indirect(block_no,1,final_data_blocks,j,sb,file,inode_no);

		for(int k = 0;k<4;k++){
			inode_buffer[40+12*4+k] = (block_no>>(8*k)) & 0xFF;
		}
	}

	//double indirect
	if(j<final_data_blocks.size()){
		uint32_t block_no = calculate_bytes(inode_buffer+40+13*4,4);

		fill_indirect(block_no,2,final_data_blocks,j,sb,file,inode_no);

		for(int k = 0;k<4;k++){
			inode_buffer[40+13*4+k] = (block_no>>(8*k)) & 0xFF;
		}
	}

	//triple
	if(j<final_data_blocks.size()){
		uint32_t block_no = calculate_bytes(inode_buffer+40+14*4,4);

		fill_indirect(block_no,3,final_data_blocks,j,sb,file,inode_no);

		for(int k = 0;k<4;k++){
			inode_buffer[40+14*4+k] = (block_no>>(8*k)) & 0xFF;
		}
	}

	for(int k = 0;k<4;k++){
		inode_buffer[4+k] = (new_size>>(8*k)) & 0xFF;
	}

	file.seekp(byte_offset);
	file.write(inode_buffer,sb.inode_size);

}

void fill_indirect(uint32_t &block_no,int level,vector<uint32_t>& new_blocks,int& j,Superblock sb, fstream& file,uint32_t inode_no){
	if(j>=new_blocks.size()){
		return;
	}

	if(block_no==0){
		vector<uint32_t> temp = allocate_blocks(1,inode_no,sb,file);

		if(temp.empty()){
			return;
		}

		block_no = temp[0];

		char empty_block[sb.block_size] = {};

		file.seekp(block_no*sb.block_size);
		file.write(empty_block,sb.block_size);
	}

	uint32_t entry_capacity = sb.block_size/4;

	char buffer[sb.block_size];

	file.seekg(block_no*sb.block_size);
	file.read(buffer,sb.block_size);

	for(uint32_t i = 0;i<entry_capacity && j<new_blocks.size();i++){
		uint32_t next_block = calculate_bytes(buffer+4*i,4);

		if(level==1){
			if(next_block==0){
				next_block = new_blocks[j++];
				for(int k = 0;k<4;k++){
					buffer[4*i+k] = (next_block>>(8*k)) & 0xFF;
				}
			}
		}
		else{
			fill_indirect(next_block,level-1,new_blocks,j,sb,file,inode_no);
			for(int k = 0;k<4;k++){
				buffer[4*i+k] = (next_block>>(8*k)) & 0xFF;
			}
		}

		file.seekp(block_no*sb.block_size);
		file.write(buffer,sb.block_size);
	}
	return;
}
vector<char> read_input_file(string filename){

	ifstream input(filename,ios::binary);

	if(!input){
		cout << "Failed to open file!" << endl;
		return {};
	}

	vector<char> data;
	char byte;
	while(input.get(byte)){
		data.push_back(byte);
	}

	return data;
}

void write_data(vector<char>& data,vector<uint32_t>& data_blocks,Superblock sb,fstream& file){
	int data_index = 0;
	for(uint32_t block_no : data_blocks){
		//write into the remaining space first
		int bytes_left = data.size()-data_index;
		if(bytes_left<=0){
			break;
		}

		int bytes_to_write = min((int)sb.block_size,bytes_left);

		cout << "Writing " << bytes_to_write << " bytes to block " << block_no << endl;

		//file.seekp(block_no*sb.block_size);
		//file.write(data.data()+data_index,bytes_to_write);

		data_index += bytes_to_write;
	}
}

void overwrite_file(uint32_t cwd_inode,string filename,string input_filename,Superblock sb,fstream& file){
	vector<char> data = read_input_file(input_filename);

	uint32_t inode_no = search(cwd_inode,sb,file,filename,1);

	if(inode_no==0){
		cout << "File not found!" << endl;
		return;
	}

	inode target = process_inode(inode_no,sb,file);

	vector<uint32_t> final_data_blocks = get_data_blocks(target,sb,file);

	int old_blocks = final_data_blocks.size();
	int new_blocks = ceil(((float)data.size())/sb.block_size);

	if(new_blocks>old_blocks){
		//give new blocks to file
		return;
	}

	final_data_blocks.resize(new_blocks);
	write_data(data,final_data_blocks,sb,file);
	write_inode(inode_no,final_data_blocks,data.size(),sb,file);
	return;
}

void append(uint32_t cwd_inode,string filename,string input_filename,Superblock sb, fstream& file){
	vector<char> data = read_input_file(input_filename);

	uint32_t inode_no = search(cwd_inode,sb,file,filename,1);

	if(inode_no==0){
		cout << "File not found!" << endl;
		return;
	}

	inode target = process_inode(inode_no,sb,file);

	int old_blocks = ceil(((float)target.size)/sb.block_size);
	int new_blocks = ceil(((float)data.size())/sb.block_size);

	return;
}

vector<uint32_t> allocate_blocks(int no_of_blocks,uint32_t inode_no,Superblock sb,fstream& file){
	vector<uint32_t> block_numbers = {};

	int block_group = (inode_no-1)/sb.inodes_per_group;

	file.seekg(2048);
	char buffer[sb.block_size];
	file.read(buffer,sb.block_size);

	group_descriptor gd = read_group_descriptor(buffer+(32*block_group));
	file.seekg(gd.block_bitmap*sb.block_size);
	unsigned char bitmap[sb.block_size];
	file.read((char*)bitmap,sb.block_size);


	for(int i = 0;i<sb.blocks_per_group;i++){
		int byte_index = i/8;
		int bit_index = i%8;

		int allocated = (bitmap[byte_index]>>bit_index) & 1;

		if(!allocated){
			bitmap[byte_index] = bitmap[byte_index] | (1 << bit_index);

			uint32_t block_number = block_group*sb.blocks_per_group + i;
			block_numbers.push_back(block_number);

			if(block_numbers.size()==no_of_blocks){
				break;
			}
		}
	}
	//file.seekp(gd.block_bitmap*sb.block_size);
	//file.write((char*)bitmap,sb.block_size);

	return block_numbers;
}

int main(){

	fstream file("/home/shivanshu_muppana/disk_proj/disk-backpup.img",ios::in | ios::out | ios::binary);

	//if error
	if(!file){
		cerr << "Failed: " << strerror(errno) << endl;
		return 1;
	}


	//process superblock
	file.seekg(1024);
	char bing[1024];
	file.read(bing,1024);

	Superblock sb;
	sb = process_superblock(bing);

	uint32_t current_inode = 2;
	vector<string> current_path;
	string command;

	while(true){
		cout << "/";
		for(int i = 0;i<current_path.size();i++){
			cout << current_path[i];
			if(i!=current_path.size()-1){
				cout << "/";
			}
		}
		cout << "> ";
		cin >> command;
		if(command=="exit"){
			break;
		}

		else if(command=="info"){
			read_core_structures(sb,file);
		}

		else if(command=="traverse"){
			traverse_directory(2,sb,file,0);
		}

		else if(command=="cd"){
			string target;
			cin >> target;

			uint32_t temp_inode_no = search(current_inode,sb,file,target,2);
			if(temp_inode_no==0){
				cout << "Can't find said directory." << endl;
				continue;
			}
			current_inode = temp_inode_no;

			if(target==".."){
				if(!current_path.empty()){
					current_path.pop_back();
				}
			}
			else if(target!="."){
				current_path.push_back(target);
			}
		}

		else if(command=="pwd"){
			cout << "/";

			for(int i = 0;i<current_path.size();i++){
				cout << current_path[i];
				if(i!=current_path.size()-1){
					cout << "/";
				}
			}
			cout << endl;
		}

		else if(command=="read"){
			string filename;
			cin >> filename;

			print_file(current_inode,sb,file,filename);
		}

		else if(command=="write"){
			string filename;
			string input_filename;

			cin >> filename;
			cin >> input_filename;

			overwrite_file(current_inode,filename,input_filename,sb,file);
		}
		else if(command=="append"){
			string filename;
			string input_filename;

			cin >> filename;
			cin >> input_filename;

			//append
		}

		else{
			cout << "Unknown command\n" << endl;
		}
	}


	return 0;
}
