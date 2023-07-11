#include "userfs.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum {
    BLOCK_SIZE = 512,
    MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
    /** Block memory. */
    char *memory;
    /** How many bytes are occupied. */
    int occupied;
    /** Next block in the file. */
    struct block *next;
    /** Previous block in the file. */
    struct block *prev;

    /* PUT HERE OTHER MEMBERS */
};

struct file {
    /** Double-linked list of file blocks. */
    struct block *block_list;
    /**
     * Last block in the list above for fast access to the end
     * of file.
     */
    struct block *last_block;
    /** How many file descriptors are opened on the file. */
    int refs;
    /** File name. */
    char *name;
    /** Files are stored in a double-linked list. */
    struct file *next;
    struct file *prev;

    int is_deleted;
    size_t size;
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
    struct file *file;
    int offset;
    int flags;
    struct block *block;
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;
static int file_descriptor_capacity = 0;

enum ufs_error_code
ufs_errno() {
    return ufs_error_code;
}

int
create_filedesc(struct file *file, int flags) {
    if (file_descriptor_count == file_descriptor_capacity) {
        file_descriptor_capacity = (file_descriptor_capacity << 1) + 1;
        struct filedesc **new_list = (struct filedesc **) malloc(sizeof(struct filedesc *) * file_descriptor_capacity);
        memcpy(new_list, file_descriptors, sizeof(struct filedesc *) * (file_descriptor_capacity / 2));
        if (file_descriptors != NULL)
            free(file_descriptors);
        file_descriptors = new_list;
    }
    file->refs++;
    struct filedesc *fd = (struct filedesc *) malloc(sizeof(struct filedesc));
    fd->file = file;
    fd->flags = flags;
    fd->offset = 0;
    fd->block = file->block_list;
    int i = 0;
    for (; i < file_descriptor_capacity; i++) {
        if (file_descriptors[i] == NULL) {
            file_descriptor_count++;
            file_descriptors[i] = fd;
            break;
        }
    }
    return i;
}

struct file *
find_file(const char *name) {
    struct file *fs = file_list;
    while (fs != NULL) {
        if (!strcmp(fs->name, name))
            break;
        fs = fs->next;
    }
    return fs;
}

int
ufs_open(const char *filename, int flags) {
    /* IMPLEMENT THIS FUNCTION */
    if (flags == UFS_CREATE) {
        struct file *fs = find_file(filename);
        if (fs != NULL) {
            int fd = create_filedesc(fs, UFS_READ_WRITE);
            ufs_error_code = UFS_ERR_NO_ERR;
            return fd;
        }
        struct file *new_file = (struct file *) malloc(sizeof(struct file));
        new_file->name = (char *) malloc(strlen(filename) + 1);
        new_file->size = new_file->refs = new_file->is_deleted = 0;
        new_file->next = new_file->prev = NULL;
        strcpy(new_file->name, filename);
        if (file_list == NULL) {
            file_list = new_file;
        } else {
            fs = file_list;
            while (fs->next != NULL)
                fs = fs->next;
            fs->next = new_file;
            fs->next->prev = fs;
        }
        new_file->block_list = new_file->last_block = (struct block *) malloc(sizeof(struct block));
        new_file->block_list->memory = NULL;
        new_file->block_list->next = new_file->block_list->prev = NULL;
        new_file->block_list->occupied = 0;
        int fd = create_filedesc(new_file, UFS_READ_WRITE);
        ufs_error_code = UFS_ERR_NO_ERR;
        return fd;
    }
    if (flags == 0) {
        flags = UFS_READ_WRITE;
    }
    struct file *fs = find_file(filename);
    if (fs == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    int fd = create_filedesc(fs, flags);
    ufs_error_code = UFS_ERR_NO_ERR;
    return fd;
}

ssize_t
ufs_write(int fd, const char *buf, size_t size) {
    if (fd < 0 || fd >= file_descriptor_capacity || file_descriptors[fd] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct filedesc *f = file_descriptors[fd];
    if (f->flags != UFS_READ_WRITE && f->flags != UFS_WRITE_ONLY) {
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }
    size_t written = 0;
    struct block *block = f->block;

    while (written != size) {
        size_t cnt = (BLOCK_SIZE - f->offset);
        if (cnt > size - written) {
            cnt = size - written;
        }
        if (f->file->size + cnt > MAX_FILE_SIZE) {
            ufs_error_code = UFS_ERR_NO_MEM;
            return -1;
        }
        if (cnt > 0 && block->occupied == 0) {
            block->memory = (char *) malloc(sizeof(char) * BLOCK_SIZE);
        }
        memcpy(block->memory + f->offset, buf + written, cnt);
        f->offset += cnt;
        if (block->occupied < f->offset)
            block->occupied = f->offset;
        f->file->size += cnt;
        written += cnt;
        if (block->occupied == BLOCK_SIZE) {
            if (block->next == NULL) {
                block->next = (struct block *) malloc(sizeof(struct block));
                block->next->prev = block;
                block->next->next = NULL;
                block->next->occupied = 0;
                block->next->memory = NULL;
                f->file->last_block = block->next;
            }
            f->offset = 0;
            f->block = block->next;
            block = block->next;
        }
    }
    ufs_error_code = UFS_ERR_NO_ERR;

    return written;
}

ssize_t
ufs_read(int fd, char *buf, size_t size) {
    /* IMPLEMENT THIS FUNCTION */
    if (fd < 0 || fd >= file_descriptor_capacity || file_descriptors[fd] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct filedesc *f = file_descriptors[fd];
    if (f->flags != UFS_READ_WRITE && f->flags != UFS_READ_ONLY) {
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }
    size_t read = 0;
    while (read != size) {
        size_t cnt = (f->block->occupied - f->offset);
        if (cnt > size - read) {
            cnt = size - read;
        }
        if (cnt == 0)
            break;
        memcpy(buf + read, f->block->memory + f->offset, cnt);
        f->offset += cnt;
        read += cnt;
        if (f->offset == BLOCK_SIZE) {
            f->block = f->block->next;
            f->offset = 0;
        }
    }

    ufs_error_code = UFS_ERR_NO_ERR;
    return read;
}

void
destroy_blocks(struct block *list) {
    while (list != NULL) {
        struct block *n = list->next;
        if (list->memory != NULL)
            free(list->memory);
        if (list != NULL)
            free(list);
        list = n;
    }
}

int
ufs_close(int fd) {
    /* IMPLEMENT THIS FUNCTION */
    if (fd < 0 || fd >= file_descriptor_capacity || file_descriptors[fd] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    file_descriptors[fd]->file->refs--;

    if (file_descriptors[fd]->file->refs == 0 && file_descriptors[fd]->file->is_deleted) {
        free(file_descriptors[fd]->file->name);
        destroy_blocks(file_descriptors[fd]->file->block_list);
        free(file_descriptors[fd]->file);
    }
    free(file_descriptors[fd]);
    file_descriptors[fd] = NULL;
    file_descriptor_count--;
    ufs_error_code = UFS_ERR_NO_ERR;
    return 0;
}

int
ufs_delete(const char *filename) {
    /* IMPLEMENT THIS FUNCTION */
    struct file *file = file_list;
    while (file != NULL) {
        if (!strcmp(file->name, filename)) {
            break;
        }
        file = file->next;
    }
    if (file == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    if (file->prev != NULL) {
        file->prev->next = file->next;
    } else {
        file_list = file->next; // first file is head of the list
    }
    if (file->next != NULL) {
        file->next->prev = file->prev;
    }
    if (file->refs != 0) {
        file->is_deleted = 1;
    } else {
        destroy_blocks(file->block_list);
        free(file->name);
        free(file);
    }
    ufs_error_code = UFS_ERR_NO_ERR;
    return 0;
}

void
ufs_destroy(void) {
    struct file *file = file_list;
    while (file != NULL) {
        struct file *n = file->next;
        destroy_blocks(file->block_list);
        free(file->name);
        free(file);
        file = n;
    }
    free(file_list);
    for (int i = 0; i < file_descriptor_capacity; i++) {
        ufs_close(i);
    }
    free(file_descriptors);
}

int
ufs_resize(int fd, size_t new_size) {
    if (fd < 0 || fd >= file_descriptor_capacity || file_descriptors[fd] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct filedesc *f = file_descriptors[fd];
    if (f->file->size < new_size) {
        while (f->file->size != new_size) {
            size_t cnt = BLOCK_SIZE - f->file->last_block->occupied;
            if (cnt > (new_size - f->file->size))
                cnt = new_size - f->file->size;
            if (f->file->size + cnt > MAX_FILE_SIZE) {
                ufs_error_code = UFS_ERR_NO_MEM;
                return -1;
            }
            if (f->file->last_block->memory == NULL) {
                f->file->last_block->memory = (char *) malloc(sizeof(char) * BLOCK_SIZE);
            }
            f->file->last_block->occupied += cnt;
            if (f->file->last_block->occupied == BLOCK_SIZE) {
                if (f->file->last_block->next == NULL) {
                    f->file->last_block->next = (struct block *) malloc(sizeof(struct block));
                    f->file->last_block->next->prev = f->file->last_block;
                    f->file->last_block->next->next = NULL;
                    f->file->last_block->next->occupied = 0;
                    f->file->last_block->next->memory = NULL;
                }
                f->file->last_block = f->file->last_block->next;
            }
        }
        f->file->size = new_size;
    } else if (f->file->size > new_size) {
        struct block *block = f->file->block_list;
        for (size_t i = 0; i < new_size / BLOCK_SIZE; i++) {
            block = block->next;
        }
        destroy_blocks(block->next);
        block->next = NULL;
        f->file->size = new_size;
        block->occupied = new_size % BLOCK_SIZE;
        for (int i = 0; i < file_descriptor_capacity; i++) {
            if (file_descriptors[i] != NULL && file_descriptors[i]->file == f->file) {
                size_t offset = 0;
                struct block *b = file_descriptors[i]->block;
                while (offset < new_size) {
                    if (block == b) {
                        file_descriptors[i]->block = block;
                        if ((size_t) file_descriptors[i]->offset > new_size % BLOCK_SIZE)
                            file_descriptors[i]->offset = new_size % BLOCK_SIZE;
                        break;
                    }
                    offset += BLOCK_SIZE;
                    b = b->next;
                }
                if (offset >= new_size) {
                    file_descriptors[i]->block = block;
                    file_descriptors[i]->offset = new_size % BLOCK_SIZE;
                }
            }
        }
    }
    ufs_error_code = UFS_ERR_NO_ERR;
    return 0;
}