
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <errno.h>


void mmkdir(const char *name);

void cat(const char *filename){
	int fd;
	char buffer[1024];
	ssize_t bytes_read;

	// Open file for reading
	fd = syscall(SYS_open, filename, O_RDONLY);
	if (fd == -1){
		const char *error_msg = "cat error: Could not open file\n";
		syscall(SYS_write, 1, error_msg, strlen(error_msg));
		return;
	}

	// Read file content to stdout
	while ((bytes_read = syscall(SYS_read, fd, buffer, 1024)) > 0){
		syscall(SYS_write, 1, buffer, bytes_read);
	}

	// Close the file after reading
	syscall(SYS_close, fd);

}

void rm(const char *filename){
	int fd = 1;
	struct stat file_stat;

	// Check if file exists, or if its directory
	if (stat(filename, &file_stat) == -1){
		const char *error_msg = "rm error: File does not exist or cannot be accessed\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
		return;
	}

	// Check if its a directory
	if (S_ISDIR(file_stat.st_mode)){
		const char *error_msg = "rm error: Cannot remove a directory\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
		return;
	}

	// Use the unlink system call to remove the file
	if (syscall(SYS_unlink, filename) == -1){
		const char *error_msg = "rm error: Could not remove file\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
	} else{
		const char *success_msg = "File removed successfully\n";
		syscall(SYS_write, fd, success_msg, strlen(success_msg));
	}

}

void mv(const char *src, const char *dst){
	int fd = 1;
	struct stat dst_stat;

	// Extract the directory part
	char dst_dir[1024];
	strcpy(dst_dir, dst);
	char *last_slash = strrchr(dst_dir, '/');

	// If there is a directory
	if (last_slash != NULL){
		*last_slash = '\0';
		if (stat(dst_dir, &dst_stat) == -1){
			if (errno == ENOENT){
				mmkdir(dst_dir);
			} else{
				const char *error_msg = "mv error: Could not stat destination directory\n";
				syscall(SYS_write, fd, error_msg, strlen(error_msg));
				return;
			}
		}
	}

	// Use rename system call to move the file
	if (syscall(SYS_rename, src, dst) == -1){
		const char *error_msg = "mv error: Could not move file\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
	} else{
		const char *success_msg = "File moved successfully\n";
		syscall(SYS_write, fd, success_msg, strlen(success_msg));
	}

}

void cp(const char *src, const char *dst){
	int src_fd, dst_fd;
	char buffer[1024];
	ssize_t bytes_read, bytes_written;
	int fd = 1;
	struct stat dst_stat;

	// Extract the directory part
	char dst_dir[1024];
	strcpy(dst_dir, dst);
	char *last_slash = strrchr(dst_dir, '/');

	// If there is a directory
	if (last_slash != NULL){
		*last_slash = '\0';
		if (stat(dst_dir, &dst_stat) == -1){
			if (errno == ENOENT){
				mmkdir(dst_dir);
			} else{
				const char *error_msg = "cp error: Could not stat destination directory.\n";
				syscall(SYS_write, fd, error_msg, strlen(error_msg));
				return;
			}
		}
	}

	// Open src file for reading
	src_fd = syscall(SYS_open, src, O_RDONLY);
	if (src_fd == -1){
		const char *error_msg = "cp error: Could not open source file\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
		return;
	}

	// Open dst file for writing or truncate if already exists
	dst_fd = syscall(SYS_open, dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (dst_fd == -1){
		const char *error_msg = "cp error: Could not open destination file\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
		syscall(SYS_close, src_fd);
		return;
	}

	// Copy content
	while ((bytes_read = syscall(SYS_read, src_fd, buffer, 1024)) > 0){
		bytes_written = syscall(SYS_write, dst_fd, buffer, bytes_read);
		if (bytes_written != bytes_read){
			const char *error_msg = "cp error: Error while writing to destination file\n";
			syscall(SYS_write, fd, error_msg, strlen(error_msg));
			break;
		}
	}

	// Close both files
	syscall(SYS_close, src_fd);
	syscall(SYS_close, dst_fd);

	const char *success_msg = "File copied succesfully\n";
	syscall(SYS_write, fd, success_msg, strlen(success_msg));

}

void cd(const char *path){
	int ret = syscall(SYS_chdir, path);
	int fd = 1;

	if (ret == -1){
		const char *error_msg = "cd error: Could not change directory\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
	} else{
		const char *success_msg = "Directory changed successfully\n";
		syscall(SYS_write, fd, success_msg, strlen(success_msg));
	}
}

void mmkdir(const char *name){
	int ret = syscall(SYS_mkdir, name, 0777);
	int fd = 1;

	if (ret == -1){
		const char *error_msg = "mkdir error: Could not create directory\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
	} else{
		const char *success_msg = "Directory created successfully\n";
		syscall(SYS_write, fd, success_msg, strlen(success_msg));
	}
}

void pwd(){
	char cwd[PATH_MAX];
	ssize_t ret;
	int fd = 1;

	if (getcwd(cwd, sizeof(cwd)) != NULL){
		ret = syscall(SYS_write, fd, cwd, strlen(cwd));
		syscall(SYS_write, fd, "\n", 1);
	} else{
		const char *error_msg = "pwd error\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
	}
}

void ls(){
	struct dirent *entry;
	DIR *dp = opendir(".");
	int fd = 1;

	if (dp == NULL){
		const char *error_msg = "Failed to open directory\n";
		syscall(SYS_write, fd, error_msg, strlen(error_msg));
		return;
	}

	while ((entry = readdir(dp)) != NULL){
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
			syscall(SYS_write, fd, entry->d_name, strlen(entry->d_name));
			syscall(SYS_write, fd, "\n", 1);
		}
	}

	closedir(dp);
}
