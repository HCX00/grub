/*
 * Copyright (C) 2014 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * @file
 *
 * EFI file system access
 *
 */

#include <stdio.h>
#include <string.h>
#include <wimboot.h>
#include <maplib.h>
#include <vfat.h>
#include <wimpatch.h>
#include <wimfile.h>

/**
 * Read from file
 *
 * @v file              Virtual file
 * @v data              Data buffer
 * @v offset            Offset
 * @v len               Length
 */
void
disk_read_file (struct vfat_file *vfile, void *data, size_t offset, size_t len)
{
  grub_file_t file = vfile->opaque;
  /* Set file position */
  grub_file_seek (file, offset);
  /* Read from file */
  grub_file_read (file, data, len);
}

void
mem_read_file (struct vfat_file *file, void *data, size_t offset, size_t len)
{
  memcpy (data, ((char *)file->opaque + offset), len);
}

void
grub_extract (void)
{
  struct grub_vfatdisk_file *f = NULL;
  for (f = vfat_file_list; f; f = f->next)
  {
    if (f->addr)
      add_file (f->name, f->addr, f->file->size, mem_read_file);
    else
      add_file (f->name, f->file, f->file->size, disk_read_file);
  }
  /* Check that we have a boot file */
  if (! bootmgfw)
    die ("FATAL: bootmgfw.efi not found\n");
}

void
grub_wimboot_init (int argc, char *argv[])
{
  int i;
  struct grub_vfatdisk_file *wim = NULL;
  void *addr = NULL;

  for (i = 0; i < argc; i++)
  {
    const char *fname = argv[i];
    char *file_name = NULL;
    grub_file_t file = 0;
    if (grub_memcmp (argv[i], "@:", 2) == 0 || 
        grub_memcmp (argv[i], "m:", 2) == 0)
    {
      const char *ptr, *eptr;
      ptr = argv[i] + 2;
      eptr = grub_strchr (ptr, ':');
      if (eptr)
      {
        file_name = grub_strndup (ptr, eptr - ptr);
        if (!file_name)
          die ("file name error.\n");
        fname = eptr + 1;
      }
    }
    file = grub_file_open (fname,
                GRUB_FILE_TYPE_LINUX_INITRD | GRUB_FILE_TYPE_NO_DECOMPRESS);
    if (!file)
      die ("bad file.\n");
    if (!file_name)
      file_name = grub_strdup (file->name);
    /* Skip wim file */
    if (!wim && strlen(file_name) > 4 &&
        strcasecmp ((file_name + (strlen (file_name) - 4)), ".wim") == 0)
    {
      wim = malloc (sizeof (struct grub_vfatdisk_file));
      wim->name = grub_strdup (file_name);
      wim->file = file;
      if (argv[i][0] == 'm')
      {
        addr = NULL;
        addr = grub_malloc (file->size);
        if (!addr)
          die ("out of memory.\n");
        grub_printf ("Loading %s ...\n", file->name);
        grub_file_read (file, addr, file->size);
        grub_printf ("Add: (mem)%p+%ld -> %s\n",
                   addr, (unsigned long) file->size, file_name);
        wim->addr = addr;
      }
      else
        wim->addr = NULL;
      wim->next = NULL;
      grub_free (file_name);
      continue;
    }
    if (argv[i][0] == 'm')
      append_vfat_list (file, file_name, addr, 1);
    else
      append_vfat_list (file, file_name, NULL, 0);
    grub_free (file_name);
  }
  if (wim)
  {
    struct grub_vfatdisk_file *f = vfat_file_list;
    while (f && f->next)
    {
      f = f->next;
    }
    f->next = wim;
  }
}
