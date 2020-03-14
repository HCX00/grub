/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2020  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

#include <grub/term.h>

#define OPEN_MAX 256

static FILE *fd_table[OPEN_MAX] = { stdin, stdout, stderr };

static int high_water_mark = 2;

static unsigned random_seed = 42;

/* Convert an integer file descriptor to a FILE *; on failure, sets errno and
 * returns NULL.  Only valid on file descriptors previously returned from
 * file_to_fd. */
static FILE *fd_to_file(int fd)
{
    if (fd < 0 || fd >= OPEN_MAX)
        return NULL;
    return fd_table[fd];
}

/* Convert a FILE * to an integer file descriptor; on failure, sets errno and
 * returns -1.  Will handle files never-before assigned an fd.
 *
 * This is a linear search for simplicity, but high_water_mark keeps it
 * reasonable for small numbers of files. */
static int file_to_fd(FILE *file)
{
    int fd;
    int unused_fd = -1;
    for (fd = 0; fd <= high_water_mark; fd++) {
        if (fd_table[fd] == file)
            return fd;
        if (unused_fd == -1 && !fd_table[fd])
            unused_fd = fd;
    }
    if (unused_fd == -1) {
        if (high_water_mark == (OPEN_MAX - 1))
            return -1;
        unused_fd = ++high_water_mark;
    }
    fd_table[unused_fd] = file;
    return unused_fd;
}

/* Record file closure, to stop tracking it for file<->fd conversions. */
static void note_file_closure(FILE *file)
{
    int fd = file_to_fd(file);
    if (fd <= 2)
        return;
    fd_table[fd] = NULL;
    if (fd == high_water_mark)
        while (--high_water_mark >= 0)
            if (fd_table[high_water_mark])
                break;
}

int atoi(const char *str)
{
    grub_errno = GRUB_ERR_NONE;
    return grub_strtol(str, NULL, 10);
}

int fclose(FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream == stdin || stream == stdout || stream == stderr) {
        grub_printf("ERROR: attempt to close stdin, stdout, or stderr.\n");
        return -1;
    }
    note_file_closure(stream);
    return (grub_file_close(stream) == GRUB_ERR_NONE) ? 0 : EOF;
}

int feof(FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream == stdin || stream == stdout || stream == stderr)
        return 0;
    return stream->offset == stream->size;
}

int ferror(FILE *stream)
{
    (void)stream;
    grub_errno = GRUB_ERR_NONE;
    return 0;
}

int fflush(FILE *stream)
{
    (void)stream;
    grub_errno = GRUB_ERR_NONE;
    return 0;
}

int fgetc(FILE *stream)
{
    unsigned char c;
    grub_errno = GRUB_ERR_NONE;
    return fread(&c, 1, 1, stream) ? c : EOF;
}

char *fgets(char *s, int size, FILE *stream)
{
    char *ret = s;
    grub_errno = GRUB_ERR_NONE;
    while (--size) {
        int c = fgetc(stream);
        if (c == EOF) {
            if (s == ret)
                return NULL;
            break;
        }
        *s++ = c;
        if (c == '\n')
            break;
    }
    *s = '\0';
    return ret;
}

int fileno(FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    return file_to_fd(stream);
}

FILE *fopen(const char *path, const char *mode)
{
    grub_errno = GRUB_ERR_NONE;
    if (grub_strcmp(mode, "r") != 0 && grub_strcmp(mode, "rb") != 0) {
        grub_printf("ERROR: attempt to open a file with unsupported mode \"%s\"\n",
                    mode);
        return NULL;
    }
    return grub_file_open(path, GRUB_FILE_TYPE_SKIP_SIGNATURE);
}

int fprintf(FILE *stream, const char *format, ...)
{
    va_list args;
    int ret;
    grub_errno = GRUB_ERR_NONE;
    va_start(args, format);
    ret = vfprintf(stream, format, args);
    va_end(args);
    return ret;
}

int fputc(int c, FILE *stream)
{
    const char s[] = { (unsigned char)c, '\0' };
    grub_errno = GRUB_ERR_NONE;
    if (stream != stdout && stream != stderr) {
        grub_printf("ERROR: attempt to write to a file.\n");
        return EOF;
    }
    grub_xputs(s);
    return (unsigned char)c;
}

int fputs(const char *s, FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream != stdout && stream != stderr) {
        grub_printf("ERROR: attempt to write to a file.\n");
        return EOF;
    }
    grub_xputs(s);
    return 1;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    ssize_t read_return;
    grub_errno = GRUB_ERR_NONE;
    if (stream == stdout || stream == stderr) {
        grub_printf("ERROR: attempt to fread from stdout or stderr.\n");
        return 0;
    }
    if (stream == stdin) {
        size_t i, j;
        unsigned char *bytes = ptr;
        for (i = 0; i < nmemb; i++)
            for (j = 0; j < size; j++)
                *bytes++ = grub_getkey();
        return nmemb;
    }

    read_return = grub_file_read(stream, ptr, size * nmemb);
    if (read_return <= 0)
        return 0;
    return read_return / size;
}

int fseek(FILE *stream, long offset, int whence)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream == stdin || stream == stdout || stream == stderr) {
        grub_printf("ERROR: attempt to seek on stdin, stdout, or stderr.\n");
        return -1;
    }
    switch (whence)
    {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            offset += stream->offset;
            break;
        case SEEK_END:
            offset += stream->size;
            break;
        default:
            return -1;
    }
    return (grub_file_seek(stream, offset) == -1ULL) ? -1 : 0;
}

int fstat(int fd, struct stat *buf)
{
    grub_errno = GRUB_ERR_NONE;
    buf->st_mtime = 0;
    if (fd >= 0 && fd < 3) {
        buf->st_mode = S_IFCHR | 0777;
        buf->st_size = 0;
    } else {
        grub_file_t file = fd_to_file(fd);
        if (!file)
            return -1;
        buf->st_mode = S_IFREG | 0777;
        buf->st_size = grub_file_size(file);
    }
    return 0;
}

long ftell(FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream == stdin || stream == stdout || stream == stderr)
        return 0;
    return grub_file_tell(stream);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream != stdout && stream != stderr) {
        grub_printf("ERROR: attempt to write to a file.\n");
        return 0;
    }
    if (size > GRUB_INT_MAX || nmemb > GRUB_INT_MAX || (uint64_t)size * (uint64_t)nmemb > GRUB_INT_MAX) {
        grub_error(GRUB_ERR_OUT_OF_RANGE,
                   "ERROR: attempt to write more than 2GB to stdout or stderr.\n");
        return 0;
    }
    return grub_printf("%.*s", (int)(size * nmemb), (char *)ptr);
}

int isatty(int fd)
{
    grub_errno = GRUB_ERR_NONE;
    return fd >= 0 && fd < 3;
}

void iterate_directory(const char *dirname, int (*callback)(const char *filename, const struct grub_dirhook_info *info, void *hook_data),
			void *data)
{
    char *device_name;
    grub_device_t device;
    grub_errno = GRUB_ERR_NONE;
    device_name = grub_file_get_device_name(dirname);
    device = grub_device_open(device_name);
    if (device) {
        grub_fs_t fs = grub_fs_probe(device);
        if (fs)
            fs->fs_dir(device, dirname, callback, data);
        grub_device_close(device);
    }
    grub_free(device_name);
}

static const char *is_directory_filename;
static int is_directory_result;

static int is_directory_callback(const char *filename, const struct grub_dirhook_info *info, void *data __attribute__((unused)))
{
    if ((info->case_insensitive ? grub_strcasecmp : grub_strcmp)(is_directory_filename, filename) == 0) {
        is_directory_result = !!info->dir;
        return 1;
    }
    return 0;
}

int is_directory(const char *filename)
{
    char *basename;
    char *dirname;
    size_t i;
    char *copy;

    if (grub_strcmp(filename, "/") == 0)
        return 1;

    copy = grub_strdup(filename);
    if (!copy)
        return 0;
    dirname = copy;
    i = grub_strlen(dirname);
    while (i && dirname[i - 1] == '/')
        dirname[--i] = '\0';
    basename = grub_strrchr(dirname, '/');
    if (basename)
        *basename++ = '\0';
    else
        basename = (char *)"/";
    if (*dirname == '\0')
        dirname = (char *)"/";

    is_directory_filename = basename;
    is_directory_result = 0;
    iterate_directory(dirname, is_directory_callback, NULL);

    grub_free(copy);
    return is_directory_result;
}

struct lconv *localeconv(void)
{
    static char grouping[] = { CHAR_MAX };
    static struct lconv lconv = { .decimal_point = (char *)".",
                                  .thousands_sep = (char *)"",
                                  .grouping = grouping };
    grub_errno = GRUB_ERR_NONE;
    return &lconv;
}

off_t lseek(int fd, off_t offset, int whence)
{
    grub_file_t file;

    if (fd >= 0 && fd < 3)
    {
        grub_printf("Internal error: Python attempted to seek on stdin, stdout, or stderr.\n");
        return (off_t)-1;
    }

    file = fd_to_file(fd);
    if (!file)
        return (off_t)-1;
    grub_errno = GRUB_ERR_NONE;
    if (fseek(file, offset, whence) < 0)
        return (off_t)-1;
    return file->offset;
}

int rand(void)
{
    unsigned bit;    /* Must be 16bit to allow bit<<15 later in the code */
    unsigned lfsr = random_seed;
    /* taps: 16 15 13 4; feedback polynomial: x^16 + x^15 + x^13 + x^4 + 1 */
    bit  = ((lfsr >> 0) ^ (lfsr >> 1) ^ (lfsr >> 3) ^ (lfsr >> 12) ) & 1;
    lfsr =  (lfsr >> 1) | (bit << 31);

    return 0;
}

void rewind(FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    fseek(stream, 0L, SEEK_SET);
}

void setbuf(FILE *stream, char *buf)
{
    (void)stream;
    (void)buf;
    grub_errno = GRUB_ERR_NONE;
}

sighandler_t signal(int signum, sighandler_t handler)
{
    (void)signum;
    (void)handler;
    grub_errno = GRUB_ERR_NONE;
    return SIG_ERR;
}

void srand(unsigned seed)
{
    random_seed = seed;
    return;
}

int stat(const char *path, struct stat *buf)
{
    FILE *file;
    grub_errno = GRUB_ERR_NONE;
    file = grub_file_open(path, GRUB_FILE_TYPE_SKIP_SIGNATURE);
    if (file) {
        buf->st_size = grub_file_size(file);
        grub_file_close(file);
        buf->st_mode = S_IFREG | 0777;
    } else {
        if (grub_errno == GRUB_ERR_BAD_FILE_TYPE && is_directory(path)) {
            grub_errno = GRUB_ERR_NONE;
            buf->st_size = 0;
            buf->st_mode = S_IFDIR | 0777;
        } else {
            return -1;
        }
    }
    buf->st_mtime = 0;
    return 0;
}

char *strerror(int errnum)
{
    static char buf[sizeof("GRUB error 4294967296")];
    grub_errno = GRUB_ERR_NONE;
    grub_snprintf(buf, sizeof(buf), "GRUB error %u", errnum);
    return buf;
}

int ungetc(int c, FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream == stdout || stream == stderr) {
        grub_printf("ERROR: attempt to ungetc on stdout or stderr.\n");
        return EOF;
    }
    if (stream == stdin) {
        grub_printf("ERROR: attempt to ungetc on stdin.\n");
        return EOF;
    }
    if (stream->offset == 0) {
        grub_printf("ERROR: attempt to ungetc at the beginning of a file.\n");
        return EOF;
    }
    grub_file_seek(stream, stream->offset-1);
    if (fgetc(stream) != c) {
        grub_printf("ERROR: attempt to ungetc a character it didn't getc.\n");
        return EOF;
    }
    grub_file_seek(stream, stream->offset-1);
    return c;
}

int unlink(const char *pathname __attribute__((unused)))
{
    grub_errno = GRUB_ERR_NONE;
    grub_printf("ERROR: attempt to unlink a file.\n");
    return -1;
}

int vfprintf(FILE *stream, const char *format, va_list args)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream != stdout && stream != stderr) {
        grub_printf("ERROR: attempt to write to a file.\n");
        return -1;
    }
    return grub_vprintf(format, args);
}

int open(const char *pathname, int flags __attribute__((unused)))
{
  return file_to_fd(fopen(pathname, "r"));
}

int close(int fd)
{
  return fclose (fd_to_file(fd));
}

ssize_t read(int fd, void *buf, size_t count)
{
  return fread(buf, count, 1, fd_to_file(fd));
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
    grub_errno = GRUB_ERR_NONE;
    if (stream != stdin) {
        grub_printf("ERROR: attempt to getline from a file.\n");
        return -1;
    }
    *lineptr = grub_getline (0);
    *n = grub_strlen (*lineptr);
    return *n;
}

struct posix_test_parse_ctx
{
  int exist;
  struct grub_dirhook_info info;
  char *name;
};

/* A hook for iterating directories. */
static int
posix_find_file (const char *name,
                const struct grub_dirhook_info *info, void *data)
{
  struct posix_test_parse_ctx *ctx = data;

  if ((info->case_insensitive ?
         grub_strcasecmp (name, ctx->name)
       : grub_strcmp (name, ctx->name)) == 0)
  {
    ctx->info = *info;
    ctx->exist = 1;
    return 1;
  }
  return 0;
}

/* Check if file exists and fetch its information. */
static void
posix_get_fileinfo (const char *path, struct posix_test_parse_ctx *ctx)
{
  char *pathname;
  char *device_name;
  grub_fs_t fs;
  grub_device_t dev;
  char *tmp_path = grub_strdup (path);

  ctx->exist = 0;
  device_name = grub_file_get_device_name (tmp_path);
  dev = grub_device_open (device_name);
  if (! dev)
  {
    grub_free (device_name);
    return;
  }

  fs = grub_fs_probe (dev);
  if (! fs)
  {
    grub_free (device_name);
    grub_device_close (dev);
    return;
  }

  pathname = grub_strchr (tmp_path, ')');
  if (! pathname)
    pathname = tmp_path;
  else
    pathname++;

  /* Remove trailing '/'. */
  while (*pathname && pathname[grub_strlen (pathname) - 1] == '/')
    pathname[grub_strlen (pathname) - 1] = 0;

  /* Split into path and filename. */
  char *t;
  ctx->name = grub_strrchr (pathname, '/');
  if (! ctx->name)
  {
    t = grub_strdup ("/");
    ctx->name = pathname;
  }
  else
  {
    ctx->name++;
    t = grub_strdup (pathname);
    t[ctx->name - pathname] = 0;
  }

  /* It's the whole device. */
  if (! *pathname)
  {
    ctx->exist = 1;
    grub_memset (&ctx->info, 0, sizeof (ctx->info));
    /* Root is always a directory. */
    ctx->info.dir = 1;
  }
  else
    (fs->fs_dir) (dev, t, posix_find_file, ctx);

  if (t)
    grub_free (t);
  grub_device_close (dev);
  if (tmp_path)
    grub_free (tmp_path);
  grub_free (device_name);
}

static int
posix_file_exist (const char *path)
{
  struct posix_test_parse_ctx ctx = { .exist = 0};
  posix_get_fileinfo (path, &ctx);
  if (ctx.exist && !ctx.info.dir)
    return 1;
  else
    return 0;
}

int access(const char *pathname, int mode)
{
    if (mode & W_OK)
      return -1;
    if (posix_file_exist (pathname))
      return 0;
    else
      return -1;
}

