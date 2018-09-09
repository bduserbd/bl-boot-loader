#if 0
NOTE: Old version file !!!

#include "include/acpi.h"
#include "include/string.h"
#include "include/screen.h"

static int bl_acpi_checksum_correct(u8 *ptr, bl_size_t len)
{
	bl_size_t i;
	bl_uint8_t sum;

	sum = 0;
	for (i = 0; i < len; i++)
		sum += ptr[i];

	return !!sum;
}

static int bl_acpi_check_rsdp(struct bl_acpi_rsdp_version10 *rsdp1)
{
	struct bl_acpi_rsdp_version20 *rsdp2;
	bl_size_t second_table_len = 0;

	if (rsdp1->revision == 0x02) {
		rsdp2 = (struct bl_acpi_rsdp_version20 *)rsdp1;
		second_table_len = rsdp2->length;
	} else if (rsdp1->revision != 0x00)
		return 1;

	if (bl_acpi_checksum_correct((u8 *)rsdp1, sizeof(struct bl_acpi_rsdp_version10)))
		return 1;

	if (bl_acpi_checksum_correct((u8 *)rsdp1, second_table_len))
		return 1;

	return 0;
}

static void *bl_acpi_find_rsdp_bios(void)
{
	u8 *ptr;

	for (ptr = (u8 *)0xe0000; ptr < (u8 *)0xfffff; ptr += 16) {
		if (bl_memcmp(ptr, BL_ACPI_RSDP_SIGNATURE, 8))
			continue;

		if (bl_acpi_check_rsdp((struct bl_acpi_rsdp_version10 *)ptr))
			return NULL;
		else
			return (void *)ptr;
	}

	return NULL;
}

static void *bl_acpi_find_rsdp_ebda(void)
{
	int i;
	u8 *ptr;

	/* EBDA base address (Word). */
	ptr = (u8 *)((*(u16 *)0x40e << 4) & 0xfffff);

	for (i = 0; i < 1024; i += 16) {
		if (bl_memcmp(&ptr[i], BL_ACPI_RSDP_SIGNATURE, 8))
			continue;

		if (bl_acpi_check_rsdp((struct bl_acpi_rsdp_version10 *)ptr))
			return NULL;
		else
			return (void *)ptr;
	}

	return NULL;
}

static void *bl_acpi_find_rsdp(void)
{
	void *ret;

	(void)((ret = bl_acpi_find_rsdp_ebda()) || (ret = bl_acpi_find_rsdp_bios()));

	return ret;
}

static void bl_acpi_parse_fadt(struct bl_acpi_fadt *fadt)
{
	bl_size_t i;
	struct bl_acpi_dsdt *dsdt;

	if (bl_acpi_checksum_correct((u8 *)fadt, fadt->sdt.length))
		return;

	if (fadt->dsdt_address)
		dsdt = (struct bl_acpi_dsdt *)fadt->dsdt_address;
	else
		dsdt = (struct bl_acpi_dsdt *)fadt->x_dsdt_address;

	for (i = 0; i < dsdt->sdt.length - sizeof(dsdt->sdt) - 1; i++) {
	}
}

static void bl_acpi_parse_sdt(struct bl_acpi_sdt *sdt)
{
	if (!bl_strncmp(sdt->signature, BL_ACPI_FADT_SIGNATURE, 4)) {
		bl_acpi_parse_fadt((struct bl_acpi_fadt *)sdt);
	}
}

static void bl_acpi_parse_xsdt(struct bl_acpi_xsdt *xsdt)
{
	bl_size_t i;

	if (bl_strncmp(xsdt->sdt.signature, BL_ACPI_RSDT_SIGNATURE, 4) ||
		bl_acpi_checksum_correct((u8 *)xsdt, xsdt->sdt.length))
		return;

	for (i = 0; i < (xsdt->sdt.length - sizeof(*xsdt)) / sizeof(bl_uint64_t); i++)
		bl_acpi_parse_sdt(xsdt->entries[i]);
}

static void bl_acpi_parse_rsdt(struct bl_acpi_rsdt *rsdt)
{
	bl_size_t i;

	if (bl_strncmp(rsdt->sdt.signature, BL_ACPI_RSDT_SIGNATURE, 4) ||
		bl_acpi_checksum_correct((u8 *)rsdt, rsdt->sdt.length))
		return;

	for (i = 0; i < (rsdt->sdt.length - sizeof(*rsdt)) / sizeof(bl_uint32_t); i++)
		bl_acpi_parse_sdt(rsdt->entries[i]);
}

void bl_acpi_setup(void)
{
	struct bl_acpi_rsdp_version10 *rsdp1;
	struct bl_acpi_rsdp_version20 *rsdp2;

	rsdp1 = bl_acpi_find_rsdp();
	if (!rsdp1)
		return;

	if (rsdp1->revision == 0x00)
		bl_acpi_parse_rsdt((struct bl_acpi_rsdt *)rsdp1->rsdt_address);
	else {
		rsdp2 = (struct bl_acpi_rsdp_version20 *)rsdp1; 
		if (rsdp2->xsdt_address)
			bl_acpi_parse_xsdt((struct bl_acpi_xsdt *)rsdp2->xsdt_address);
		else
			bl_acpi_parse_rsdt((struct bl_acpi_rsdt *)rsdp1->rsdt_address);
	}
}

#endif

