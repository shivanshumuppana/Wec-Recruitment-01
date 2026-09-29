#include <iostream>
#include <cstdint>
#include <fstream>
using namespace std;


int main(){
	ifstream file("disk-backpup.img",ios::binary);

	file.seekg(1024);

	char bing[1024];

	file.read(bing,1024);
	for(int i = 0;i<1024;i++){
		cout << hex << (unsigned int)(unsigned char)bing[i] << " ";
	}
}
