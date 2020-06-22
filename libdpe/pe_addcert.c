// SPDX-License-Identifier: GPLv2
/*
 * pe_addcert.c - helpers to manage the PE cert directory
 * Copyright Peter Jones <pjones@redhat.com>
 * Copyright Red Hat, Inc.
 */
#include <unistd.h>

#include "libdpe_priv.h"

int
pe_clearcert(Pe *pe)
{
	int rc;
	data_directory *dd = NULL;

	if (!pe) {
		errno = EINVAL;
		return -1;
	}

	rc = pe_getdatadir(pe, &dd);
	if (rc < 0)
		return rc;

	if (dd->certs.virtual_address != 0) {
		pe_freespace(pe, dd->certs.virtual_address, dd->certs.size);
		memset(&dd->certs, '\0', sizeof (dd->certs));
	}

	return 0;
}

int
pe_alloccert(Pe *pe, size_t size)
{
	int rc;
	data_directory *dd = NULL;

	if (!pe) {
		errno = EINVAL;
		return -1;
	}

	pe_clearcert(pe);

	uint32_t new_space = 0;
	rc = pe_extend_file(pe, size, &new_space, 8);
	if (rc < 0)
		return rc;

	rc = pe_getdatadir(pe, &dd);
	if (rc < 0)
		return rc;

	void *addr = compute_mem_addr(pe, new_space);
	/* We leave the whole list empty until finalize...*/
	memset(addr, '\0', size);

	dd->certs.virtual_address = compute_file_addr(pe, addr);
	dd->certs.size += size;

	return 0;
}

int
pe_populatecert(Pe *pe, void *cert, size_t size)
{
	int rc;
	data_directory *dd = NULL;

	if (!pe) {
		errno = EINVAL;
		return -1;
	}

	rc = pe_getdatadir(pe, &dd);
	if (rc < 0)
		return rc;

	if (size != dd->certs.size)
		return -1;

	void *mem = compute_mem_addr(pe, dd->certs.virtual_address);
	if (!mem)
		return -1;

	memcpy(mem, cert, size);

	uint64_t max_size = pe->maximum_size;
	uint32_t new_space;
	uint32_t page_size = sysconf(_SC_PAGESIZE);

	pe_extend_file(pe, 0, &new_space, page_size);
	uint64_t new_max_size = pe->maximum_size;
	mem = compute_mem_addr(pe, 0);
	msync(mem, new_max_size, MS_SYNC);
	new_max_size -= max_size;
	pe_shorten_file(pe, new_max_size);

	return 0;
}
