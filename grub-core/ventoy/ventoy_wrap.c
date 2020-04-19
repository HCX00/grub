/******************************************************************************
 * ventoy.c 
 *
 * Copyright (c) 2020, longpanda <admin@ventoy.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/device.h>
#include <grub/term.h>
#include <grub/partition.h>
#include <grub/file.h>

#ifdef GRUB_MACHINE_EFI
#include <grub/efi/api.h>
#include <grub/efi/efi.h>
#include <grub/cpu/efi/memory.h>
#endif

#include <grub/ventoy.h>
#include "ventoy_def.h"
#include "ventoy_wrap.h"

#ifdef GRUB_MACHINE_EFI
void *
grub_efi_allocate_iso_buf (grub_size_t size)
{
  return grub_efi_allocate_pages_real (GRUB_EFI_MAX_USABLE_ADDRESS,
                                       GRUB_EFI_BYTES_TO_PAGES (size),
                                       GRUB_EFI_ALLOCATE_ANY_PAGES,
                                       GRUB_EFI_RUNTIME_SERVICES_DATA);
}
#endif
