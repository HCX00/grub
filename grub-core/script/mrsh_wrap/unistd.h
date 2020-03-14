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

#ifndef GRUB_POSIX_UNISTD_H
#define GRUB_POSIX_UNISTD_H	1

#include <sys/types.h>
#include <time.h>
#include <grub/env.h>

#define stdin ((FILE *)1)
#define stdout ((FILE *)2)
#define stderr ((FILE *)3)

#define STDIN_FILENO 1

typedef unsigned int mode_t;

struct stat
{
  mode_t st_mode;
  off_t st_size;
  time_t st_mtime;
};

int isatty (int fd);

int stat (const char *path, struct stat *buf);
int fstat (int fd, struct stat *buf);

#define O_RDONLY  1
#define O_CLOEXEC 1

int open(const char *pathname, int flags);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);

typedef int uid_t;
typedef int pid_t;
typedef int gid_t;

static inline uid_t getuid(void)
{
  return 0;
}

static inline uid_t geteuid(void)
{
  return 0;
}

static inline pid_t getpid(void)
{
  return 14530529;
}

static inline pid_t getppid(void)
{
  return 4760904;
}

static inline gid_t getgid(void)
{
  return 0;
}

static inline gid_t getegid(void)
{
  return 0;
}

static inline char *getcwd(char *buf, size_t size)
{
  const char *cwd = grub_env_get ("root");
  if (!cwd)
    return NULL;
  if (size < grub_strlen (cwd) + 4)
    return NULL;
  grub_snprintf (buf, size, "(%s)/", cwd);
  return buf;
}

#define F_OK  0     /* test for existence of file */
#define X_OK  0x01  /* test for execute or search permission */
#define W_OK  0x02  /* test for write permission */
#define R_OK  0x04  /* test for read permission */

int access(const char * pathname, int mode);

#endif
