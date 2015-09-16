#ifndef UFDISK_H
#define UFDISK_H

int ufdisk_need_create_partition(void);
int ufdisk_create_partition(void);
void ufdisk_umount_all(void);

#endif
