/* Header file for mounting/unmount Linux filesystems.
   Copyright (C) 1996,1997,1998,1999,2000,2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* This is taken from /usr/include/linux/fs.h.  */

#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H	1

#include <features.h>
#include <sys/ioctl.h>


#define BLOCK_SIZE_BITS	10
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)


/* These are the fs-independent mount-flags: up to 16 flags are
   supported  */
enum
{
  MS_RDONLY = 1,		/* Mount read-only.  */
  MS_NOSUID = 2,		/* Ignore suid and sgid bits.  */
  MS_NODEV = 4,			/* Disallow access to device special files.  */
  MS_NOEXEC = 8,		/* Disallow program execution.  */
  MS_SYNCHRONOUS = 16,		/* Writes are synced at once.  */
  MS_REMOUNT = 32,		/* Alter flags of a mounted FS.  */
  MS_MANDLOCK = 64,		/* Allow mandatory locks on an FS.  */
  S_WRITE = 128,		/* Write on file/directory/symlink.  */
  S_APPEND = 256,		/* Append-only file.  */
  S_IMMUTABLE = 512,		/* Immutable file.  */
  MS_NOATIME = 1024,		/* Do not update access times.  */
  MS_NODIRATIME = 2048,		/* Do not update directory access times.  */
  MS_BIND = 4096,		/* Bind directory at different place.  */
};

/* Flags that can be altered by MS_REMOUNT  */
#define MS_RMT_MASK (MS_RDONLY|MS_SYNCHRONOUS|MS_MANDLOCK|MS_NOATIME \
		     |MS_NODIRATIME)


/* Magic mount flag number. Has to be or-ed to the flag values.  */

#define MS_MGC_VAL 0xc0ed0000	/* Magic flag number to indicate "new" flags */
#define MS_MGC_MSK 0xffff0000	/* Magic flag number mask */


/* The read-only stuff doesn't really belong here, but any other place
   is probably as bad and I don't want to create yet another include
   file.  */

#define BLKROSET   _IO(0x12,93)	/* set device read-only (0 = read-write) */
#define BLKROGET   _IO(0x12,94)	/* get read-only status (0 = read_write) */
#define BLKRRPART  _IO(0x12,95)	/* re-read partition table */
#define BLKGETSIZE _IO(0x12,96)	/* return device size /512 (long *arg) */
#define BLKFLSBUF  _IO(0x12,97)	/* flush buffer cache */
#define BLKRASET   _IO(0x12,98)	/* set read ahead for block device */
#define BLKRAGET   _IO(0x12,99)	/* get current read ahead setting */
#define BLKFRASET  _IO(0x12,100)/* set filesystem (mm/filemap.c) read-ahead */
#define BLKFRAGET  _IO(0x12,101)/* get filesystem (mm/filemap.c) read-ahead */
#define BLKSECTSET _IO(0x12,102)/* set max sectors per request (ll_rw_blk.c) */
#define BLKSECTGET _IO(0x12,103)/* get max sectors per request (ll_rw_blk.c) */
#define BLKSSZGET  _IO(0x12,104)/* get block device sector size */
#define BLKBSZGET  _IOR(0x12,112,size_t)
#define BLKBSZSET  _IOW(0x12,113,size_t)
#define BLKGETSIZE64 _IOR(0x12,114,size_t)	/* return device size in bytes (u64 *arg) */


/* Possible value for FLAGS parameter of `umount2'.  */
enum
{
  MNT_FORCE = 1			/* Force unmounting.  */
#define MNT_FORCE MNT_FORCE
};


__BEGIN_DECLS

/* Mount a filesystem.  */
extern int mount (__const char *__special_file, __const char *__dir,
		  __const char *__fstype, unsigned long int __rwflag,
		  __const void *__data) __THROW;

/* Unmount a filesystem.  */
extern int umount (__const char *__special_file) __THROW;

/* Unmount a filesystem.  Force unmounting if FLAGS is set to MNT_FORCE.  */
extern int umount2 (__const char *__special_file, int __flags) __THROW;

__END_DECLS

#endif /* _SYS_MOUNT_H */
