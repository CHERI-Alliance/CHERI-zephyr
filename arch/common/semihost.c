/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 * Copyright (c) 2026 University of Birmingham, Modified to support CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/common/semihost.h>

struct semihost_poll_in_args {
	long zero;
} __packed;

struct semihost_open_args {
#ifdef __CHERI_PURE_CAPABILITY__
/*
 * The semihosting interface expects pointers to be represented as
 * memory addresses rather than capabilities. Store only the address
 * portion of the capability. This also avoids the alignment
 * requirements of a capability field in this packed structure.
 */
	unsigned long path;
#else
	const char *path;
#endif /* __CHERI_PURE_CAPABILITY__ */
	long mode;
	long path_len;
} __packed;

struct semihost_close_args {
	long fd;
} __packed;

struct semihost_flen_args {
	long fd;
} __packed;

struct semihost_seek_args {
	long fd;
	long offset;
} __packed;

struct semihost_read_args {
	long fd;
#ifdef __CHERI_PURE_CAPABILITY__
/*
 * The semihosting interface expects pointers to be represented as
 * memory addresses rather than capabilities. Store only the address
 * portion of the capability. This also avoids the alignment
 * requirements of a capability field in this packed structure.
 */
	unsigned long buf;
#else
	char *buf;
#endif /* __CHERI_PURE_CAPABILITY__ */
	long len;
} __packed;

struct semihost_write_args {
	long fd;
#ifdef __CHERI_PURE_CAPABILITY__
/*
 * The semihosting interface expects pointers to be represented as
 * memory addresses rather than capabilities. Store only the address
 * portion of the capability. This also avoids the alignment
 * requirements of a capability field in this packed structure.
 */
	unsigned long buf;
#else
	const char *buf;
#endif /* __CHERI_PURE_CAPABILITY__ */
	long len;
} __packed;

char semihost_poll_in(void)
{
	struct semihost_poll_in_args args = {
		.zero = 0
	};

	return (char)semihost_exec(SEMIHOST_READC, &args);
}

void semihost_poll_out(char c)
{
	/* WRITEC takes a pointer directly to the character */
	(void)semihost_exec(SEMIHOST_WRITEC, &c);
}

long semihost_open(const char *path, long mode)
{
	struct semihost_open_args args = {
#ifdef __CHERI_PURE_CAPABILITY__
/*
 * The semihosting interface expects pointers to be represented as
 * memory addresses rather than capabilities.
 */
		.path = (unsigned long)path,
#else
		.path = path,
#endif /* __CHERI_PURE_CAPABILITY__ */
		.mode = mode,
		.path_len = strlen(path)
	};

	return semihost_exec(SEMIHOST_OPEN, &args);
}

long semihost_close(long fd)
{
	struct semihost_close_args args = {
		.fd = fd
	};

	return semihost_exec(SEMIHOST_CLOSE, &args);
}

long semihost_flen(long fd)
{
	struct semihost_flen_args args = {
		.fd = fd
	};

	return semihost_exec(SEMIHOST_FLEN, &args);
}

long semihost_seek(long fd, long offset)
{
	struct semihost_seek_args args = {
		.fd = fd,
		.offset = offset
	};

	return semihost_exec(SEMIHOST_SEEK, &args);
}

long semihost_read(long fd, void *buf, long len)
{
	struct semihost_read_args args = {
		.fd = fd,
#ifdef __CHERI_PURE_CAPABILITY__
/*
 * The semihosting interface expects pointers to be represented as
 * memory addresses rather than capabilities.
 */
		.buf = (unsigned long)buf,
#else
		.buf = buf,
#endif /* __CHERI_PURE_CAPABILITY__ */
		.len = len
	};
	long ret;

	ret = semihost_exec(SEMIHOST_READ, &args);
	/* EOF condition */
	if (ret == len) {
		ret = -EIO;
	}
	/* All bytes read */
	else if (ret == 0) {
		ret = len;
	}
	return ret;
}

long semihost_write(long fd, const void *buf, long len)
{
	struct semihost_write_args args = {
		.fd = fd,
#ifdef __CHERI_PURE_CAPABILITY__
/*
 * The semihosting interface expects pointers to be represented as
 * memory addresses rather than capabilities.
 */
		.buf = (unsigned long)buf,
#else
		.buf = buf,
#endif /* __CHERI_PURE_CAPABILITY__ */
		.len = len
	};

	return semihost_exec(SEMIHOST_WRITE, &args);
}
