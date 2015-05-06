#include "LFS.h"
#include <algorithm>

using namespace std;

// Constructor
LFS::LFS(int n, int s, int b, int p): 
numFiles(n), numSegments(s), blocksPerSegment(b), rw_head(1), policy(p){
	for(int i = 0; i <= numSegments; i++){
		Segment s(blocksPerSegment, numFiles);
		data.push_back(s);
	}

	// Initialize the file_block2segment map
	// Index 0 is a garbage map to allow for 1 indexing
	for(int fileID = 0; fileID <= numFiles; fileID++){
		unordered_map<int, int> blankMap;
		file_block2segment.push_back(blankMap);
	}
}   

void LFS::addFile(int fileID, int blocksInFile){

	// Validate file ID
	if(fileID < 1 || fileID > numFiles){
		cerr << "Failed to add file ID " << fileID << ": file ID must satisfy 1 <= fileID <= numFiles." << endl;
		return;
	}

	int blocksFilled = 1;
	// Start the loop at the position of the r/w head
	int currSegment = rw_head;

	// Loop until all blocks of the file are stored
	bool segmentAdded = false;

	// This will track the number of times the r/w head has started
	// from the beginning of the disk. If the r/w head was half way through
	// the disk and reached the end, it will start at the beginning. If the
	// second time around, it still does not find a place to write the remaining
	// blocks(s), there is no point in continuing, because the entire disk is filled.
	// This count should never reach a value greater than 1.
	int restartCount = 0;

	while(blocksFilled <= blocksInFile && restartCount < 2){
		// Make sure we are accessing a valid segment
		if(currSegment < numSegments){
			// Check if the segment is full
			if(data[currSegment].live_blocks < blocksPerSegment){
				data[currSegment].live_blocks++;
				data[currSegment].free_blocks--;
				data[currSegment].block2file[fileID]++;
				// Add this segment to the file2block mapping
				file_block2segment[fileID][blocksFilled] = currSegment;
				blocksFilled++;
			}
			else{
				// If it's full, try the next segment
				currSegment++;

				// Update r/w head position
				rw_head = currSegment;
			}
		}
		else{
			endOfDiskHandler();
			restartCount++;
		}
	}

	if(restartCount == 2){
		cout << "Write of file ID " << fileID << " failed: "
		<< "disk is full. Blocks written: " << blocksFilled << endl;
	}
}

void LFS::updateFile(int fileID, int numBlock){

	// Validate file ID
	if(fileID < 1 || fileID > numFiles){
		cerr << "Failed to add file ID " << fileID << ": file ID must satisfy 1 <= fileID <= numFiles." << endl;
		return;
	}

	// Get segment number
	int sNum = file_block2segment[fileID][numBlock];

	// Get the segment from disk
	Segment * s;
	if(sNum < numSegments){
		s = &data[sNum];
	}

	// Invalidate the original block
	s->free_blocks++;
	s->live_blocks--;
	s->block2file[fileID]--;

	// Add the new updated block
	// Start the loop at the position of the r/w head
	int currSegment = rw_head;
	// Make sure we are accessing a valid segment
	bool blockWritten = false;
	int restartCount = 0;
	while(!blockWritten && restartCount < 2){
		if(currSegment < numSegments){
			// Check if the segment is full
			if(data[currSegment].live_blocks < blocksPerSegment){
				// Update the segment characteristics
				data[currSegment].live_blocks++;
				data[currSegment].free_blocks--;
				data[currSegment].block2file[fileID]++;

				// Update the file block to segment mapping
				file_block2segment[fileID][numBlock] = currSegment;

				// Set the flag to terminate the loop
				blockWritten = true;
			}
			else{
				// If it's full, try the next segment
				currSegment++;
				// Update rw head
				rw_head = currSegment;
			}
		}
		else{
			endOfDiskHandler();
			restartCount++;
		}
	}

	if(restartCount == 2){
		cerr << "Update to file ID " << fileID << " failed: "
		<< " the disk is full." << endl;
	}
}

void LFS::endOfDiskHandler(){
	cout << "Reached end of disk." << endl;
	// We have reached the end of the disk, need to figure out what to do here
	// Clean?
	// Go back and find a free block?
	// Exit the loop and display an error?

	// Threading approach - Start at the beginning of the log
	// This approach will cause fragmentation of the log
	if(policy = POLICY_THREADING){
		rw_head = 1;
	}

	// Compaction approach with threading
	else if(policy = POLICY_CLEAN){
		// Move free blocks from unfilled segments to a new segment, leaving clean segments
		// Start at the beginning of the disk
	}

}

void LFS::displayFSContents(){
	cout << "==== BEGIN DISPLAY OF FILE SYSTEM CONTENTS ====" << endl;
	for(int segment = 1; segment < numSegments; segment++){
		cout << "Segment " << segment << " : " 
		<< "Free Blocks: " << data[segment].free_blocks 
		<< "\tLive Blocks: " << data[segment].live_blocks 
		<< endl;
		for(auto it : data[segment].block2file){
			if(it.first != 0){
				cout << "\tFile ID " << it.first << " has " << it.second
				<< " blocks in this segment." << endl;
			}
		}
	}
	cout << "==== END DISPLAY OF FILE SYSTEM CONTENTS ====" << endl;
}

void LFS::displayMap(){

	cout << "==== BEGIN DISPLAY OF FILE BLOCK TO SEGMENT MAP ====" << endl;
	for(int id = 1; id <= numFiles; id++){
		for(auto it0 : file_block2segment[id]){
			cout << "File ID " << id << " block " << it0.first 
			<< " is located in segment " << it0.second << endl;
		}
	}
	cout << "==== END DISPLAY OF FILE BLOCK TO SEGMENT MAP ====" << endl;
}

