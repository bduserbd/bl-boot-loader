#ifndef UEFI_H
#define UEFI_H

#include "include/types.h"

/* More EFI related types. */
typedef u8		efi_uint8_t;
typedef u16		efi_uint16_t;
typedef u32		efi_uint32_t;
typedef u64		efi_uint64_t;
typedef unsigned long	efi_uintn_t;

#define EFI_TRUE	1
#define EFI_FALSE	0
typedef efi_uint8_t	efi_boolean_t;

typedef efi_uint16_t	efi_wchar_t;

typedef efi_uint64_t	efi_physical_address_t, efi_virtual_address_t;

typedef struct guid	efi_guid_t;

struct efi_system_table;

/* EFI Status. */
typedef unsigned long	efi_status_t;

// Status code types.
#define EFI_ERROR_STATUS(status)	\
	(((efi_status_t)1 << (sizeof(efi_status_t) * 8 - 1)) | (status))

#define EFI_ERROR_RESERVED_STATUS(status)	\
	(((efi_status_t)0xc << (sizeof(efi_status_t) * 8 - 4)) | (status))

#define EFI_WARNING_STATUS(status)	(status)

// List of status codes.
#define EFI_SUCCESS	0

#define EFI_LOAD_ERROR		EFI_ERROR_STATUS(1)
#define EFI_INVALID_PARAMETER	EFI_ERROR_STATUS(2)
#define EFI_UNSUPPORTED		EFI_ERROR_STATUS(3)
#define EFI_BAD_BUFFER_SIZE	EFI_ERROR_STATUS(4)
#define EFI_BUFFER_TOO_SMALL	EFI_ERROR_STATUS(5)
#define EFI_NOT_READY		EFI_ERROR_STATUS(6)
#define EFI_DEVICE_ERROR	EFI_ERROR_STATUS(7)
#define EFI_WRITE_PROTECTED	EFI_ERROR_STATUS(8)
#define EFI_OUT_OF_RESOURCES	EFI_ERROR_STATUS(9)
#define EFI_VOLUME_CORRUPTED	EFI_ERROR_STATUS(10)
#define EFI_VOLUME_FULL		EFI_ERROR_STATUS(11)
#define EFI_NO_MEDIA		EFI_ERROR_STATUS(12)
#define EFI_MEDIA_CHANGED	EFI_ERROR_STATUS(13)
#define EFI_NOT_FOUND		EFI_ERROR_STATUS(14)

#define EFI_INVALID_ELF_FILE		EFI_ERROR_RESERVED_STATUS(1)
#define EFI_INVALID_ELF_FILE_SIZE	EFI_ERROR_RESERVED_STATUS(2)
#define EFI_NO_EXPORT_SYMBOLS_FOUND	EFI_ERROR_RESERVED_STATUS(3)

// Some status macros.
#define EFI_SUCCEEDED(status)	((status) == EFI_SUCCESS)
#define EFI_FAILED(status)	(!EFI_SUCCEEDED(status))

/* EFI Handle. */
typedef void *efi_handle_t;

/* EFI EDID discovered protocol. */
#define EFI_EDID_DISCOVERED_PROTOCOL_GUID	\
	{ 0x1c0c34f6, 0xd380, 0x41fa, { 0xa0, 0x49, 0x8a, 0xd0, 0x6c, 0x1a, 0x66, 0xaa } }

struct efi_edid_discovered_protocol {
	efi_uint32_t size_of_edid;
	efi_uint8_t *edid;
} __attribute__((packed));

/* EFI EDID active protocol. */
#define EFI_EDID_ACTIVE_PROTOCOL_GUID	\
	{ 0xbd8c1056, 0x9f36, 0x44ec, { 0x92, 0xa8, 0xa6, 0x33, 0x7f, 0x81, 0x79, 0x86 } }

struct efi_edid_active_protocol {
	efi_uint32_t size_of_edid;
	efi_uint8_t *edid;
} __attribute__((packed));

/* EFI Graphics pixel format. */
typedef enum {
	EFI_PIXEL_RED_GREEN_BLUE_RESERVED_8_BIT_PER_COLOR,
	EFI_PIXEL_BLUE_RED_GREEN_RESERVED_8_BIT_PER_COLOR,
	EFI_PIXEL_BIT_MASK,
	EFI_PIXEL_BLT_ONLY,
	EFI_PIXEL_FORMAT_MAX,
} efi_graphics_pixel_format_t; 

/* EFI Pixel bitmask. */
struct efi_pixel_bitmask {
	efi_uint32_t red_mask;
	efi_uint32_t green_mask;
	efi_uint32_t blue_mask;
	efi_uint32_t reserved_mask;
} __attribute__((packed));

/* EFI Graphics output mode information. */
struct efi_graphics_output_mode_information {
	efi_uint32_t version;
	efi_uint32_t horizontal_resolution;
	efi_uint32_t vertical_resolution;
	efi_graphics_pixel_format_t pixel_format;
	struct efi_pixel_bitmask pixel_information;
	efi_uint32_t pixels_per_scan_line;
} __attribute__((packed));

/* EFI Graphics output protocol mode. */
struct efi_graphics_output_protocol_mode {
	efi_uint32_t max_mode;
	efi_uint32_t mode;
	struct efi_graphics_output_mode_information *info;
	efi_uintn_t size_of_info;
	efi_physical_address_t frame_buffer_base;
	efi_uintn_t frame_buffer_size;
} __attribute__((packed));

/* EFI Graphics output blt pixel. */
struct efi_graphics_output_blt_pixel {
	efi_uint8_t blue;
	efi_uint8_t green;
	efi_uint8_t red;
	efi_uint8_t reserved;
} __attribute__((packed));

/* EFI Graphics blt operation type. */
typedef enum {
	EFI_BLT_VIDEO_FILL,
	EFI_BLT_VIDEO_TO_BLT_BUFFER,
	EFI_BLT_BUFFER_TO_VIDEO,
	EFI_BLT_VIDEO_TO_VIDEO,
	EFI_GRAPHICS_OUTPUT_BLT_OPERATION_MAX,
} efi_graphics_output_blt_operation_t;

/* EFI Graphics output protocol. */
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID	\
	{ 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a} }

struct efi_graphics_output_protocol {
	efi_status_t (*query_mode)(struct efi_graphics_output_protocol *, efi_uint32_t,
		efi_uintn_t, struct efi_graphics_output_mode_information **);

	efi_status_t (*set_mode)(struct efi_graphics_output_protocol *, efi_uint32_t);

	efi_status_t (*blt)(struct efi_graphics_output_protocol *,
		struct efi_graphics_output_blt_pixel *,
		efi_graphics_output_blt_operation_t,
		efi_uintn_t, efi_uintn_t,
		efi_uintn_t, efi_uintn_t,
		efi_uintn_t, efi_uintn_t,
		efi_uintn_t);

	struct efi_graphics_output_protocol_mode *mode;
} __attribute__((packed));

/* EFI Time. */
struct efi_time {
	efi_uint16_t year;	// 1900 - 9999
	efi_uint8_t month;	// 1 - 12
	efi_uint8_t day;	// 1 - 31
	efi_uint8_t hour;	// 0 - 23
	efi_uint8_t minute;	// 0 - 59
	efi_uint8_t second;	// 0 - 59
	efi_uint8_t pad1;
	efi_uint32_t nanosecond;// 0 - 999,999,999
	efi_uint16_t time_zone;	// -1440 to 1440 or 2047
	efi_uint8_t daylight;
	efi_uint8_t pad2;
} __attribute__((packed));

/* EFI Table header (located at the beginning of each EFI table). */
struct efi_table_header {
	efi_uint64_t signature;
	efi_uint32_t revision;
	efi_uint32_t header_size;
	efi_uint32_t crc32;
	efi_uint32_t reserved;
} __attribute__((packed));

/* EFI Allocation type. */
typedef enum {
	EFI_ALLOCATE_ANY_PAGES,
	EFI_ALLOCATE_MAX_ADDRESS,
	EFI_ALLOCATE_ADDRESS,
	EFI_MAX_ALLOCATE_TYPE,
} efi_allocate_type_t;

/* EFI Memory regions type. */
typedef enum {
	EFI_RESERVED_MEMORY_TYPE,
	EFI_LOADER_CODE,
	EFI_LOADER_DATA,
	EFI_BOOT_SERVICES_CODE,
	EFI_BOOT_SERVICES_DATA,
	EFI_RUNTIME_SERVICES_CODE,
	EFI_RUNTIME_SERVICES_DATA,
	EFI_CONVENTIONAMEMORY,
	EFI_UNUSABLE_MEMORY,
	EFI_ACPI_RECLAIM_MEMORY,
	EFI_ACPI_MEMORY_NVS,
	EFI_MEMORY_MAPPED_IO,
	EFI_MEMORY_MAPPED_IO_PORT_SPACE,
	EFI_PACODE,
	EFI_PERSISTENT_MEMORY,
	EFI_MAX_MEMORY_TYPE,
} efi_memory_type_t;

/* EFI Memory descriptor. */
struct efi_memory_descriptor {
	efi_uint32_t type;
	efi_uint8_t padding1[4];
	efi_physical_address_t physical_address;
	efi_virtual_address_t virtual_address;
	efi_uint64_t number_of_pages;
	efi_uint64_t attribute;
	efi_uint8_t padding8[8];
} __attribute__((packed));

/* EFI Loaded image protocol. */
#define EFI_LOADED_IMAGE_PROTOCOL_GUID	\
	{ 0x5b1b31a1, 0x9562, 0x11d2, { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

struct efi_loaded_image_protocol {
	efi_uint32_t revision;
	efi_handle_t parent_handle;
	struct efi_system_table *system_table;

	/* Source location of the image. */
	efi_handle_t device_handle;
	void *file_path;
	void *reserved;

	/* Image's load options. */
	efi_uint32_t load_options_size;
	void *load_options;

	/* Location where image was loaded. */
	void *image_base;
	efi_uint64_t image_size;
	efi_memory_type_t image_code_type;
	efi_memory_type_t image_data_type;
	void *unload;
} __attribute__((packed));

/* EFI Open modes. */
#define EFI_FILE_MODE_READ	0x0000000000000001
#define EFI_FILE_MODE_WRITE	0x0000000000000002
#define EFI_FILE_MODE_CREATE	0x0000000000000000

/* EFI File attributes. */
#define EFI_FILE_READ_ONLY	0x0000000000000001
#define EFI_FILE_HIDDEN		0x0000000000000002
#define EFI_FILE_SYSTEM		0x0000000000000004
#define EFI_FILE_RESERVED	0x0000000000000008
#define EFI_FILE_DIRECTORY	0x0000000000000010
#define EFI_FILE_ARCHIVE	0x0000000000000020
#define EFI_FILE_VALID_ATTR	0x0000000000000037

/* EFI File info. */
#define EFI_FILE_INFO_ID_GUID	\
	{ 0x09576e92, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_FILE_INFO_MAX_NAME_LEN	32 // Our limit.

struct efi_file_info {
	efi_uint64_t size;
	efi_uint64_t file_size;
	efi_uint64_t physical_size;
	struct efi_time create_time;
	struct efi_time last_access_time;
	struct efi_time modification_time;
	efi_uint64_t attribute;
	efi_wchar_t file_name[EFI_FILE_INFO_MAX_NAME_LEN];
} __attribute__((packed));

/* EFI File protocol. */
struct efi_file_protocol {
	efi_uint64_t revision;

	efi_status_t (*open)(struct efi_file_protocol *, struct efi_file_protocol **,
		efi_wchar_t *, efi_uint64_t, efi_uint64_t);

	efi_status_t (*close)(struct efi_file_protocol *);

	void *delete;

	efi_status_t (*read)(struct efi_file_protocol *, efi_uintn_t *, void *);

	void *write;
	void *get_position;
	void *set_position;

	efi_status_t (*get_info)(struct efi_file_protocol *, efi_guid_t *,
		efi_uintn_t *, void *);

	void *set_info;
	void *flush;
	void *open_ex;
	void *read_ex;
	void *write_ex;
	void *flush_ex;
} __attribute__((packed));

/* EFI Simple file system protocol. */
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID	\
	{ 0x0964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

struct efi_simple_file_system_protocol {
	efi_uint64_t revision;

	efi_status_t (*open_volume)(struct efi_simple_file_system_protocol *,
		struct efi_file_protocol **);

} __attribute__((packed));

/* EFI Locate search type. */
typedef enum {
	EFI_ALL_HANDLES,
	EFI_BY_REGISTER_NOTIFY,
	EFI_BY_PROTOCOL,
} efi_locate_search_type_t;

/* EFI Open protocol attributes. */
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL	0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL		0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL		0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER	0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRICER		0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE		0x00000020

/* EFI Event type. */
#define EFI_EVT_TIMER		0x80000000
#define EFI_EVT_RUNTIME		0x40000000

#define EFI_EVT_NOTIFY_WAIT	0x00000100
#define EFI_EVT_NOTIFY_SIGNAL	0x00000200

#define EFI_EVT_SIGNAL_EXIT_BOOT_SERVICES	0x00000201
#define EFI_EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE	0x60000202

/* EFI TPL - Task priority level. */
typedef efi_uintn_t	efi_tpl_t;

#define EFI_TPL_APPLICATION	4
#define EFI_TPL_CALLBACK	8
#define EFI_TPL_NOTIFY		16
#define EFI_TPL_HIGH_LEVEL	31

/* EFI Event type. */
typedef void *efi_event_t;

/* EFI Event notify function. */
typedef efi_status_t (*efi_event_notify_t)(efi_event_t, void *);

/* EFI Timer delay. */
typedef enum {
	EFI_TIMER_CANCEL,
	EFI_TIMER_PERIODIC,
	EFI_TIMER_RELATIVE,
} efi_timer_delay_t;

/* EFI Boot services table. */
#define EFI_BOOT_SERVICES_SIGNATURE	0x56524553544f4f42

struct efi_boot_services_table {
	struct efi_table_header hdr;

	/* Task priority services. */
	void *raise_tpl;
	void *restore_tpl;

	/* Memory services. */
	efi_status_t (*allocate_pages)(efi_allocate_type_t, efi_memory_type_t,
		efi_uintn_t, efi_physical_address_t *);

	efi_status_t (*free_pages)(efi_physical_address_t, efi_uintn_t);

	efi_status_t (*get_memory_map)(efi_uintn_t *, struct efi_memory_descriptor *,
		efi_uintn_t *, efi_uintn_t *, efi_uint32_t *);

	efi_status_t (*allocate_pool)(efi_memory_type_t, efi_uintn_t, void **);

	efi_status_t (*free_pool)(void *);

	/* Event & timer services. */
	efi_status_t (*create_event)(efi_uint32_t, efi_tpl_t, efi_event_notify_t,
		void *, efi_event_t *);

	efi_status_t (*set_timer)(efi_event_t, efi_timer_delay_t, efi_uint64_t);

	efi_status_t (*wait_for_event)(efi_uintn_t, efi_event_t *, efi_uintn_t *);

	void *signal_event;

	efi_status_t (*close_event)(efi_event_t);

	void *check_event;

	/* Protocol handler services. */
	void *install_protocol_interface;
	void *reinstall_protocol_interface;
	void *uninstall_protocol_interface;

	efi_status_t (*handle_protocol)(efi_handle_t, efi_guid_t *, void **);

	void *reserved;
	void *register_protocol_notify;
	void *locate_handle;
	void *locate_device_path;
	void *install_configuration_table;

	/* Image services. */
	void *load_image;
	void *start_image;
	void *exit;
	void *unload_image;

	efi_status_t (*exit_boot_services)(efi_handle_t, efi_uintn_t);

	/* Miscellaneous services. */
	void *get_next_monotonic_count;

	efi_status_t (*stall)(efi_uintn_t);

	void *set_watchdog_timer;

	/* DriverSupport services. */
	void *connect_controller;
	void *disconnect_controller;

	/* Open and close protocol services. */
	efi_status_t (*open_protocol)(efi_handle_t, efi_guid_t *, void **,
		efi_handle_t, efi_handle_t, efi_uint32_t);

	void *close_protocol;
	void *open_protocoinformation;

	/* Library services. */
	void *protocols_per_handle;

	efi_status_t (*locate_handle_buffer)(efi_locate_search_type_t, efi_guid_t *,
		void *, efi_uintn_t *, efi_handle_t **);

	efi_status_t (*locate_protocol)(efi_guid_t *, void *, void **);

	void *install_multiple_protocol_interfaces;
	void *uninstall_multiple_protocol_interfaces;

	/* 32-bit CRC services. */
	void *calculate_crc32;

	/* Miscellaneous services. */
	void (*copy_mem)(void *, void *, efi_uintn_t);

	void (*set_mem)(void *, efi_uintn_t, efi_uint8_t);

	void *create_event_ex;
} __attribute__((packed));

/* EFI Runtime services table. */
#define EFI_RUNTIME_SERVICES_SIGNATURE	0x56524553544e5552

struct efi_runtime_services_table {
	struct efi_table_header hdr;

	/* Time services. */
	void *get_time;
	void *set_time;
	void *get_wakeup_time;
	void *set_wakeup_time;

	/* Virtual memory services. */
	void *set_virtuaaddress_map;
	void *convert_pointer;

	/* Variable services. */
	void *get_variable;
	void *get_next_variable_name;
	void *set_variable;

	/* Miscellaneous services. */
	void *get_next_high_monotonic_count;
	void *reset_system;

	/* UEFI 2.0 capsule services. */
	void *update_capsule;
	void *query_capsule_capabilities;

	/* Miscellaneous UEFI 2.0 service. */
	void *query_variable_info;
} __attribute__((packed));

/* EFI Configuration table. */
struct efi_configuration_table {
	efi_guid_t vendor_guid;
	efi_uint32_t vendor_table;
} __attribute__((packed));

/* EFI simple text output protocol. */
struct efi_simple_text_output_protocol {
	efi_status_t (*reset)(struct efi_simple_text_output_protocol *, efi_boolean_t);

	efi_status_t (*output_string)(struct efi_simple_text_output_protocol *,
		efi_wchar_t *);

	efi_status_t (*test_string)(struct efi_simple_text_output_protocol *,
		efi_wchar_t *);

	void *query_mode;
	void *set_mode;
	void *set_attribute;
	void *clear_screen;
	void *set_cursor_position;
	void *enable_cursor;
	void *mode;
} __attribute__((packed));

/* EFI System table. */
#define EFI_SYSTEM_TABLE_SIGNATURE	0x5453595320494249

struct efi_system_table {
	struct efi_table_header hdr;
	efi_wchar_t *firmware_vendor;
	efi_uint32_t firmware_revision;

	efi_handle_t console_in_handle;
	void *con_in;

	efi_handle_t console_out_handle;
	struct efi_simple_text_output_protocol *con_out;

	efi_handle_t standard_error_handle;
	void *std_err;

	struct efi_runtime_services_table *runtime_services;
	struct efi_boot_services_table *boot_services;

	efi_uint32_t number_of_table_entries;
	struct efi_configuration_table *configuration_table;
} __attribute__((packed));

#endif

