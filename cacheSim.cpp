/* 046267 Computer Architecture - Spring 2020 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h> 

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

typedef struct cache_entry_t {

	std::vector<int> LRU;
	std::vector<way_t> ways;
	
} cache_entry;

typedef struct way_t {
	int tag;
	bool valid;
	bool dirty;
	unsigned long int address;
};

std::vector<cache_entry_t> L1_cache;
std::vector<cache_entry_t> L2_cache;

bool IsInCache(unsigned long int num, unsigned Bsize, int SetNum, unsigned cacheAss, std::vector<cache_entry_t> cache);
unsigned long int writeBlockToCache(unsigned long int num, unsigned Bsize, int SetNum, unsigned cacheAssoc, std::vector<cache_entry_t> cache);
void UpdateDirty(unsigned long int num, unsigned Bsize, int SetNum, unsigned cacheAssoc, std::vector<cache_entry_t> cache, bool dirty);
void removeBlockFromCache(unsigned long int num, unsigned Bsize, int SetNum, unsigned cacheAssoc, std::vector<cache_entry_t> cache);

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	//initiates caches 

	//initiate L1
	int L1NumLines = pow(2,L1Size)/(pow(2,BSize)) ;
	int L1NumSets = L1NumLines / (pow(2, L1Assoc));
	

	for (int i = 0; i < L1NumSets; i++)
	{
		for (int j = 0; j < (pow(2, L1Assoc)); j++)
			L1_cache[i].LRU[j] = j;
			L1_cache[i].ways[j].valid = 0;
			L1_cache[i].ways[j].dirty = 0;
			L1_cache[i].ways[j].address = 0;
			
	}

	//initiate L2
	int L2NumLines = pow(2, L2Size) / (pow(2, BSize));
	int L2NumSets = L2NumLines / (pow(2, L2Assoc));

	for (int i = 0; i < L2NumSets; i++)
	{
		for (int j = 0; j < (pow(2, L2Assoc)); j++)
			L2_cache[i].LRU[j] = j;
			L2_cache[i].ways[j].valid = 0;
			L2_cache[i].ways[j].dirty = 0;
			L2_cache[i].ways[j].address = 0;
	}

	int L1AccCnt = 0;
	int L2AccCnt = 0;
	int L1MissCnt = 0;
	int L2MissCnt = 0;
	double TotalTime = 0;

	while (getline(file, line)) {

		 
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

		//if read access
		if (operation == 'r')
		{
			//update L1 access counter and time
			L1AccCnt++;
			TotalTime += L1Cyc;

			//search for tag in L1
			if (IsInCache(num, Bsize, L1NumSets, L1Assoc, L1_cache))
			{
				updateLRU(num, Bsize, L1NumSets, L1Assoc, L1_cache)
				continue;
			}
			//tag not in L1,need to check in L2
			//update L1 miss cnt, L2 acc counter and add L2 acc time to total time
			L1MissCnt++;
			L2AccCnt++;
			TotalTime += L2Cyc;

			//search for tag in L2
			if (IsInCache(num, Bsize, L2NumSets, L2Assoc, L2_cache))
			{
				//tag in L2, write it to L1
				unsigned long int removedAddress = writeBlockToCache(num, Bsize, L1NumSets, L1Assoc, L1_cache);
				// if removed block from L1 and it is dirty: update L2 block as dirty
				if (removedAddress != 1)
				{
					UpdateDirty(removedAddress, Bsize, L2NumSets, L2Assoc, L2_cache, true);
				}
					
				//update L2 LRU because we read from block of address "num"
				updateLRU(num, Bsize, L2NumSets, L2Assoc, L2_cache);
				continue;
			}
			
			// tag in mem (not in L1, not in L2)
			L2MissCnt++;
			TotalTime += MemCyc;
			//add address to L2
			unsigned long int removedAddress = writeBlockToCache(num, Bsize, L2NumSets, L2Assoc, L2_cache);
				//if L2 have no space, take someone out
			//check if removed block form L2 is in L1
			if (IsInCache(removedAddress, Bsize, L1NumSets, L1Assoc, L1_cache))
			{
				//removed block from L2 is in L1, due to inclussivness,need to remove it
				removeBlockFromCache(num, Bsize, L1NumSets, L1Assoc, L1_cache);

			}

			//tag in L2, write it to L1
			//add address to L1
			//if L1 have no space, take someone out
			removedAddress = writeBlockToCache(num, Bsize, L1NumSets, L1Assoc, L1_cache);
			// if removed block from L1 and it is dirty: update L2 block as dirty
			if (removedAddress != 1)
			{
				// if it dirty update L2 dirty
				UpdateDirty(removedAddress, Bsize, L2NumSets, L2Assoc, L2_cache, true);
			}
			continue;

			
			
				
		}
		else if (operation == 'w')
		{
			//update L1 access counter and time
			L1AccCnt++;
			TotalTime += L1Cyc;

			//search for tag in L1
			if (IsInCache(num, Bsize, L1NumSets, L1Assoc, L1_cache))
			{
				//update dirty L1
				UpdateDirty(num, Bsize, L1NumSets, L1Assoc, L1_cache, true);
				updateLRU(num, Bsize, L1NumSets, L1Assoc, L1_cache);
				continue;
			}
			//tag not in L1,need to check in L2
			//update L1 miss cnt, L2 acc counter and add L2 acc time to total time
			L1MissCnt++;
			L2AccCnt++;
			TotalTime += L2Cyc;

			//search for tag in L2
			if (IsInCache(num, Bsize, L2NumSets, L2Assoc, L2_cache))
			{
				if (WrAlloc) //if WrAlloc==true need to write in L1
				{
					
					//if L1 have no space, take someone out
					unsigned long int removedAddress = writeBlockToCache(num, Bsize, L1NumSets, L1Assoc, L1_cache);
					UpdateDirty(num, Bsize, L1NumSets, L1Assoc, L1_cache, true);//mark data in L1 as dirty
					
					// if removed block from L1 and it is dirty: update L2 block as dirty
					if (removedAddress != 1)
					{
						UpdateDirty(removedAddress, Bsize, L2NumSets, L2Assoc, L2_cache, true);// if it dirty update L2 dirty
					}

					//update L2 LRU because we read from block of address "num"
					updateLRU(num, Bsize, L2NumSets, L2Assoc, L2_cache);
					continue;
					
				}
				//update dirty to L2, no need to write to L1 because no allocate
				updateLRU(num, Bsize, L2NumSets, L2Assoc, L2_cache);
				//update blokc in L2 as dirty
				UpdateDirty(num, Bsize, L2NumSets, L2Assoc, L2_cache, true);
				continue;
			}
			
			// tag in mem (not in L1, not in L2)
			TotalTime += MemCyc;
			L2MissCnt++;
			if (WrAlloc) //if WrAlloc==true need to write in L1 and L2
			{
				//add address to L1
					//if L1 have no space, take someone out
						// if it dirty update L2 dirty, (and update L2 lru??)
				//mark data in L1 as dirty

				
				//add address to L2
				unsigned long int removedAddress = writeBlockToCache(num, Bsize, L2NumSets, L2Assoc, L2_cache);
				//if L2 have no space, take someone out
				//check if removed block form L2 is in L1
				if (IsInCache(removedAddress, Bsize, L1NumSets, L1Assoc, L1_cache))
				{
					//removed block from L2 is in L1, due to inclussivness,need to remove it
					removeBlockFromCache(num, Bsize, L1NumSets, L1Assoc, L1_cache);

				}

				//tag in L2, write it to L1
				//add address to L1
				//if L1 have no space, take someone out
				removedAddress = writeBlockToCache(num, Bsize, L1NumSets, L1Assoc, L1_cache);
				// if removed block from L1 and it is dirty: update L2 block as dirty
				if (removedAddress != 1)
				{
					// if it dirty update L2 dirty
					UpdateDirty(removedAddress, Bsize, L2NumSets, L2Assoc, L2_cache, true);
				}
				UpdateDirty(num, Bsize, L1NumSets, L1Assoc, L1_cache, true);
			}
			//write to mem,  no need to write to L1 and L2 because no allocate
		}
	}
	
	double L1MissRate = 0;
	double L2MissRate = 0;
	double avgAccTime = 0;
	if (L1AccCnt != 0)
	{
		double L1MissRate = L1MissCnt / L1AccCnt;
		double avgAccTime = TotalTime / L1AccCnt;
	}
	if (L2AccCnt != 0)
	{
		double L2MissRate = L2MissCnt / L2AccCnt;
	}

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}

bool IsInCache(unsigned long int num,unsigned Bsize,int SetNum ,unsigned cacheAssoc ,std::vector<cache_entry_t> cache)
{
	int set = (num/Bsize)%(log2((double) SetNum));
	int tag =num/(Bsize + log2((double)SetNum)) ;
	for (int i = 0; i < pow(2, cacheAssoc); i++)
	{
		if ((cache[set].ways[i].valid == 1) && (cache[set].ways[i].tag == tag))
		{
			return true;
		}
	}
	return false;
}

void updateLRU(unsigned long int num, unsigned Bsize, int SetNum, unsigned cacheAssoc, std::vector<cache_entry_t> cache)
{
	int set = (num / Bsize) % (log2((double)SetNum));
	int tag = num / (Bsize + log2((double)SetNum));
	
	//find LRU idx of accessd way
	int i;
	for (i = 0; i < pow(2, cacheAssoc); i++)
	{
		if ((cache[set].ways[i].valid == 1) && (cache[set].ways[i].tag == tag))
		{
			break;
		}
	}
	//update LRU counters
	int tmp = cache[set].LRU[i];
	cache[set].LRU[i] = pow(2,cacheAssoc) -1;
	for (int j = 0; j < pow(2, cacheAssoc); j++)
	{
		if ((j != i) && (cache[set].LRU[j]> tmp))
		{
			cache[set].LRU[j]--;
		}
	}
}


//write to cache and return the tag of removed block case a block got removed duo to write opration
unsigned long int writeBlockToCache(unsigned long int num,unsigned Bsize, int SetNum, unsigned cacheAssoc, std::vector<cache_entry_t> cache)
{
	int set = (num / Bsize) % (log2((double)SetNum));
	int tag = num / (Bsize + log2((double)SetNum));

	//first check for empty ways
	for (int i = 0; i < pow(2, cacheAssoc); i++)
	{
		if (cache[set].ways[i].valid == 0)
		{
			//case there is an empty way, write the tag to it
			cache[set].ways[i].valid = 1;
			cache[set].ways[i].dirty = 0;
			cache[set].ways[i].tag = tag;
			cache[set].ways[i].address = num;
			updateLRU(num, Bsize, SetNum, cacheAssoc, cache);
			//case not dirty, no need to update so send false addres(because it is not aligned to 4 )
			return 1;

		}
	}

	//search LRU way
	int idx = 0;
	for (int i = 0; i < pow(2, cacheAssoc); i++)
	{
		if (cache[set].LRU[i] == 0)
		{
			idx = i;
		}
	}
	//save removed address
	
	
	unsigned long int removedAddress = 1; //case not dirty, no need to update so send false addres(because it is not aligned to 4 )
	
	if (cache[set].ways[idx].dirty)
	{
		removedAddress = cache[set].ways[idx].address;
	}
	
			
	//update way
	cache[set].ways[idx].valid = 1;
	cache[set].ways[idx].dirty = 0;
	cache[set].ways[idx].tag = tag;
	cache[set].ways[idx].address = num;
	updateLRU(num, Bsize, SetNum, cacheAssoc, cache);
	
	return removedAddress;
}

void UpdateDirty(unsigned long int num, unsigned Bsize, int SetNum, unsigned cacheAssoc, std::vector<cache_entry_t> cache,bool dirty)
{
	int set = (num / Bsize) % (log2((double)SetNum));
	int tag = num / (Bsize + log2((double)SetNum));

	for (int i = 0; i < pow(2, cacheAssoc); i++)
	{
		if (cache[set].ways[i].tag == tag)
		{
			cache[set].ways[i].dirty = dirty;
			return;
		}
	}
}

void removeBlockFromCache(unsigned long int num, unsigned Bsize, int SetNum, unsigned cacheAssoc, std::vector<cache_entry_t> cache)
{
	int set = (num / Bsize) % (log2((double)SetNum));
	int tag = num / (Bsize + log2((double)SetNum));

	for (int i = 0; i < pow(2, cacheAssoc); i++)
	{
		if (cache[set].ways[i].tag == tag)
		{
			cache[set].ways[i].valid= false;
			return;
		}
	}
}
}