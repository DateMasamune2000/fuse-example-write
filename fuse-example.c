#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#define MAXSIZE 64

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fuse.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *filepath = "/file";
static const char *filename = "file";
char filecontent[MAXSIZE] = "I'm the content of the only file available there\n";
static int filesize;

static int getattr_callback(const char *path, struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if (strcmp(path, filepath) == 0) {
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = filesize;
    return 0;
  }

  return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi) {
  (void)offset;
  (void) fi;
  (void) path;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  filler(buf, filename, NULL, 0);

  return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  printf("Open called on %s\n", path);
  if (strcmp(path, filepath) == 0) {
    fi->fh = 1;
    return 0;
  } else {
    return -ENOENT;
  }
}

static int release_callback(const char *path, struct fuse_file_info *fi) {
  (void) path;
  if (fi->fh == 1) {
    return 0;
  } else {
    return -ENOENT;
  }
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi) {
  (void)path; // done because hte "path" argument is not used

  printf("Read called\n");
  if (fi->fh == 1) {
    size_t len = filesize;
    if ((size_t) offset >= len) {
      return 0;
    }

    if (offset + size > len) {
      memcpy(buf, filecontent + offset, len - offset);
      return len - offset;
    }

    memcpy(buf, filecontent + offset, size);
    return size;
  }

  return -ENOENT;
}

static int write_callback(const char *path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi) {
  (void)path; // done because the "path" argument is not used
  if (fi->fh == 1) {
    int wbytes = (size + offset >= filesize ? filesize - offset : size);
    strncpy(offset + filecontent, buf, wbytes);
    filesize = offset + size;
    return wbytes;
  } else {
    return -ENOENT;
  }
}

static int truncate_callback(const char *path, off_t size) {
  printf("truncate called on %s\n", path);
  if (strcmp(path, filepath) != 0) {
    return -ENOENT;
  }
  return 0;
}

void *init_callback(struct fuse_conn_info *conn) {
  (void) conn;
  filesize = strlen(filecontent);
  return NULL;
}

static struct fuse_operations fuse_example_operations = {
    .getattr = getattr_callback,
    .open = open_callback,
    .read = read_callback,
    .readdir = readdir_callback,
    .write = write_callback,
    .truncate = truncate_callback,
    .release = release_callback,
    .init = init_callback
};

int main(int argc, char *argv[]) {
  return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
