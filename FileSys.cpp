//File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount() {
  bfs.mount();
  curr_dir = 1;
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
}

// make a directory
void FileSys::mkdir(const char *name)
{

	if (strlen(name) > MAX_FNAME_SIZE) {
		cout << "File name too long" << endl;
		return;
	}
	
	//Get Curr dirblock
	struct dirblock_t curr;
	struct dirblock_t subDir;
	bfs.read_block(curr_dir, &curr);
	int num = curr.num_entries;	

	if (num == MAX_DIR_ENTRIES) {
		cout << "Directory is full" << endl;
		return;
	}

	bfs.read_block(curr_dir, &curr);
	int entries = curr.num_entries;
	
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		int blockNum = curr.dir_entries[i].block_num;
		
		if (blockNum == 0) { continue; }

		bfs.read_block(blockNum, &subDir);

		if (subDir.magic == DIR_MAGIC_NUM &&
			strcmp(curr.dir_entries[i].name, name) == 0)
			{
				cout << "Directory Exists" << endl;
				return;
			}
	}

	//get free block	
	short newBlockNum = bfs.get_free_block();
	
	if (newBlockNum == 0) { cout << "Disk Full" << endl; return;}

	//create new dirblock
	struct dirblock_t dirblock;
	dirblock.magic = DIR_MAGIC_NUM;
	dirblock.num_entries = 0;
	
	//Update curBlock
	//check to see if any previously allocated dir have been freed first for a spot
	for (int j = 0; j < curr.num_entries; j++) {
		if (curr.dir_entries[j].block_num == 0) {
			num = j;
			break;
		}
	}
	//Wasnt sure about the name so i did it a hard way
	//becasue you advised about strcpy() so who knows if this is ok
	for (int i = 0; i < strlen(name); i++) {
		curr.dir_entries[num].name[i] = name[i];
	}
	curr.dir_entries[num].name[strlen(name)] = '\0';
	curr.dir_entries[num].block_num = newBlockNum;
	curr.num_entries++;	

	//write new dir
	bfs.write_block(newBlockNum, &dirblock);
	//write updated curr dir
	bfs.write_block(curr_dir, &curr);

}

// switch to a directory
void FileSys::cd(const char *name)
{
	
	struct dirblock_t currBlock;
	struct dirblock_t subDir;

	bfs.read_block(curr_dir, &currBlock);
	int entries = currBlock.num_entries;

	if (entries == 0) {
		cout << "Directory Empty" << endl;
		return;
	}

	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		int blocknum = currBlock.dir_entries[i].block_num;

		if (blocknum == 0) { continue; }	
	
		bfs.read_block(blocknum, &subDir);

		if (subDir.magic == DIR_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0) 
			{
				curr_dir = blocknum;
				return;
			}

	}

	cout << "File is not a directory" << endl;

}

// switch to home directory
void FileSys::home()
{
	curr_dir = 1;
}

// remove a directory
void FileSys::rmdir(const char *name)
{
	struct dirblock_t currBlock;
	struct dirblock_t subDir;

	bfs.read_block(curr_dir, &currBlock);
	int entries = currBlock.num_entries;
	
	if (entries == 0) {
		cout << "Directory Empty" << endl;
		return;
	}
	
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		int blockNum = currBlock.dir_entries[i].block_num;
		
		if (blockNum == 0) { continue; }

		bfs.read_block(blockNum, &subDir);

		if (subDir.magic == DIR_MAGIC_NUM
			&& strcmp(currBlock.dir_entries[i].name, name) == 0)
			{
				if (subDir.num_entries != 0) {
					cout << "Directory not empty" << endl;
					return;
				}
				else {
					//free sub
					bfs.reclaim_block(currBlock.dir_entries[i].block_num);
					//change dir
					for (int j = 0; j < MAX_FNAME_SIZE; j++) {
						currBlock.dir_entries[i].name[j] = ' ';
					}
					currBlock.dir_entries[i].block_num = 0;
					currBlock.num_entries--;
					//update dir
					bfs.write_block(curr_dir, &currBlock);
					return;
				}
			}
	}
	cout << "File is not directory" << endl;		
			
}


// list the contents of current directory
void FileSys::ls()
{
	struct dirblock_t dirblock;
	struct dirblock_t subDir;

	bfs.read_block(curr_dir, &dirblock);
	
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		int entry = dirblock.dir_entries[i].block_num;
		
		if (entry == 0) { continue; }
		
		bfs.read_block(entry,&subDir);

		if (subDir.magic == DIR_MAGIC_NUM) {
			cout << dirblock.dir_entries[i].name << '/' << ' ';
		}
		else {
			cout << dirblock.dir_entries[i].name << ' ';
		}
	}
	cout << endl;	 		
}

// create an empty data file
void FileSys::create(const char *name)
{

	if (strlen(name) > MAX_FNAME_SIZE) {
		cout << "File name too long" << endl;
		return;
	}	
	
	//Get Curr dirblock
    struct dirblock_t curr;
	struct inode_t subDir;

    bfs.read_block(curr_dir, &curr);   
	int num = curr.num_entries; 
	
	//Check if Dir Full
	if (num == MAX_DIR_ENTRIES) {
		cout << "Directory is full" << endl;
		return;
	}

	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		int blockNum = curr.dir_entries[i].block_num;
		
		if (blockNum == 0) { continue; }

		bfs.read_block(blockNum, &subDir);

		if (subDir.magic == INODE_MAGIC_NUM &&
			strcmp(curr.dir_entries[i].name, name) == 0)
			{
				cout << "File Exists" << endl;
				return;
			}
	}
 
    //get free block
    short newBlockNum = bfs.get_free_block();  

	if (newBlockNum == 0) { cout << "Disk Full" << endl; return;}
	
	//create new file block                
	struct inode_t fileblock;
    fileblock.magic = INODE_MAGIC_NUM;
    fileblock.size = 0;                                                                 
	for (int l = 0; l < MAX_DATA_BLOCKS; l++) {
		fileblock.blocks[l] = 0;
	} 

	for (int j = 0; j < curr.num_entries; j++) {
		if (curr.dir_entries[j].block_num == 0) {
			num = j;
			break;
		}
	}

    //Update curBlock
    for (int i = 0; i < strlen(name); i++) { 
		 curr.dir_entries[num].name[i] = name[i];  
	}
    curr.dir_entries[num].name[strlen(name)] = '\0';
	curr.dir_entries[num].block_num = newBlockNum;
	curr.num_entries++;
	
	//write new dir              
	bfs.write_block(newBlockNum, &fileblock);               
	//write updated curr dir          
    bfs.write_block(curr_dir, &curr);	

}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{

	if (strlen(data) > MAX_FILE_SIZE) {
		cout << "Data too large for file" << endl;
		return;
	}

	struct inode_t node;
	struct datablock_t d;
	//read dir
	struct dirblock_t currBlock;
	bfs.read_block(curr_dir, &currBlock);
	int num = currBlock.num_entries;

	if (num == 0) {
		cout << "Directory Empty" << endl;
		return;
	}
	bool found = false;
	
	//check dir for file
	for (int i = 0; i < num; i++) {
		int entry = currBlock.dir_entries[i].block_num;
		
		bfs.read_block(entry, &node);

	
		if (node.magic == DIR_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0)
			{
				cout << "File is Directory" << endl;
				return;
			}

		if (node.magic == INODE_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0)
			{
				found = true;
				//found file and has read in node
				//check if full
				if (node.size == MAX_FILE_SIZE) {
					cout << "File Full" << endl;
					return;
				}
				//No data make new
				if (node.size == 0) {
					int numBlocksReq = (strlen(data) + BLOCK_SIZE - 1) / BLOCK_SIZE; 
					//for each block req fill and write
					char* newData = strdup(data);
					int index;				
					
					for (int x = 0; x < numBlocksReq - 1; x++) {
						for (int j = 0; j < BLOCK_SIZE; j++) {
							d.data[j] = newData[j];
						}
						index = bfs.get_free_block();
						if (index == 0) { cout << "Disk Full" << endl; return;}
						node.size += BLOCK_SIZE;
						node.blocks[x] = index;
						bfs.write_block(index, &d);
						bfs.write_block(currBlock.dir_entries[i].block_num, &node);
						newData = newData + BLOCK_SIZE; 
					}
					for (int l = 0; l < strlen(newData); l++) {
						d.data[l] = newData[l];
					}
					index = bfs.get_free_block();
					if (index == 0) { cout << "Disk Full" << endl; return;}
					node.size += strlen(newData);
					if (node.size < BLOCK_SIZE) {
						node.blocks[0] = index;
					}
					else { 
						node.blocks[numBlocksReq-1] = index;
					}
					bfs.write_block(index, &d);
					bfs.write_block(currBlock.dir_entries[i].block_num, &node);
					return; 	
				}
				//Existing Data
				if (node.size > 0 && strlen(data) + node.size < MAX_FILE_SIZE) {
					
					bool leftover = false;
					char* newData = strdup(data);
					int numBlocksUsed = (node.size + BLOCK_SIZE - 1) / BLOCK_SIZE;
					//Get data block that partially full
					
					if (node.size % BLOCK_SIZE != 0) {
							bfs.read_block(node.blocks[numBlocksUsed-1], &d);
							
							int startPoint = node.size - ((numBlocksUsed - 1) * BLOCK_SIZE);
						
							int val;	
							if (strlen(newData) < BLOCK_SIZE - startPoint) { val = strlen(newData); }
							else { val = BLOCK_SIZE - startPoint; }
							//Copy data
							for (int l = 0; l < val; l++) {
								d.data[startPoint + l] = newData[l];
							}
							//update node and write data/node back
							newData = newData + val;
							node.size += val;
							bfs.write_block(node.blocks[numBlocksUsed-1], &d);
							bfs.write_block(currBlock.dir_entries[i].block_num, &node);
							if (strlen(newData) > 0) {leftover = true;}		
					}
					//Fill the rest of the data
					//bfs.read_block(node.blocks[numBlocksUsed-1], &d);
					
					if ((node.size % BLOCK_SIZE == 0 || leftover) && strlen(newData) > 0) {
							numBlocksUsed = (node.size + BLOCK_SIZE - 1) / BLOCK_SIZE;
													
							bfs.read_block(node.blocks[numBlocksUsed], &d);
					
							int numBlocksReq = (strlen(newData) + BLOCK_SIZE - 1) / BLOCK_SIZE; 
							//for each block req fill and write
							int index;				
				
							for (int x = 0; x < numBlocksReq - 1; x++) {
								for (int j = 0; j < BLOCK_SIZE; j++) {
									d.data[j] = newData[j];
								}
								index = bfs.get_free_block();
								if (index == 0) { cout << "Disk Full" << endl; return;}
								node.size += BLOCK_SIZE;
								if(leftover) {node.blocks[x] = index;}
								else {node.blocks[numBlocksUsed] = index;}
								bfs.write_block(index, &d);
								bfs.write_block(currBlock.dir_entries[i].block_num, &node);
								newData = newData + BLOCK_SIZE; 
							}
							for (int l = 0; l < strlen(newData); l++) {
								d.data[l] = newData[l];
							}
							index = bfs.get_free_block();
							if (index == 0) { cout << "Disk Full" << endl; return;}
							node.size += strlen(newData);
							if (node.size < BLOCK_SIZE) {
								node.blocks[0] = index;
							}
							else { 
								node.blocks[numBlocksUsed] = index;
							}
							bfs.write_block(index, &d);
							bfs.write_block(entry, &node);
							return;
					}
				
				} 
				else {cout << "Append exceeds maximum file size" << endl; return; }		
			}
	}						
	if (!found) { cout << "File not found" << endl; }	 			
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
	struct inode_t node;	
	struct datablock_t d;
	//read dir
	struct dirblock_t currBlock;
	bfs.read_block(curr_dir, &currBlock);
	int num = currBlock.num_entries;

	if (num == 0) {
		cout << "Directory Empty" << endl;
		return;
	}
	
	//check dir for file
	for (int i = 0; i < MAX_DIR_ENTRIES ; i++) {
		int entry = currBlock.dir_entries[i].block_num;
	
		if (entry == 0) {continue;}
			
		bfs.read_block(entry, &node);

		if (node.magic == DIR_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0)
			{
				cout << "File is Directory" << endl;
				return;
			}

		if (node.magic == INODE_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0) 
			{
							
				int req = (node.size + BLOCK_SIZE - 1) / BLOCK_SIZE;
				for (int j = 0; j < req - 1; j++) {
					bfs.read_block(node.blocks[j], &d);
					for (int l = 0; l < BLOCK_SIZE; l++) {
						cout << d.data[l];
					}
				}
				bfs.read_block(node.blocks[req-1], &d);
				for (int x = 0; x < node.size - ((req-1)*BLOCK_SIZE); x++) {
					cout << d.data[x];
				}
			cout << endl;
			return;
			}
	}
	cout << "File not found" << endl; 
}

// display the last N bytes of the file
void FileSys::tail(const char *name, unsigned int n)
{

	struct inode_t node;	
	struct datablock_t d;
	//read dir
	struct dirblock_t currBlock;
	bfs.read_block(curr_dir, &currBlock);
	int num = currBlock.num_entries;

	if (num == 0) {
		cout << "Directory Empty" << endl;
		return;
	}
	
	//check dir for file
	for (int i = 0; i < MAX_DIR_ENTRIES ; i++) {
		int entry = currBlock.dir_entries[i].block_num;
	
		if (entry == 0) {continue;}
			
		bfs.read_block(entry, &node);

		if (node.magic == DIR_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0)
			{
				cout << "File is Directory" << endl;
				return;
			}

		if (node.magic == INODE_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0) 
			{
				//Found node	
				if (n >= node.size) {return cat(name);}					
		
				int totReqBlocks = (node.size + BLOCK_SIZE - 1) / BLOCK_SIZE;
				int lastBlockIndex = totReqBlocks - 1;
				int charInLastBlock = node.size - ((totReqBlocks-1)*BLOCK_SIZE);		

				if (n <= charInLastBlock) {
					//Last Block print				
					bfs.read_block(node.blocks[lastBlockIndex], &d);
					int start = charInLastBlock - n;
					for (int x = start; x < charInLastBlock; x++) {
						cout << d.data[x];
					}
				}
				else {
					
					int startBlockIndex = (((node.size - n) + BLOCK_SIZE - 1) / BLOCK_SIZE) - 1;
					//First Block
					int start = (node.size - n) - (startBlockIndex*BLOCK_SIZE);
					bfs.read_block(node.blocks[startBlockIndex], &d);
					for (int y = start; y < BLOCK_SIZE; y++) {
						cout << d.data[y];
					}
					//Middle if any
					for (int j = startBlockIndex + 1; j < lastBlockIndex; j++) {
						bfs.read_block(node.blocks[j], &d);
						for (int l = 0; l < BLOCK_SIZE; l++) {
							cout << d.data[l];
						}
					}
					//LastBlock
					bfs.read_block(node.blocks[lastBlockIndex], &d);
					for (int x = 0; x < charInLastBlock; x++) {
						cout << d.data[x];
					}

				}
					
			cout << endl;
			return;	
			}
	}
	cout << "File not found" << endl;
}

// delete a data file
void FileSys::rm(const char *name)
{

	struct inode_t node;	
	struct datablock_t d;
	//read dir
	struct dirblock_t currBlock;
	bfs.read_block(curr_dir, &currBlock);
	int num = currBlock.num_entries;

	if (num == 0) {
		cout << "Directory Empty" << endl;
		return;
	}
	
	//check dir for file
	for (int i = 0; i < MAX_DIR_ENTRIES ; i++) {
		int entry = currBlock.dir_entries[i].block_num;
	
		if (entry == 0) {continue;}
			
		bfs.read_block(entry, &node);

		
		if (node.magic == DIR_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0)
			{
				cout << "File is Directory" << endl;
				return;
			}

		if (node.magic == INODE_MAGIC_NUM && 
			strcmp(currBlock.dir_entries[i].name, name) == 0) 
			{
				//Reclaim data in file
				int usedBlocks = (node.size + BLOCK_SIZE - 1) / BLOCK_SIZE; 
				for (int j = 0; j < usedBlocks; j++) {
					bfs.reclaim_block(node.blocks[j]);
				}

				//Reclaim File
				bfs.reclaim_block(entry);

				//update curr dir
  				for (int l = 0; l < strlen(name); l++) { 
					currBlock.dir_entries[i].name[l] = '\0';  
				}
    			currBlock.dir_entries[i].name[strlen(name)] = '\0';
				currBlock.dir_entries[i].block_num = 0;
				currBlock.num_entries--;

				bfs.write_block(curr_dir, &currBlock);
				return;
			}
	}
	cout << "File not Found" << endl;
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
	
	struct dirblock_t dirblock;
	struct dirblock_t subDir;

	bfs.read_block(curr_dir, &dirblock);
	
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		int entry = dirblock.dir_entries[i].block_num;
		
		if (entry == 0) { continue; }
		
		bfs.read_block(entry,&subDir);

		if (subDir.magic == DIR_MAGIC_NUM 
			&& strcmp(dirblock.dir_entries[i].name, name) == 0)
			{
				cout << "Directory Name: " << dirblock.dir_entries[i].name << endl;
				cout << "Directory Block: " << dirblock.dir_entries[i].block_num << endl;
				return;
			}
		if (subDir.magic == INODE_MAGIC_NUM
			&& strcmp(dirblock.dir_entries[i].name, name) == 0)
			{	
				//I could get the static cast to work so i reread it
				struct inode_t sub;
				bfs.read_block(entry, &sub);
				cout << "Inode block: " << dirblock.dir_entries[i].block_num << endl;
				cout << "Bytes in file: " << sub.size << endl;
				cout << "Number of blocks: " << ((sub.size + BLOCK_SIZE - 1) / BLOCK_SIZE) + 1 << endl;
				cout << "First block: " << sub.blocks[0] << endl;
				return;
			}
	}
	cout << "File not in Directory" << endl;

}

// HELPER FUNCTIONS (optional)

/*
Just helping me with logic stuff - unnecessary


void FileSys::writeNewBlock(int numBlocksReq, char * data, inode_t * node, dirblock_t * curr, int entryIndex) {
	
	struct datablock_t d;
	int index;
	for (int i = 0; i < numBlocksReq - 1; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			d.data[j] = data[j];
		}
		index = bfs.get_free_block();
		node->size += BLOCK_SIZE;
		node->blocks[i] = index;
		bfs.write_block(index, &d);
		bfs.write_block(curr->dir_entries[i].block_num, &node);
		data = data + BLOCK_SIZE; 
	}
	for (int l = 0; l < strlen(data); l++) {
		d.data[l] = data[l];
	}
	index = bfs.get_free_block();
	node->size += strlen(data);
	node->blocks[(node->size + BLOCK_SIZE - 1)/BLOCK_SIZE] = index;
	bfs.write_block(index, &d);
	bfs.write_block(curr->dir_entries[entryIndex].block_num, &node);
}

*/
