#include "vbe.h"
#include "include/string.h"
#include "core/include/loader/loader.h"
#include "core/include/video/video.h"
#include "core/include/video/fb.h"
#include "core/include/video/print.h"
#include "core/include/memory/heap.h"
#include "core/arch/i386/real-mode/include/bios.h"

BL_MODULE_NAME("VESA BIOS Extensions");

static bl_size_t vbe_video_memory_size = 0;

static int vbe_video_modes_count = 0;
static bl_uint16_t *vbe_video_modes = NULL;

static bl_uint16_t vbe_selected_mode_number;
static struct bl_vbe_mode_info_block vbe_selected_mode;

#if 0
static bl_status_t bl_vbe_scroll(int pixels)
{
	struct bl_bios_registers iregs, oregs;
	static s16 moved = 0;

	moved += pixels;
	if (moved < 0)
		moved = 0;

	bl_bios_init_registers(&iregs);
	iregs.ah = BL_VBE_BIOS_FUNCTION;
	iregs.al = BL_VBE_BIOS_SUBFUNCTION_GET_SET_DISPLAY_START;
	iregs.bh = 0x00;
	iregs.bl = 0x00;
	iregs.cx = 0;
	iregs.dx = moved;
	bl_bios_call(0x10, &iregs, &oregs);

	if ((oregs.eax & 0xffff) == BL_VBE_SUCCESS)
		return BL_STATUS_SUCCESS;
	else
		return BL_STATUS_FAILURE;
}
#endif

static bl_status_t bl_vbe_get_info(struct bl_video_info *video)
{
	if (!vbe_video_modes || !vbe_video_modes || !vbe_selected_mode_number)
		return BL_STATUS_UNSUPPORTED;

	video->width = vbe_selected_mode.x_resolution;
	video->height = vbe_selected_mode.y_resolution;
	video->pitch = vbe_selected_mode.bytes_per_scan_line;

	video->frame_buffer = vbe_selected_mode.physical_base_address;
	video->video_memory_size = vbe_video_memory_size;

	video->bits_per_pixel = vbe_selected_mode.bits_per_pixel;
	video->bytes_per_pixel = (video->bits_per_pixel + (video->bits_per_pixel & 0x7)) >> 3;

	video->red.position = vbe_selected_mode.red_field_position;
	video->red.mask_size = vbe_selected_mode.red_mask_size;
	video->green.position = vbe_selected_mode.green_field_position;
	video->green.mask_size = vbe_selected_mode.green_mask_size;
	video->blue.position = vbe_selected_mode.blue_field_position;
	video->blue.mask_size = vbe_selected_mode.blue_mask_size;
	video->reserved.position = vbe_selected_mode.reserved_field_position;
	video->reserved.mask_size = vbe_selected_mode.reserved_mask_size;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_vbe_set_display_start(bl_uint16_t x, bl_uint16_t y)
{
	struct bl_bios_registers iregs, oregs;

	bl_bios_init_registers(&iregs);
	iregs.ah = BL_VBE_BIOS_FUNCTION;
	iregs.al = BL_VBE_BIOS_SUBFUNCTION_GET_SET_DISPLAY_START;
	iregs.bh = 0x00;
	iregs.bl = 0x00;
	iregs.cx = x;
	iregs.dx = y;

	bl_bios_interrupt(0x10, &iregs, &oregs);

	if (oregs.ax == BL_VBE_SUCCESS)
		return BL_STATUS_SUCCESS;
	else
		return BL_STATUS_FAILURE;
}

static bl_status_t bl_vbe_set_mode(bl_uint16_t mode)
{
	struct bl_bios_registers iregs, oregs;

	bl_bios_init_registers(&iregs);
	iregs.ah = BL_VBE_BIOS_FUNCTION;
	iregs.al = BL_VBE_BIOS_SUBFUNCTION_SET_MODE_INFO;
	iregs.bx = (1 << 14) /* Linear frame buffer */ | (mode & 0x1ff);
	
	bl_bios_interrupt(0x10, &iregs, &oregs);

	if (oregs.eax == BL_VBE_SUCCESS)
		return BL_STATUS_SUCCESS;
	else
		return BL_STATUS_FAILURE;
}

static bl_status_t bl_vbe_get_mode_info_block(bl_uint16_t mode, struct bl_vbe_mode_info_block *info)
{
	struct bl_bios_registers iregs, oregs;

	if (mode < 0x100)
		return BL_STATUS_UNSUPPORTED;

	bl_bios_init_registers(&iregs);
	iregs.ah = BL_VBE_BIOS_FUNCTION;
	iregs.al = BL_VBE_BIOS_SUBFUNCTION_GET_MODE_INFO;
	iregs.cx = mode;
	iregs.es = BL_BIOS_PM_TO_RM_SEGMENT((bl_uint32_t)info);
	iregs.di = BL_BIOS_PM_TO_RM_OFFSET((bl_uint32_t)info);

	bl_bios_interrupt(0x10, &iregs, &oregs);

	if (oregs.ax == BL_VBE_SUCCESS)
		return BL_STATUS_SUCCESS;
	else
		return BL_STATUS_FAILURE;
}

#if 0
static bl_uint32_t bl_vbe_read_edid(struct bl_video_edid *edid)
{
	struct bl_bios_registers iregs, oregs;

	bl_bios_init_registers(&iregs);
	iregs.ah = BL_VBE_BIOS_FUNCTION;
	iregs.al = BL_VBE_BIOS_SUBFUNCTION_DDC;
	iregs.bl = 0x1; // Read EDID
	bl_bios_set_address_parameter(&iregs, (bl_addr_t)edid);

	bl_bios_call(0x10, &iregs, &oregs);
	return oregs.eax & 0xffff;
}
#endif

static bl_status_t bl_vbe_set_video(void)
{
	int i;
	int valid_mode;
	bl_status_t status;
	struct bl_vbe_mode_info_block info;

	if (!vbe_video_modes)
		return BL_STATUS_FAILURE;

	valid_mode = 0;

	for (i = 0; i < vbe_video_modes_count; i++) {
		status = bl_vbe_get_mode_info_block(vbe_video_modes[i], &info);
		if (status)
			continue;

		if (info.x_resolution != BL_VIDEO_DEFAULT_WIDTH ||
			info.y_resolution != BL_VIDEO_DEFAULT_HEIGHT)
			continue;

		if (!(info.mode_attributes & BL_VBE_MODE_ATTRIBUTES_HARDWARE_SUPPORT) ||
			!(info.mode_attributes & BL_VBE_MODE_ATTRIBUTES_COLOR_MODE) ||
			!(info.mode_attributes & BL_VBE_MODE_ATTRIBUTES_GRPHICS_MODE) ||
			!(info.mode_attributes & BL_VBE_MODE_ATTRIBUTES_LINEAR_FRAME_BUFFER))
			continue;

		if (info.memory_model != BL_VBE_MEMORY_MODEL_DIRECT_COLOR)
			continue;

		if (info.bits_per_pixel != 15 && info.bits_per_pixel != 16 &&
			info.bits_per_pixel != 24 && info.bits_per_pixel != 32)
			continue;

#if 0
		bl_print_hex(info.x_resolution);
		bl_print_str("X");
		bl_print_hex(info.y_resolution);
		bl_print_str(" ");
		bl_print_hex(info.red_mask_size);
		bl_print_str(" ");
		bl_print_hex(info.green_mask_size);
		bl_print_str(" ");
		bl_print_hex(info.blue_mask_size);
		bl_print_str(" ");
		bl_print_hex(info.reserved_mask_size);
		bl_print_str(" ");
		bl_print_hex(info.bits_per_pixel);
		bl_print_str(" ");
		bl_print_hex(info.physical_base_address);
		bl_print_str(" ");
		bl_print_hex(info.memory_model);
		bl_print_str(" ");
		bl_print_hex(info.bytes_per_scan_line * info.y_resolution);
		bl_print_str("\n");
#endif

		if (info.red_mask_size == BL_VIDEO_DEFAULT_RED_MASK_SIZE &&
			info.green_mask_size == BL_VIDEO_DEFAULT_GREEN_MASK_SIZE &&
			info.blue_mask_size == BL_VIDEO_DEFAULT_BLUE_MASK_SIZE &&
			info.reserved_mask_size == BL_VIDEO_DEFAULT_RESERVED_MASK_SIZE) {
			valid_mode = 1;
			break;
		}
	}

	if (!valid_mode)
		return BL_STATUS_UNSUPPORTED;

	vbe_selected_mode_number = vbe_video_modes[i];
	bl_memcpy(&vbe_selected_mode, &info, sizeof(struct bl_vbe_mode_info_block));

	status = bl_vbe_set_mode(vbe_selected_mode_number);
	if (status)
		return status;

	status = bl_vbe_set_display_start(0, 0);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_vbe_get_info_block(struct bl_vbe_info_block *info)
{
	struct bl_bios_registers iregs, oregs;

	bl_bios_init_registers(&iregs);
	iregs.ah = BL_VBE_BIOS_FUNCTION;
	iregs.al = BL_VBE_BIOS_SUBFUNCTION_GET_INFO;
	iregs.es = BL_BIOS_PM_TO_RM_SEGMENT((bl_uint32_t)info);
	iregs.di = BL_BIOS_PM_TO_RM_OFFSET((bl_uint32_t)info);

	bl_bios_interrupt(0x10, &iregs, &oregs);

	if (oregs.ax != BL_VBE_SUCCESS)
		return BL_STATUS_FAILURE;

	if (bl_memcmp(info->signature, BL_VBE_INFO_BLOCK_SIGNATURE, sizeof(info->signature)))
		return BL_STATUS_UNSUPPORTED;

	if (info->version != BL_VBE_VERSION_2_0 && info->version != BL_VBE_VERSION_3_0)
		return BL_STATUS_UNSUPPORTED;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_vbe_init(void)
{
	int i;
	bl_status_t status;
	bl_uint16_t *video_modes;
	struct bl_vbe_info_block info;

	status = bl_vbe_get_info_block(&info);
	if (status)
		return status;

	/* 64KB units. */
	vbe_video_memory_size = info.total_memory << 16; 

	/* Video modes list can be returned on the stack. Store them on the heap. */
	video_modes = (bl_uint16_t *)BL_BIOS_FAR_PTR_TO_ADDRESS(info.video_modes);
	if (video_modes[0] == BL_VBE_VIDEO_MODES_LIST_END)
		return BL_STATUS_UNSUPPORTED;

	for (i = 0; video_modes[i] != BL_VBE_VIDEO_MODES_LIST_END; i++) ;

	vbe_video_modes_count = i;

	vbe_video_modes = bl_heap_alloc(vbe_video_modes_count * sizeof(bl_uint16_t));
	if (!vbe_video_modes)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	for (i = 0; i < vbe_video_modes_count; i++)
		vbe_video_modes[i] = video_modes[i];

	return BL_STATUS_SUCCESS;
}

static void bl_vbe_uninit(void)
{
	if (vbe_video_modes) {
		bl_heap_free(vbe_video_modes, vbe_video_modes_count * sizeof(bl_uint16_t));

		vbe_video_modes_count = 0;
		vbe_video_modes = NULL;
	}
}

static struct bl_video_driver vbe = {
	.type = BL_VIDEO_TYPE_VBE_GRAPHICS,
	.init = bl_vbe_init,
	.uninit = bl_vbe_uninit,
	.set_video = bl_vbe_set_video,
	.get_info = bl_vbe_get_info,
};

BL_MODULE_INIT()
{
	bl_video_driver_register(&vbe);	
}

BL_MODULE_UNINIT()
{

}

