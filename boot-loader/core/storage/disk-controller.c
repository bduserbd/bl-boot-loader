#include "include/export.h"
#include "include/string.h"
#include "core/include/storage/storage.h"
#include "core/include/memory/heap.h"

static struct bl_disk_controller *controller_list = NULL;

struct bl_disk_controller *
bl_disk_controller_match_type(bl_disk_controller_t type)
{
	struct bl_disk_controller *curr;

	curr = controller_list;
	while (curr) {
		/* Is this enough ? */
		if (type == curr->funcs->type)
			return curr;

		curr = curr->next;
	}

	return NULL;
}

void bl_disk_controller_register(struct bl_disk_controller *controller)
{
	controller->next = controller_list;
	controller_list = controller;
}
BL_EXPORT_FUNC(bl_disk_controller_register);

void bl_disk_controller_unregister(struct bl_disk_controller *controller)
{

}
BL_EXPORT_FUNC(bl_disk_controller_unregister);

