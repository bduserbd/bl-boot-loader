#ifndef BL_STORAGE_H
#define BL_STORAGE_H

#include "include/bl-types.h"
#include "include/error.h"

#define BL_STORAGE_SECTOR_SIZE	512

typedef enum {
	BL_DISK_CONTROLLER_TYPE_PATA,
	BL_DISK_CONTROLLER_TYPE_AHCI,
	BL_DISK_CONTROLLER_TYPE_USB_SCSI,
} bl_disk_controller_t;

struct bl_storage_device;

struct bl_disk_controller_functions {
	bl_disk_controller_t type;

	bl_status_t (*read)(struct bl_storage_device *, bl_uint8_t *,
		bl_uint64_t, bl_uint64_t);

	/* For disk controllers that are not associated with a device (like USB SCSI). */
	bl_status_t (*get_info)(struct bl_storage_device *);
};

struct bl_disk_controller {
	struct bl_disk_controller_functions *funcs;

	void *data;

	struct bl_disk_controller *next;
};

struct bl_partition {
	bl_uint64_t lba;
	bl_uint64_t sectors;

	struct bl_partition *next;
	// struct bl_partition *children;
};

struct bl_partition_table_functions {
	bl_status_t (*iterate)(struct bl_storage_device *); 

	struct bl_partition_table_functions *next;
};

typedef enum {
	BL_STORAGE_TYPE_HARD_DRIVE,
	BL_STORAGE_TYPE_USB_DRIVE,
} bl_storage_t;

struct bl_storage_device {
	bl_storage_t type;

	char *product;
	char *serial_number;

	bl_size_t sector_size;
	bl_uint64_t sector_count;
	struct bl_partition *partitions;

	struct bl_disk_controller *controller;
	void *data;

	struct bl_storage_device *next;
};

struct bl_storage_device *bl_storage_device_get(int);
struct bl_partition *bl_storage_partition_get(struct bl_storage_device *, int);

bl_status_t bl_storage_probe(void);

void bl_storage_dump_devices(void);

bl_status_t bl_storage_device_read(struct bl_storage_device *, bl_uint8_t *,
	bl_uint64_t, bl_size_t, bl_offset_t);

void bl_storage_device_register(struct bl_storage_device *);
void bl_storage_device_unregister(struct bl_storage_device *);

struct bl_disk_controller *bl_disk_controller_match_type(bl_disk_controller_t);

bl_status_t bl_partition_table_probe(struct bl_storage_device *);

void bl_partition_table_register(struct bl_partition_table_functions *);
void bl_partition_table_unregister(struct bl_partition_table_functions *);

void bl_disk_controller_register(struct bl_disk_controller *);
void bl_disk_controller_unregister(struct bl_disk_controller *);

#endif

