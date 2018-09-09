#include "core/include/usb/usb.h"
#include "core/include/memory/heap.h"

int bl_usb_traversal_is_empty(struct bl_usb_traversal *traversal)
{
        return traversal->head == traversal->tail;
}

void bl_usb_traversal_push(struct bl_usb_traversal *traversal, struct bl_usb_hub *hub)
{
        if (traversal->tail < 127 - 1)
                traversal->hubs[traversal->tail++] = hub;
}

struct bl_usb_hub *bl_usb_traversal_pop(struct bl_usb_traversal *traversal)
{
        if (traversal->head < traversal->tail)
                return traversal->hubs[traversal->head++];

        return NULL;
}

struct bl_usb_traversal *bl_usb_traversal_init(void)
{
        int i;
        struct bl_usb_traversal *traversal;

        traversal = bl_heap_alloc(sizeof(struct bl_usb_traversal));
        if (!traversal)
                return NULL;

        traversal->head = traversal->tail = 0;

        for (i = 0; i < 127; i++)
                traversal->hubs[i] = NULL;

        return traversal;
}

void bl_usb_traversal_uninit(struct bl_usb_traversal *traversal)
{
	bl_heap_free(traversal, sizeof(struct bl_usb_traversal));
}

