#ifndef BL_SHELL_COMMAND_H
#define BL_SHELL_COMMAND_H

#include "include/bl-types.h"

#define BL_SHELL_BUFFER_LENGTH	32

void bl_command_setup(void);

int bl_last_result;

void bl_command_run(const char *);

int bl_command_help(int, char *argv[]);
int bl_command_true(int, char *argv[]);
int bl_command_false(int, char *argv[]);
int bl_command_last_result(int, char *argv[]);
int bl_command_pci_list(int, char *argv[]);
int bl_command_usb_list(int, char *argv[]);

#endif

