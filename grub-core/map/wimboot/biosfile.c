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

/** bootmgr.exe path within WIM */
static const wchar_t bootmgr_path[] = L"\\Windows\\Boot\\PXE\\bootmgr.exe";

/** bootmgr.exe file */
struct vfat_file *bootmgr;
void *bootmgr_exe_data;

/** Minimal length of embedded bootmgr.exe */
#define BOOTMGR_MIN_LEN 16384

/**
 * Add embedded bootmgr.exe extracted from bootmgr
 *
 * @v data    File data
 * @v len    Length
 * @ret file    Virtual file, or NULL
 *
 * bootmgr.exe is awkward to obtain, since it is not available as a
 * standalone file on the installation media, or on an installed
 * system, or in a Windows PE image as created by WAIK or WADK.  It
 * can be extracted from a typical boot.wim image using ImageX, but
 * this requires installation of the WAIK/WADK/wimlib.
 *
 * A compressed version of bootmgr.exe is contained within bootmgr,
 * which is trivial to obtain.
 */
static struct vfat_file *
add_bootmgr (const void *data, size_t len)
{
  const uint8_t *compressed;
  size_t offset;
  size_t compressed_len;
  ssize_t (* decompress) (const void *data, size_t len, void *buf);
  ssize_t decompressed_len;
  size_t padded_len;

  /* Look for an embedded compressed bootmgr.exe on a paragraph
   * boundary.
   */
  for (offset = BOOTMGR_MIN_LEN; offset < (len - BOOTMGR_MIN_LEN); offset += 0x10)
  {
    /* Initialise checks */
    decompress = NULL;
    compressed = (data + offset);
    compressed_len = (len - offset);
    /* Check for an embedded LZNT1-compressed bootmgr.exe.
     * Since there is no way for LZNT1 to compress the
     * initial "MZ" bytes of bootmgr.exe, we look for this
     * signature starting three bytes after a paragraph
     * boundary, with a preceding tag byte indicating that
     * these two bytes would indeed be uncompressed.
     */
    if (((compressed[0x02] & 0x03) == 0x00) &&
        (compressed[0x03] == 'M') && (compressed[0x04] == 'Z'))
    {
      DBG ("...checking for LZNT1-compressed bootmgr.exe at +0x%llx\n", offset);
      decompress = lznt1_decompress;
    }
    /* Check for an embedded XCA-compressed bootmgr.exe.
     * The bytes 0x00, 'M', and 'Z' will always be
     * present, and so the corresponding symbols must have
     * a non-zero Huffman length.  The embedded image
     * tends to have a large block of zeroes immediately
     * beforehand, which we check for.  It's implausible
     * that the compressed data could contain substantial
     * runs of zeroes, so we check for that too, in order
     * to eliminate some common false positive matches.
     */
    if (((compressed[0x00] & 0x0f) != 0x00) &&
        ((compressed[0x26] & 0xf0) != 0x00) &&
        ((compressed[0x2d] & 0x0f) != 0x00) &&
        (is_empty_pgh (compressed - 0x10)) &&
        (! is_empty_pgh ((compressed + 0x400))) &&
        (! is_empty_pgh ((compressed + 0x800))) &&
        (! is_empty_pgh ((compressed + 0xc00))))
    {
      DBG ("...checking for XCA-compressed bootmgr.exe at +0x%llx\n", offset);
      decompress = xca_decompress;
    }
    /* If we have not found a possible bootmgr.exe, skip
     * to the next paragraph.
     */
    if (! decompress)
      continue;

    /* Find length of decompressed image */
    decompressed_len = decompress (compressed, compressed_len, NULL);
    if (decompressed_len < 0)
    {
      /* May be a false positive signature match */
      continue;
    }
    bootmgr_exe_data = malloc (decompressed_len);
    if (!bootmgr_exe_data)
      return NULL;

    /* Prepend decompressed image to initrd */
    DBG ("...extracting embedded bootmgr.exe\n");
    padded_len = ((decompressed_len + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
    decompress (compressed, compressed_len, bootmgr_exe_data);

    /* Add decompressed image */
    return vdisk_add_file ("bootmgr.exe", bootmgr_exe_data, decompressed_len,
                           mem_read_file);
  }

  DBG ("...no embedded bootmgr.exe found\n");
  return NULL;
}

/**
 * File handler
 *
 * @v name    File name
 * @v data    File data
 * @v len    Length
 * @ret rc    Return status code
 */
int add_file (const char *name, void *data, size_t len,
              void (* read) (struct vfat_file *file,
                             void *data, size_t offset, size_t len))
{
  struct vfat_file *vfile;

  /* Store file */
  vfile = vfat_add_file (name, data, len, read);

  /* Check for special-case files */
  if (strcasecmp (name, "bootmgr.exe") == 0)
  {
    DBG ( "...found bootmgr.exe\n" );
    bootmgr = vfile;
  }
  else if (strcasecmp (name, "bootmgr") == 0)
  {
    DBG ("...found bootmgr\n");
    if ((! bootmgr) && (bootmgr = add_bootmgr (data, len)))
    {
      DBG ("...extracted bootmgr.exe\n");
    }
  }
  else if (strcasecmp ((name + strlen (name) - 4), ".wim") == 0)
  {
    DBG ("...found WIM file %s\n", name);
    vfat_patch_file (vfile, patch_wim);
    if ((! bootmgr) &&
        (bootmgr = wim_add_file (vfile, wimboot_cmd.index, bootmgr_path,
         L"bootmgr.exe")))
    {
      DBG ("...extracted bootmgr.exe from WIM\n");
    }
  }
  return 0;
}
