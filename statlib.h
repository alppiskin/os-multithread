#ifndef STATLIB_H
#define STATLIB_H

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>

using namespace std;

// Color coding for the printed output
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define DEF "\033[0m"

#define SHOULD_PRINT false

unsigned long FILE_SIZE_LIMIT = (4000000000);

// Struct filled with data of the given file
struct FILE_DATA {
	bool isHumanReadable;
	unsigned long byte_count;
	string content;
};

// Struct filled for acquired stats
struct ACQUIRED_STATS {
	int bad_files;
	int directories;
	int regular_files;
	int special_files;
	unsigned long regular_file_bytes;
	int text_files;
	unsigned long text_file_bytes;
};

// Function prototypes
struct FILE_DATA processFile(int file);
void printAcquiredStats(struct ACQUIRED_STATS stats);
struct ACQUIRED_STATS makeAcquiredStats();
struct ACQUIRED_STATS acquireFileStats(string location);
struct ACQUIRED_STATS addAcquiredStats(struct ACQUIRED_STATS first, struct ACQUIRED_STATS second);
void setAcquiredStats(struct ACQUIRED_STATS *toSet, struct ACQUIRED_STATS real);

// Processing the input file
struct FILE_DATA processFile(int file){
	struct FILE_DATA data; // To fill content, readability, and bytes
	char buf[256]; // Buffer :)
	int char_count = 0;
	unsigned long total_count = 0;
	bool isHumanReadable = true;
	string content = "";
	
	// Reads the char count from the file
	while((char_count = read(file, buf, 256)) > 0){
		total_count += char_count;
		content.append(string(buf)); // Appends content filled from the boffer to content var

		// Readability check
		for(int i = 0; i < char_count; i++) {
			if( !(isspace(buf[i]) || isprint(buf[i])) ){
				isHumanReadable = false;
			}
		}
		// some /dev/ files are too big
		if(total_count > FILE_SIZE_LIMIT) {
			cout << RED << "MAXIMUM FILE SIZE LIMIT REACHED, RETURNING DATA SO FAR..." << DEF << endl;
			break;
		}
	}

	// Filled data struct
	data.byte_count = total_count;
	data.isHumanReadable = isHumanReadable;
	data.content = content;
	return data;
}

// Printing the stat struct with color coding
void printAcquiredStats(struct ACQUIRED_STATS stats){
	cout << "\n************************" << endl;
	cout << RED << "Bad Files: " << DEF << stats.bad_files << endl;
	cout << GREEN << "Directories: " << DEF << stats.directories << endl;
	cout << GREEN << "Regular Files: " << DEF << stats.regular_files << endl;
	cout << GREEN << "Special Files: " << DEF << stats.special_files << endl;
	cout << BLUE << "Regular File Bytes: " << DEF << stats.regular_file_bytes << endl;
	cout << GREEN << "Text Files: " << DEF << stats.text_files << endl;
	cout << BLUE << "Text File Bytes: " << DEF << stats.text_file_bytes << endl << endl;
}

// Initialized the stat structure with 0 first
struct ACQUIRED_STATS makeAcquiredStats(){
	struct ACQUIRED_STATS stats;
	stats.bad_files = 0;
	stats.directories = 0;
	stats.regular_files = 0;
	stats.special_files = 0;
	stats.regular_file_bytes = 0;
	stats.text_files = 0;
	stats.text_file_bytes = 0;
	return stats;
}

// Fills up the acquired stats struct
struct ACQUIRED_STATS acquireFileStats(string location){
	// Initialize stats
	string str = location;
	struct ACQUIRED_STATS stats = makeAcquiredStats();

	// SHOULD_PRINT used for debugging purposes
	if(SHOULD_PRINT)cout << "\n-----\nChecking file: " << RED << str << DEF << endl;
	
	// Call from stat() syscall
	struct stat s;
	if(0 == stat(str.c_str(), &s)){
	
		// If mode of the the file is regular
		if(S_ISREG(s.st_mode)){
			if(SHOULD_PRINT)cout << "FILE IS "<< GREEN << "REGULAR" << endl;
			stats.regular_files++; // Increment on regular file amount
			if(SHOULD_PRINT)cout << "Opening file: " << RED << str << DEF << endl;
			int file = open(str.c_str(), O_RDONLY | O_NONBLOCK | O_ASYNC); // Open the file
			if(SHOULD_PRINT)cout << "File size: " << s.st_size << endl;
			if(file >= 0){
				// To fill file data
				struct FILE_DATA data = processFile(file);
				// If a regular file is readable, it is a text
				if(data.isHumanReadable){
					if(SHOULD_PRINT)cout << GREEN << "FILE READABLE!" << DEF << endl;
					stats.text_files++; // Increment on text file count
					stats.text_file_bytes += s.st_size; // Increment the size
					stats.regular_file_bytes += s.st_size; // Increment the regular file stats since every text file is a regular file
				} else {
					stats.regular_file_bytes += s.st_size; // Only increment on regular file size. The file is not readable not a text
					if(SHOULD_PRINT)cout << RED << "FILE IS NOT READABLE!" << DEF << endl;
				}
			} else {
				if(SHOULD_PRINT)cout << "ERROR: " << RED << "COULD NOT OPEN FILE -> " << file << DEF << endl;
			}
			// Check for directory files
		} else if(S_ISDIR(s.st_mode)){
			stats.directories++; // Increment on directory file count
			if(SHOULD_PRINT)cout << "FILE IS " << BLUE << "DIRECTORY" << DEF << endl;
		} else {
			stats.special_files++; // If file is none of them, it is a special file
			if(SHOULD_PRINT)cout << "FILE IS " << MAGENTA << "SPECIAL" << DEF << endl;
		}


	} else {
		stats.bad_files++; // If the file is not even a special file (file opened != 0), it is a bad file increment on bad file count
		if(SHOULD_PRINT)cout << "ERROR: " << RED << "FILE STAT FAILURE" << DEF << endl;
	}

	return stats;
}

// Incrementing on whole struct at a time
struct ACQUIRED_STATS addAcquiredStats(struct ACQUIRED_STATS first, struct ACQUIRED_STATS second){
	struct ACQUIRED_STATS stats;
	stats.bad_files = first.bad_files + second.bad_files;
	stats.directories = first.directories + second.directories;
	stats.regular_files = first.regular_files + second.regular_files;
	stats.special_files = first.special_files + second.special_files;
	stats.regular_file_bytes = first.regular_file_bytes + second.regular_file_bytes;
	stats.text_files = first.text_files + second.text_files;
	stats.text_file_bytes = first.text_file_bytes + second.text_file_bytes;
	return stats;
}

// Setting filled up stat struct
void setAcquiredStats(struct ACQUIRED_STATS * toSet, struct ACQUIRED_STATS real){
	toSet->bad_files = real.bad_files;
	toSet->directories = real.directories;
	toSet->regular_files = real.regular_files;
	toSet->special_files = real.special_files;
	toSet->regular_file_bytes = real.regular_file_bytes;
	toSet->text_files = real.text_files;
	toSet->text_file_bytes = real.text_file_bytes;	
}

#endif
