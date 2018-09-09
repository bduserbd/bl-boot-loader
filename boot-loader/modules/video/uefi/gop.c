#include "core/firmware/uefi/include/utils.h"
#include "core/include/loader/loader.h"
#include "core/include/video/video.h"
#include "core/include/video/fb.h"
#include "core/include/memory/heap.h"

BL_MODULE_NAME("EFI Graphics Output Protocol");

static struct efi_graphics_output_protocol *gop;
static efi_handle_t gop_handle;

/* Double buffer page flipping. */
static void *double_buffer;

static int bl_gop_get_bpp_bit_mask(struct efi_graphics_output_mode_information *info)
{
	int i;
	bl_uint32_t num;

	num = info->pixel_information.red_mask | info->pixel_information.green_mask |
		info->pixel_information.blue_mask | info->pixel_information.reserved_mask;

	for (i = 31; i >= 0; i--)
		if (num & (1 << i))
			return i + 1;

	return 0;
}

static void bl_gop_get_color_mask(efi_uint32_t mask, struct bl_video_color_mask *color)
{
	int i, j;

	for (i = 31; i >= 0; i--)
		if (mask & (1 << i))
			break;

	for (j = i; j >= 0; j--)
		if ((mask & (1 << j)) == 0)
			break;
	
	color->position = j + 1;
	color->mask_size = i - j;
}

static int bl_gop_get_bpp(struct efi_graphics_output_mode_information *info)
{
	switch (info->pixel_format) {
	case EFI_PIXEL_RED_GREEN_BLUE_RESERVED_8_BIT_PER_COLOR:
	case EFI_PIXEL_BLUE_RED_GREEN_RESERVED_8_BIT_PER_COLOR:
		return 32; 

	case EFI_PIXEL_BIT_MASK:
		return bl_gop_get_bpp_bit_mask(info);

	default:
		return 0;
	}
}

static bl_status_t bl_gop_get_info(struct bl_video_info *video)
{
	if (!gop || !gop_handle)
		return BL_STATUS_INVALID_PARAMETERS;

	video->width = gop->mode->info->horizontal_resolution;
	video->height = gop->mode->info->vertical_resolution;

	video->frame_buffer = gop->mode->frame_buffer_base;
	video->frame_buffer_size = gop->mode->frame_buffer_size;

	video->bytes_per_pixel = sizeof(struct efi_graphics_output_blt_pixel);
	video->bits_per_pixel = video->bytes_per_pixel << 3;
	video->pitch = video->width * video->bytes_per_pixel;

	bl_gop_get_color_mask(gop->mode->info->pixel_information.red_mask, &video->red);
	bl_gop_get_color_mask(gop->mode->info->pixel_information.green_mask, &video->green);
	bl_gop_get_color_mask(gop->mode->info->pixel_information.blue_mask, &video->blue);
	video->reserved.position = 24;
	video->reserved.mask_size = 8;

	return BL_STATUS_SUCCESS;
}

static bl_status_t bl_gop_read_edid(void)
{
	efi_status_t status;
	efi_guid_t active_edid_guid = EFI_EDID_ACTIVE_PROTOCOL_GUID,
		discovered_edid_guid = EFI_EDID_DISCOVERED_PROTOCOL_GUID;
	struct efi_edid_active_protocol *active_edid;
	struct efi_edid_discovered_protocol *discovered_edid;

	status = bl_efi_open_protocol(gop_handle, &active_edid_guid, (void **)&active_edid,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_FAILED(status)) {
		status = bl_efi_open_protocol(gop_handle, &discovered_edid_guid, (void **)&discovered_edid,
			EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		if (EFI_FAILED(status))
			goto _exit;
	}

_exit:
	return status == EFI_SUCCESS ? BL_STATUS_SUCCESS : BL_STATUS_FAILURE;
}

static bl_status_t bl_gop_set_video(void)
{
	efi_uint32_t i, mode = -1;
	efi_status_t status;
	int bpp, depth = 0;
	struct efi_graphics_output_mode_information *info;
	struct bl_video_info vid_info;

	if (!gop || !gop_handle)
		return BL_STATUS_INVALID_PARAMETERS;

	/* TODO : Check preferred mode using EDID. */
	(void)bl_gop_read_edid();

	for (i = 0; i < gop->mode->max_mode; i++) {
		status = gop->query_mode(gop, i, sizeof(struct efi_graphics_output_mode_information),
			&info);
		if (EFI_FAILED(status))
			continue;

		if (info->pixel_format != EFI_PIXEL_BIT_MASK)
			continue;

		if (info->horizontal_resolution == BL_VIDEO_DEFAULT_WIDTH &&
			info->vertical_resolution == BL_VIDEO_DEFAULT_HEIGHT) {
		
			bpp = bl_gop_get_bpp(info);
			if (!bpp)
				continue;

			if (!depth || bpp > depth) {
				mode = i;
				depth = bpp;
			}
		}
	}

	if (gop->set_mode(gop, mode))
		return BL_STATUS_FAILURE;

	bl_gop_get_info(&vid_info);

	double_buffer = bl_heap_alloc(BL_VIDEO_DEFAULT_WIDTH * BL_VIDEO_DEFAULT_HEIGHT *
		sizeof(struct efi_graphics_output_blt_pixel));
	if (!double_buffer)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	if (bl_fb_init(BL_FB_DOUBLE_BUFFER_RAM, &vid_info, double_buffer))
		return BL_STATUS_FAILURE;
	
	return BL_STATUS_SUCCESS;
}

static void bl_gop_buffer_to_video(bl_uint32_t x, bl_uint32_t y, bl_uint32_t w, bl_uint32_t h)
{
	if (!gop || !gop_handle || !double_buffer)
		return;

	gop->blt(gop, double_buffer, EFI_BLT_BUFFER_TO_VIDEO, x, y, x, y,
		w, h, BL_VIDEO_DEFAULT_WIDTH * sizeof(struct efi_graphics_output_blt_pixel));
}

static void bl_gop_switch_double_buffer(void)
{
	if (!gop || !gop_handle || !double_buffer)
		return;

	bl_gop_buffer_to_video(0, 0, BL_VIDEO_DEFAULT_WIDTH, BL_VIDEO_DEFAULT_HEIGHT);
}

static bl_status_t bl_gop_init(void)
{
	efi_status_t status;
	efi_uintn_t i, no_handles;
	efi_guid_t gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	efi_handle_t *gop_handles_buffer;

	status = bl_efi_locate_handle_buffer(EFI_BY_PROTOCOL, &gop_guid,
		NULL, &no_handles, &gop_handles_buffer);
	if (EFI_FAILED(status))
		return BL_STATUS_GRAPHICS_PROTOCOL_UNSUPPORTED;

	for (i = 0; i < no_handles; i++) {
		status = bl_efi_open_protocol(gop_handles_buffer[i], &gop_guid, (void **)&gop,
				EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		if (EFI_FAILED(status))
			continue;

		gop_handle = gop_handles_buffer[i];
		break;
	}

	return status == EFI_SUCCESS ? BL_STATUS_SUCCESS : BL_STATUS_FAILURE;
}

static void bl_gop_uninit(void)
{

}

static struct bl_video_driver bl_gop = {
	.type = BL_VIDEO_TYPE_GOP_GRAPHICS,
	.init = bl_gop_init,
	.uninit = bl_gop_uninit,
	.set_video = bl_gop_set_video,
	.get_info = bl_gop_get_info,
	.buffer_to_video = bl_gop_buffer_to_video,
	.switch_double_buffer = bl_gop_switch_double_buffer,
};

BL_MODULE_INIT()
{
	bl_video_driver_register(&bl_gop);
}

BL_MODULE_UNINIT()
{

}

