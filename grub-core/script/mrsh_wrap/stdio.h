/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009, 2010  Free Software Foundation, Inc.
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

#ifndef GRUB_POSIX_STDIO_H
#define GRUB_POSIX_STDIO_H	1

#include <grub/misc.h>
#include <grub/file.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct grub_file FILE;

#define EOF    -1

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define S_IFMT  0170000
#define S_IFREG 0100000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 0070
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IRWXO 0007
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001

static inline int
vprintf ( const char *fmt, va_list args ) {
  return grub_vprintf (fmt, args);
};

static inline int
printf (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vprintf (fmt, ap);
  va_end (ap);

  return ret;
}

static inline int
snprintf (char *str, grub_size_t n, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vsnprintf (str, n, fmt, ap);
  va_end (ap);

  return ret;
}

static inline int
sprintf (char *str, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vsnprintf (str, GRUB_UINT_MAX, fmt, ap);
  va_end (ap);

  return ret;
}

int atoi(const char *str);
void clearerr(FILE *stream);
int fclose(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int fflush(FILE *stream);
int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fileno(FILE *stream);
FILE *fopen(const char *path, const char *mode);
int fprintf(FILE *stream, const char *format, ...);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);

long ftell(FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int is_directory(const char *filename);
void iterate_directory(const char *dirname, int (*callback)(const char *filename, const struct grub_dirhook_info *info, void *hook_data), void *data);
struct lconv *localeconv(void);
off_t lseek(int fd, off_t offset, int whence);

void rewind(FILE *stream);
void setbuf(FILE *stream, char *buf);
typedef void (*sighandler_t)(int);
#define SIG_ERR ((sighandler_t) -1)
#define SIG_DFL ((sighandler_t) 0)
#define SIG_IGN ((sighandler_t) 1)
sighandler_t signal(int signum, sighandler_t handler);

char *strerror(int errnum);
int ungetc(int c, FILE *stream);
int unlink(const char *pathname);
int vfprintf(FILE *stream, const char *format, va_list args);

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

static inline void perror(const char *s)
{
  if (s)
    grub_printf ("ERROR: %s\n", s);
  else
    grub_printf ("ERROR\n");
}

#endif
