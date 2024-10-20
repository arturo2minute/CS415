
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>

void copyFile(char *src, char *dst);
void makeDir(const char *name);

void displayFile(const char *filename){

	// Open File
	int thisFile = open(filename, O_RDONLY);

	// Check if opened
	if (thisFile == -1){
		perror("open() error");
		return;
	}

	// Set contents buffer
	char contents[4096];
	ssize_t bytesRead;

	// Fill buffer with contents
	while ((bytesRead = read(thisFile, contents, sizeof(contents))) > 0){
		ssize_t bytesWritten = write(1, contents, bytesRead);
                if (bytesWritten != bytesRead){
                        perror("write() error");
                        close(thisFile);
                        return;
                }
	}

	if (bytesRead == -1){
		perror("read() error");
	}

	// Close file
	close(thisFile);

}

void deleteFile(const char *filename){
	int fd = 1;
	struct stat file_stat;

	// Check if file exists, or if its directory
	if (stat(filename, &file_stat) == -1){
		const char *error_msg = "rm error: File does not exist or cannot be accessed\n";
		write(STDOUT_FILENO, error_msg, strlen(error_msg));
		return;
	}

	// Check if its a directory
	if (S_ISDIR(file_stat.st_mode)){
		const char *error_msg = "rm error: Cannot remove a directory\n";
		write(STDOUT_FILENO, error_msg, strlen(error_msg));
		return;
	}

	// Use the unlink system call to remove the file
	if (remove(filename) == -1){
		const char *error_msg = "rm error: Could not remove file\n";
		write(STDOUT_FILENO, error_msg, strlen(error_msg));
	} else{
		const char *success_msg = "File removed successfully\n";
		write(STDOUT_FILENO, success_msg, strlen(success_msg));
	}

}

void moveFile(char *src, char *dst){
	//Call copyFile and call remove on src file
	if (src == dst){
		return;
	} else{
		copyFile(src, dst);
		deleteFile(src);
	}
}

void copyFile(char *src, char *dst){
	int sourceFile = open(src, O_RDONLY);
	if (sourceFile == -1){
		perror("open() source file error");
		return;
	}

	char *fullDest = malloc(strlen(dst) + strlen(src) + 2);
	int destinationFile;
	struct stat destStat;

	// Check if dst is directory
	if (lstat(dst, &destStat) == 0 && S_ISDIR(destStat.st_mode)){
		strcpy(fullDest, dst);
		strcat(fullDest, "/");
		strcat(fullDest, basename(src));
		destinationFile = open(fullDest, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (destinationFile == -1){
			perror("open() destination file error");
			close(sourceFile);
			return;
		}
	} else{
		destinationFile = open(dst, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (destinationFile == -1){
			perror("open() destination file error");
                        close(sourceFile);
                        return;
		}
	}

	// Copy contents from src to dst
	char contents[4096];
	ssize_t bytesRead;
	while ((bytesRead = read(sourceFile, contents, sizeof(contents))) > 0){
		ssize_t bytesWritten = write(destinationFile, contents, bytesRead);
		if (bytesWritten != bytesRead){
			perror("write() error");
                        close(sourceFile);
			close(destinationFile);
                        return;
		}
	}

	if (bytesRead == -1){
		perror("read() error");
	}

	//Close all files
	if (close(sourceFile) == -1){
		perror("close() source file error");
	}

	if (close(destinationFile) == -1){
                perror("read() destination file error");
        }

	free(fullDest);
}

void changeDir(const char *path){
	int ret = chdir(path);
	int fd = 1;

	if (ret == -1){
		const char *error_msg = "cd error: Could not change directory\n";
		write(STDOUT_FILENO, error_msg, strlen(error_msg));
	} else{
		const char *success_msg = "Directory changed successfully\n";
		write(STDOUT_FILENO, success_msg, strlen(success_msg));
	}
}

void makeDir(const char *name){
	int ret = mkdir(name, 0777);
	int fd = 1;

	if (ret == -1){
		const char *error_msg = "mkdir error: Could not create directory\n";
		write(STDOUT_FILENO, error_msg, strlen(error_msg));
	} else{
		const char *success_msg = "Directory created successfully\n";
		write(STDOUT_FILENO, success_msg, strlen(success_msg));
	}
}

void showCurrentDir(){
	char cwd[PATH_MAX];
	ssize_t ret;
	int fd = 1;

	if (getcwd(cwd, sizeof(cwd)) != NULL){
		write(STDOUT_FILENO, cwd, strlen(cwd));
		write(STDOUT_FILENO, "\n", strlen("\n"));
	} else{
		const char *error_msg = "pwd error\n";
		write(STDOUT_FILENO, error_msg, strlen(error_msg));
	}
}

void listDir(){
	DIR *cur_dir;
	struct dirent *entry;

	// Open current directory
	cur_dir = opendir(".");
	if (cur_dir == NULL){
		perror("opendir");
		return;
	}

	// Read and write
	while ((entry = readdir(cur_dir)) != NULL){
		write(1, entry->d_name, strlen(entry->d_name));
		write(1, " ", 1);
	}

	//Write new line
	write(1, "\n", 1);

	// Close directory
	closedir(cur_dir);
}
