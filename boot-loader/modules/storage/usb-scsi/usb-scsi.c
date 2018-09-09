#include "usb-scsi.h"
#include "include/string.h"
#include "core/include/loader/loader.h"
#include "core/include/storage/storage.h"
#include "core/include/usb/usb.h"
#include "core/include/memory/heap.h"

BL_MODULE_NAME("USB Bulk-Only Transport");

static void bl_usb_scsi_bbb_init_cbw(struct bl_usb_bbb_cbw *cbw, bl_uint8_t lun, int direction,
	void *cmd, bl_uint8_t cb_length, bl_size_t length)
{
	static bl_uint32_t tag = 0;

	bl_memset(cbw, 0, sizeof(struct bl_usb_bbb_cbw));

	cbw->signature = BL_USB_BBB_CBW_SIGNATURE;

	cbw->tag = ++tag;

	cbw->data_transfer_length = length;
	cbw->flags = direction ? 0x0 : (1 << 7);

	cbw->lun = lun;	

	cbw->cb_length = cb_length;
	bl_memcpy(cbw->cmd, cmd, cbw->cb_length);
}

static bl_status_t bl_usb_scsi_bbb_execute_command(struct bl_usb_device *device, int direction,
	void *cmd, bl_uint8_t cb_length, void *buf, bl_size_t length)
{
	struct bl_usb_bbb_cbw cbw;
	struct bl_usb_bbb_csw csw;

	bl_usb_scsi_bbb_init_cbw(&cbw, 0, direction, cmd, cb_length, length);

	bl_usb_bulk_transfer(device, 1, &cbw, sizeof(struct bl_usb_bbb_cbw));
	bl_usb_bulk_transfer(device, direction, buf, length);

	bl_usb_bulk_transfer(device, 0, &csw, sizeof(struct bl_usb_bbb_csw));

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_usb_scsi_sense_data(struct bl_usb_device *device)
{
	struct bl_scsi_request_sense cmd;
	struct bl_scsi_request_sense_data data;

	bl_memset(&cmd, 0, sizeof(struct bl_scsi_request_sense));

	cmd.opcode = BL_SCSI_COMMAND_SENSE_DATA;
	cmd.lun = (0 << 5);
	cmd.allocation_length = sizeof(struct bl_scsi_request_sense_data);

	return bl_usb_scsi_bbb_execute_command(device, 0, &cmd, sizeof(struct bl_scsi_request_sense),
		&data, sizeof(struct bl_scsi_request_sense_data));
}

static bl_status_t bl_usb_scsi_read10(struct bl_usb_device *device, bl_uint8_t *buf,
	bl_uint64_t lba, bl_uint64_t sectors, bl_size_t length)
{
	bl_status_t status;
	struct bl_scsi_read10 cmd;

	bl_memset(&cmd, 0, sizeof(struct bl_scsi_read10));

	cmd.opcode = BL_SCSI_COMMAND_READ10;
	cmd.lba = cpu_to_be32(lba & 0xffffffff);
	cmd.tranfser_length = cpu_to_be16(sectors & 0xffff);

	status = bl_usb_scsi_bbb_execute_command(device, 0, &cmd, sizeof(struct bl_scsi_read10),
		buf, length);
	if (status)
		return status;

	status = bl_usb_scsi_sense_data(device);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

// TODO: Fix this.
#if 0
static bl_status_t bl_usb_scsi_read6(struct bl_usb_device *device, bl_uint8_t *buf,
	bl_uint64_t lba, bl_uint64_t sectors, bl_size_t length)
{
	bl_status_t status;
	struct bl_scsi_read6 cmd;
	bl_uint16_t be_sectors;

	bl_memset(&cmd, 0, sizeof(struct bl_scsi_read6));

	cmd.opcode = BL_SCSI_COMMAND_READ6;

	be_sectors = cpu_to_be16(sectors & 0xffff);
	cmd.lba[2] = (be_sectors >> 16) & 0x1f;
	cmd.lba[1] = (be_sectors >> 8) & 0xff;
	cmd.lba[0] = (be_sectors >> 0) & 0xff;

	cmd.tranfser_length = sectors & 0xff;

	status = bl_usb_scsi_bbb_execute_command(device, 0, &cmd, sizeof(struct bl_scsi_read6),
		buf, length);
	if (status)
		return status;

	status = bl_usb_scsi_sense_data(device);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}
#endif

static bl_status_t bl_usb_scsi_read(struct bl_storage_device *disk, bl_uint8_t *buf,
	bl_uint64_t lba, bl_uint64_t sectors)
{
	bl_status_t status;
	struct bl_usb_device *device;

	device = disk->data;

	//if (lba < (1 << 21) && sectors < (1 << 8)) {
	//	status = bl_usb_scsi_read6(device, buf, lba, sectors, sectors * disk->sector_size);
	//	if (status)
	//		return status;
	//} else {
		status = bl_usb_scsi_read10(device, buf, lba, sectors, sectors * disk->sector_size);
		if (status)
			return status;
	//}

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_usb_scsi_get_size(struct bl_usb_device *device, bl_uint64_t *sector_count,
	bl_size_t *sector_size)
{
	bl_status_t status;
	struct bl_scsi_read_capacity10 cmd1;
	struct bl_scsi_read_capacity10_data data1;
	struct bl_scsi_read_capacity16 cmd2;
	struct bl_scsi_read_capacity16_data data2;

	/* First try the READ CAPACITY (10) command. */
	bl_memset(&cmd1, 0, sizeof(struct bl_scsi_read_capacity10));

	cmd1.opcode = BL_SCSI_COMMAND_READ_CAPACITY10;
	cmd1.lun = (0 << 5);

	status = bl_usb_scsi_bbb_execute_command(device, 0, &cmd1, sizeof(struct bl_scsi_read_capacity10),
		&data1, sizeof(struct bl_scsi_read_capacity10_data));
	if (status)
		return status;

	status = bl_usb_scsi_sense_data(device);
	if (status)
		return status;

	/* If the storage device capacity is large (really large!) then
	   use the READ CAPACITY (16) command. */
	if (data1.lba < 0xffffffff) {
		*sector_count = be32_to_cpu(data1.lba) + 1;
		*sector_size = be32_to_cpu(data1.block_size);

		return BL_STATUS_SUCCESS;
	}

	bl_memset(&cmd2, 0, sizeof(struct bl_scsi_read_capacity16));

	cmd2.opcode = BL_SCSI_COMMAND_READ_CAPACITY16;
	cmd2.lun = (0 << 5);
	cmd2.allocation_length = cpu_to_be32(sizeof(struct bl_scsi_read_capacity16_data));

	status = bl_usb_scsi_bbb_execute_command(device, 0, &cmd2, sizeof(struct bl_scsi_read_capacity16),
		&data2, sizeof(struct bl_scsi_read_capacity16_data));
	if (status)
		return status;

	status = bl_usb_scsi_sense_data(device);
	if (status)
		return status;

	*sector_count = be64_to_cpu(data2.lba) + 1;
	*sector_size = be32_to_cpu(data2.block_size);

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_usb_scsi_inquiry(struct bl_usb_device *device)
{
	bl_status_t status;
	struct bl_scsi_inquiry cmd;
	struct bl_scsi_inquiry_data data;

	bl_memset(&cmd, 0, sizeof(struct bl_scsi_inquiry));

	cmd.opcode = BL_SCSI_COMMAND_INQUIRY;
	cmd.lun = (0 << 5);
	cmd.page_code = 0;
	cmd.reserved0 = 0;
	cmd.allocation_length = sizeof(struct bl_scsi_inquiry_data);
	cmd.control = 0;

	status = bl_usb_scsi_bbb_execute_command(device, 0, &cmd, sizeof(struct bl_scsi_inquiry),
		&data, sizeof(struct bl_scsi_inquiry_data));
	if (status)
		return status;

	status = bl_usb_scsi_sense_data(device);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_usb_scsi_validate_device(struct bl_usb_device *device)
{
	int i;
	struct bl_usb_interface *interface;

	interface = &device->interfaces[device->specific.storage.interface];

	/* Mass storage class. */
	if (interface->descriptor.interface_class != 0x8 ||
		interface->descriptor.interface_subclass != 0x6)
		return BL_STATUS_USB_INVALID_DEVICE;

	/* Used protocol. */
	switch (interface->descriptor.interface_protocol) {
	case 0x50:
		device->specific.storage.type = BL_USB_STORAGE_BBB;
		break;

	default:
		return BL_STATUS_USB_INVALID_DEVICE;
	}

	/* IN/OUT endpoints. */
	for (i = 0; i < interface->descriptor.num_endpoints; i++) {
		if (!device->specific.storage.in_endp.descriptor)
			if ((interface->endpoints[i].endpoint_address & (1 << 7)) &&
				(interface->endpoints[i].attributes == 0x2)) {
				device->specific.storage.in_endp.toggle = 0;
				device->specific.storage.in_endp.descriptor = &interface->endpoints[i];
			}

		if (!device->specific.storage.out_endp.descriptor)
			if (!(interface->endpoints[i].endpoint_address & (1 << 7)) &&
			    (interface->endpoints[i].attributes == 0x2)) {
				device->specific.storage.out_endp.toggle = 0;
				device->specific.storage.out_endp.descriptor = &interface->endpoints[i];
			}
	}

	if (!device->specific.storage.in_endp.descriptor ||
		!device->specific.storage.out_endp.descriptor)
		return BL_STATUS_USB_INVALID_DEVICE;

	return BL_STATUS_SUCCESS;
}

static void bl_usb_scsi_get_luns(struct bl_usb_device *device)
{
	bl_uint8_t max_lun;

	bl_usb_control_transfer(device, &max_lun, 
		BL_USB_SETUP_REQUEST_TYPE_DEVICE_TO_HOST |
		BL_USB_SETUP_REQUEST_TYPE_CLASS |
		BL_USB_SETUP_REQUEST_TYPE_RECIPIENT_INTERFACE,
		0xfe, 0x0, device->specific.storage.interface, 0x1);

	device->specific.storage.luns = max_lun;
}

static bl_status_t bl_usb_scsi_get_info(struct bl_storage_device *disk)
{
	bl_status_t status;
	struct bl_usb_device *device;

	device = disk->data;

	status = bl_usb_scsi_validate_device(device);
	if (status)
		return status;

	bl_usb_scsi_get_luns(device);

	status = bl_usb_scsi_inquiry(device);
	if (status)
		return status;

	status = bl_usb_scsi_get_size(device, &disk->sector_count, &disk->sector_size);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static struct bl_disk_controller_functions bl_usb_scsi_funcs = {
	.type = BL_DISK_CONTROLLER_TYPE_USB_SCSI,
	.read = bl_usb_scsi_read,
	.get_info = bl_usb_scsi_get_info,
};

BL_MODULE_INIT()
{
	struct bl_disk_controller *bl_usb_scsi;

	bl_usb_scsi = bl_heap_alloc(sizeof(struct bl_disk_controller));
	if (!bl_usb_scsi)
		return;

	bl_usb_scsi->funcs = &bl_usb_scsi_funcs;
	bl_usb_scsi->data = NULL;
	bl_usb_scsi->next = NULL;

	bl_disk_controller_register(bl_usb_scsi);
}

BL_MODULE_UNINIT()
{

}

