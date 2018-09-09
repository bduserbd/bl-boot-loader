#include "core/include/video/video.h"
#include "core/include/video/fb.h"
#include "core/include/memory/heap.h"

static struct bl_video_driver *bl_video_drivers_list = NULL;
static struct bl_video_driver *video = NULL;

bl_uint32_t bl_video_get_width(void)
{
	return bl_fb_get_width();
}

bl_uint32_t bl_video_get_height(void)
{
	return bl_fb_get_height();
}

bl_uint32_t bl_video_get_bpp(void)
{
	return bl_fb_get_bpp();
}

void bl_video_fill_rectangle(bl_video_color_t color, bl_uint32_t x, bl_uint32_t y,
	bl_uint32_t w, bl_uint32_t h)
{
	bl_fb_fill_rectangle(color, x, y, w, h);
}

void bl_video_update_from_double_buffer(void)
{
	bl_fb_update_from_double_buffer();
}

static bl_status_t bl_video_type_supported(bl_video_t t)
{
	int i;
	static const bl_video_t vids[] = {
#ifdef FIRMWARE_BIOS
		BL_VIDEO_TYPE_VBE_GRAPHICS,
#elif FIRMWARE_UEFI
		BL_VIDEO_TYPE_GOP_GRAPHICS,
#else
		BL_VIDEO_TYPE_INVALID,
#endif
	};

	for (i = 0; i < sizeof(vids) / sizeof(bl_video_t); i++)
		if (t == vids[i])
			return BL_STATUS_SUCCESS;

	return BL_STATUS_UNSUPPORTED; 
}

bl_status_t bl_video_driver_probe(void)
{
	bl_status_t status;
	struct bl_video_driver *driver;

	driver = bl_video_drivers_list;
	while (driver) {
		if (!bl_video_type_supported(driver->type))
			if (!driver->init()) {
				if (!driver->set_video()) {
					video = driver;
					break;
				}

				driver->uninit();
			}

		driver = driver->next;
	}

	if (!video)
		return BL_STATUS_GRAPHICS_NO_PROTOCOLS;

	status = bl_fb_init(video);
	if (status)
		return status;

	return BL_STATUS_SUCCESS;
}

void bl_video_driver_register(struct bl_video_driver *video_driver)
{
	if (video_driver) {
		video_driver->next = bl_video_drivers_list;
		bl_video_drivers_list = video_driver;
	}
}
BL_EXPORT_FUNC(bl_video_driver_register);

void bl_video_driver_unregister(struct bl_video_driver *video_driver)
{

}
BL_EXPORT_FUNC(bl_video_driver_unregister);

