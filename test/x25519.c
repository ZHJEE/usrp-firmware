/* Copyright (c) 2015, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include "console.h"
#include "common.h"
#include "curve25519.h"
#include "test_util.h"
#include "timer.h"
#include "util.h"
#include "watchdog.h"


static int test_x25519(void)
{
	/* Taken from https://tools.ietf.org/html/rfc7748#section-5.2 */
	static const uint8_t scalar1[32] = {
		0xa5, 0x46, 0xe3, 0x6b, 0xf0, 0x52, 0x7c, 0x9d,
		0x3b, 0x16, 0x15, 0x4b, 0x82, 0x46, 0x5e, 0xdd,
		0x62, 0x14, 0x4c, 0x0a, 0xc1, 0xfc, 0x5a, 0x18,
		0x50, 0x6a, 0x22, 0x44, 0xba, 0x44, 0x9a, 0xc4,
	};
	static const uint8_t point1[32] = {
		0xe6, 0xdb, 0x68, 0x67, 0x58, 0x30, 0x30, 0xdb,
		0x35, 0x94, 0xc1, 0xa4, 0x24, 0xb1, 0x5f, 0x7c,
		0x72, 0x66, 0x24, 0xec, 0x26, 0xb3, 0x35, 0x3b,
		0x10, 0xa9, 0x03, 0xa6, 0xd0, 0xab, 0x1c, 0x4c,
	};
	static const uint8_t expected1[32] = {
		0xc3, 0xda, 0x55, 0x37, 0x9d, 0xe9, 0xc6, 0x90,
		0x8e, 0x94, 0xea, 0x4d, 0xf2, 0x8d, 0x08, 0x4f,
		0x32, 0xec, 0xcf, 0x03, 0x49, 0x1c, 0x71, 0xf7,
		0x54, 0xb4, 0x07, 0x55, 0x77, 0xa2, 0x85, 0x52,
	};
	static const uint8_t scalar2[32] = {
		0x4b, 0x66, 0xe9, 0xd4, 0xd1, 0xb4, 0x67, 0x3c,
		0x5a, 0xd2, 0x26, 0x91, 0x95, 0x7d, 0x6a, 0xf5,
		0xc1, 0x1b, 0x64, 0x21, 0xe0, 0xea, 0x01, 0xd4,
		0x2c, 0xa4, 0x16, 0x9e, 0x79, 0x18, 0xba, 0x0d,
	};
	static const uint8_t point2[32] = {
		0xe5, 0x21, 0x0f, 0x12, 0x78, 0x68, 0x11, 0xd3,
		0xf4, 0xb7, 0x95, 0x9d, 0x05, 0x38, 0xae, 0x2c,
		0x31, 0xdb, 0xe7, 0x10, 0x6f, 0xc0, 0x3c, 0x3e,
		0xfc, 0x4c, 0xd5, 0x49, 0xc7, 0x15, 0xa4, 0x93,
	};
	static const uint8_t expected2[32] = {
		0x95, 0xcb, 0xde, 0x94, 0x76, 0xe8, 0x90, 0x7d,
		0x7a, 0xad, 0xe4, 0x5c, 0xb4, 0xb8, 0x73, 0xf8,
		0x8b, 0x59, 0x5a, 0x68, 0x79, 0x9f, 0xa1, 0x52,
		0xe6, 0xf8, 0xf7, 0x64, 0x7a, 0xac, 0x79, 0x57,
	};
	uint8_t out[32];

	X25519(out, scalar1, point1);

	if (memcmp(expected1, out, sizeof(out)) != 0) {
		ccprintf("X25519 test one failed.\n");
		return 0;
	}


	X25519(out, scalar2, point2);

	if (memcmp(expected2, out, sizeof(out)) != 0) {
		ccprintf("X25519 test two failed.\n");
		return 0;
	}

	return 1;
}

static int test_x25519_small_order(void)
{
	static const uint8_t kSmallOrderPoint[32] = {
		0xe0, 0xeb, 0x7a, 0x7c, 0x3b, 0x41, 0xb8, 0xae,
		0x16, 0x56, 0xe3, 0xfa, 0xf1, 0x9f, 0xc4, 0x6a,
		0xda, 0x09, 0x8d, 0xeb, 0x9c, 0x32, 0xb1, 0xfd,
		0x86, 0x62, 0x05, 0x16, 0x5f, 0x49, 0xb8,
	};
	uint8_t out[32], private_key[32];

	memset(private_key, 0x11, sizeof(private_key));

	if (X25519(out, private_key, kSmallOrderPoint)) {
		ccprintf("X25519 returned success with a small-order input.\n");
		return 0;
	}

	return 1;
}

static int test_x25519_iterated(void)
{
	/* Taken from https://tools.ietf.org/html/rfc7748#section-5.2 */
	static const uint8_t expected[32] = {
		0x68, 0x4c, 0xf5, 0x9b, 0xa8, 0x33, 0x09, 0x55,
		0x28, 0x00, 0xef, 0x56, 0x6f, 0x2f, 0x4d, 0x3c,
		0x1c, 0x38, 0x87, 0xc4, 0x93, 0x60, 0xe3, 0x87,
		0x5f, 0x2e, 0xb9, 0x4d, 0x99, 0x53, 0x2c, 0x51,
	};
	uint8_t scalar[32] = {9}, point[32] = {9}, out[32];
	unsigned i;

	for (i = 0; i < 1000; i++) {
		watchdog_reload();
		X25519(out, scalar, point);
		memcpy(point, scalar, sizeof(point));
		memcpy(scalar, out, sizeof(scalar));
	}

	if (memcmp(expected, scalar, sizeof(expected)) != 0) {
		ccprintf("Iterated X25519 test failed\n");
		return 0;
	}

	return 1;
}

static void test_x25519_speed(void)
{
	static const uint8_t scalar1[32] = {
		0xa5, 0x46, 0xe3, 0x6b, 0xf0, 0x52, 0x7c, 0x9d,
		0x3b, 0x16, 0x15, 0x4b, 0x82, 0x46, 0x5e, 0xdd,
		0x62, 0x14, 0x4c, 0x0a, 0xc1, 0xfc, 0x5a, 0x18,
		0x50, 0x6a, 0x22, 0x44, 0xba, 0x44, 0x9a, 0xc4,
	};
	static const uint8_t point1[32] = {
		0xe6, 0xdb, 0x68, 0x67, 0x58, 0x30, 0x30, 0xdb,
		0x35, 0x94, 0xc1, 0xa4, 0x24, 0xb1, 0x5f, 0x7c,
		0x72, 0x66, 0x24, 0xec, 0x26, 0xb3, 0x35, 0x3b,
		0x10, 0xa9, 0x03, 0xa6, 0xd0, 0xab, 0x1c, 0x4c,
	};
	uint8_t out[32];
	timestamp_t t0, t1;

	X25519(out, scalar1, point1);
	t0 = get_time();
	X25519(out, scalar1, point1);
	t1 = get_time();
	ccprintf("X25519 duration %ld us\n", t1.val - t0.val);
}

void run_test(void)
{
	watchdog_reload();
	/* do not check speed, just as a benchmark */
	test_x25519_speed();

	watchdog_reload();
	if (!test_x25519() || !test_x25519_iterated() ||
	    !test_x25519_small_order()) {
		test_fail();
		return;
	}

	test_pass();
}
