/*
 * lws-minimal-http-server-tls
 *
 * Written in 2010-2019 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * This demonstrates the most minimal http server you can make with lws,
 * with three extra lines giving it tls (ssl) capabilities, which in
 * turn allow operation with HTTP/2 if lws was configured for it.
 *
 * To keep it simple, it serves stuff from the subdirectory
 * "./mount-origin" of the directory it was started in.
 *
 * You can change that by changing mount.origin below.
 */

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>

static int interrupted;

static const struct lws_http_mount mount = {
	.mountpoint		= "/",			/* mountpoint URL */
	.origin			= "./mount-origin",	/* serve from dir */
	.def			= "index.html",		/* default filename */
	.origin_protocol	= LWSMPRO_FILE,		/* files in a dir */
	.mountpoint_len		= 1,			/* char count */
};

/* the cert and key as PEM */

static const char *cert_pem =
	"-----BEGIN CERTIFICATE-----\n"
	"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\n"
	"e2Hl/7M0c/lbZ4FvXgARlex2rkgS0Ka06HE=\n"
	"-----END CERTIFICATE-----\n",

		*key_pem =
	"-----BEGIN PRIVATE KEY-----\n"
	"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\n"
	"fBrpEY1IATtPq1taBZZogRqI3rOkkPk=\n"
	"-----END PRIVATE KEY-----\n"
	;

static const uint8_t cert_der[] = {
	0x30, 0x82, 0x05, 0xe6, 0x30, 0x82, 0x03, 0xce, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x09, 0x00,
	0xda, 0xb9, 0xd0, 0x8b, 0xb0, 0x3c, 0x52, 0xa0, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
	0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x30, 0x81, 0x86, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03,
	0x55, 0x04, 0x06, 0x13, 0x02, 0x47, 0x42, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x08,
	0x0c, 0x07, 0x45, 0x72, 0x65, 0x77, 0x68, 0x6f, 0x6e, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55,
	0x04, 0x07, 0x0c, 0x0a, 0x41, 0x6c, 0x6c, 0x20, 0x61, 0x72, 0x6f, 0x75, 0x6e, 0x64, 0x31, 0x1b,
	0x30, 0x19, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x12, 0x6c, 0x69, 0x62, 0x77, 0x65, 0x62, 0x73,
	0x6f, 0x63, 0x6b, 0x65, 0x74, 0x73, 0x2d, 0x74, 0x65, 0x73, 0x74, 0x31, 0x12, 0x30, 0x10, 0x06,
	0x03, 0x55, 0x04, 0x03, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x31,
	0x1f, 0x30, 0x1d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x10,
	0x6e, 0x6f, 0x6e, 0x65, 0x40, 0x69, 0x6e, 0x76, 0x61, 0x6c, 0x69, 0x64, 0x2e, 0x6f, 0x72, 0x67,
	0x30, 0x20, 0x17, 0x0d, 0x31, 0x38, 0x30, 0x33, 0x32, 0x30, 0x30, 0x34, 0x31, 0x36, 0x30, 0x37,
	0x5a, 0x18, 0x0f, 0x32, 0x31, 0x31, 0x38, 0x30, 0x32, 0x32, 0x34, 0x30, 0x34, 0x31, 0x36, 0x30,
	0x37, 0x5a, 0x30, 0x81, 0x86, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
	0x47, 0x42, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x07, 0x45, 0x72, 0x65,
	0x77, 0x68, 0x6f, 0x6e, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0c, 0x0a, 0x41,
	0x6c, 0x6c, 0x20, 0x61, 0x72, 0x6f, 0x75, 0x6e, 0x64, 0x31, 0x1b, 0x30, 0x19, 0x06, 0x03, 0x55,
	0x04, 0x0a, 0x0c, 0x12, 0x6c, 0x69, 0x62, 0x77, 0x65, 0x62, 0x73, 0x6f, 0x63, 0x6b, 0x65, 0x74,
	0x73, 0x2d, 0x74, 0x65, 0x73, 0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c,
	0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x31, 0x1f, 0x30, 0x1d, 0x06, 0x09,
	0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x10, 0x6e, 0x6f, 0x6e, 0x65, 0x40,
	0x69, 0x6e, 0x76, 0x61, 0x6c, 0x69, 0x64, 0x2e, 0x6f, 0x72, 0x67, 0x30, 0x82, 0x02, 0x22, 0x30,
	0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82,
	0x02, 0x0f, 0x00, 0x30, 0x82, 0x02, 0x0a, 0x02, 0x82, 0x02, 0x01, 0x00, 0xa3, 0x62, 0xdb, 0x96,
	0x68, 0x80, 0x82, 0x63, 0x4b, 0x49, 0x3e, 0xe6, 0xf1, 0xa4, 0x88, 0x08, 0x2f, 0xe5, 0x96, 0x9b,
	0x3f, 0xdf, 0x98, 0xaf, 0x08, 0x42, 0xbd, 0x75, 0x5a, 0xd7, 0x9e, 0xeb, 0xf2, 0x14, 0xc9, 0x49,
	0x68, 0xe4, 0x8e, 0xb4, 0xda, 0x6a, 0xb5, 0xa9, 0xc2, 0xe1, 0x4f, 0xf9, 0x26, 0xa6, 0x84, 0x7c,
	0x0e, 0x2d, 0xc3, 0x02, 0x61, 0xca, 0x9d, 0x25, 0x9d, 0x3d, 0x6b, 0x67, 0xd4, 0x1b, 0x57, 0x2c,
	0x4a, 0xcb, 0x95, 0x48, 0x87, 0x81, 0x90, 0xeb, 0x65, 0x62, 0x27, 0x98, 0x40, 0x63, 0x28, 0xcd,
	0x43, 0x65, 0xff, 0x82, 0xbc, 0xd1, 0x99, 0xf8, 0x4c, 0xcf, 0x80, 0x1b, 0xf9, 0x9d, 0x37, 0xa4,
	0x2d, 0x67, 0x1f, 0x23, 0x96, 0x59, 0xb6, 0x81, 0xae, 0x20, 0xfd, 0x43, 0x97, 0xf2, 0x24, 0x34,
	0x3c, 0x3c, 0xcc, 0x5c, 0xf8, 0x72, 0x98, 0x8c, 0x7b, 0xf0, 0x45, 0x19, 0xe9, 0xb2, 0xc5, 0xd1,
	0xe1, 0x2e, 0xb2, 0x87, 0x4a, 0x6f, 0x04, 0xa3, 0xe9, 0xd3, 0xef, 0x7e, 0x2d, 0x22, 0xd9, 0xc7,
	0x29, 0x3f, 0xe6, 0xe8, 0x34, 0x94, 0xd3, 0x19, 0x59, 0xd7, 0x77, 0x7a, 0x7a, 0x12, 0xd1, 0x9b,
	0xbf, 0xfe, 0x37, 0x1e, 0x3b, 0x33, 0x75, 0xcc, 0x4d, 0x11, 0xf9, 0xa8, 0xa3, 0xff, 0xed, 0x34,
	0xc4, 0xda, 0xcd, 0x14, 0xeb, 0xe3, 0x34, 0xb6, 0xc1, 0x88, 0xdb, 0x3a, 0x51, 0x8b, 0xe9, 0xba,
	0x8f, 0x38, 0x4d, 0xc8, 0xc0, 0x53, 0x27, 0x5b, 0xb9, 0xf2, 0xa0, 0x1e, 0xdd, 0x95, 0xb9, 0xff,
	0xe6, 0x00, 0x8a, 0xe6, 0x58, 0x00, 0x1e, 0xa7, 0xe5, 0xb8, 0x54, 0xa7, 0x8a, 0x05, 0xb8, 0x1e,
	0x70, 0x61, 0xb7, 0x01, 0xcb, 0x05, 0x51, 0xf2, 0xe8, 0xc8, 0x9e, 0x91, 0x7c, 0x6e, 0xe5, 0x90,
	0x52, 0x3c, 0xb9, 0x37, 0xca, 0x52, 0x36, 0x9e, 0xec, 0xcd, 0xd6, 0x2c, 0x9c, 0xb2, 0x69, 0xbc,
	0x07, 0x74, 0xb2, 0x26, 0xeb, 0x34, 0xf8, 0xc2, 0xd0, 0x54, 0x02, 0x36, 0xba, 0x4d, 0x8e, 0x02,
	0x66, 0x20, 0xad, 0xfe, 0x98, 0xa9, 0x38, 0x91, 0x75, 0xfb, 0x65, 0x3c, 0x1e, 0x7e, 0x80, 0x33,
	0x4c, 0xae, 0x25, 0xda, 0x91, 0xcd, 0xb8, 0x2e, 0x77, 0x41, 0x57, 0x3f, 0x10, 0x5f, 0xbe, 0x18,
	0x12, 0xc0, 0xc6, 0x6b, 0xc2, 0x0e, 0xaf, 0x59, 0xa4, 0xc2, 0x18, 0x8b, 0xb3, 0xa6, 0xce, 0x49,
	0x00, 0x28, 0xa0, 0xbd, 0x51, 0xee, 0x84, 0x7f, 0x6d, 0x7b, 0x2c, 0x54, 0x02, 0x14, 0x80, 0x4a,
	0x23, 0x3b, 0xfd, 0x72, 0x08, 0xbd, 0x7f, 0x03, 0xcc, 0x2e, 0x1a, 0xca, 0x95, 0xea, 0x15, 0x44,
	0xdb, 0x1e, 0x70, 0x1b, 0x02, 0x3f, 0x9e, 0xbd, 0x5a, 0x02, 0x57, 0x85, 0x49, 0xf0, 0x7f, 0x69,
	0x68, 0x9f, 0x87, 0xc4, 0x66, 0xbd, 0xfe, 0xbd, 0x1b, 0x9c, 0xf6, 0xc8, 0x5f, 0xaa, 0x75, 0x74,
	0x9c, 0xf3, 0x75, 0x20, 0xc4, 0xa7, 0xcd, 0x70, 0x9a, 0xb2, 0xde, 0xc8, 0xd9, 0xf8, 0xae, 0x45,
	0x77, 0x48, 0xcf, 0xde, 0x8a, 0x8e, 0x51, 0x90, 0xa4, 0xfe, 0x17, 0x7c, 0xd5, 0x40, 0xf9, 0x11,
	0x8b, 0xed, 0xa3, 0x27, 0x58, 0xe1, 0x48, 0x69, 0x5a, 0xca, 0x58, 0xbc, 0xc0, 0xb6, 0x0c, 0xe8,
	0x18, 0xc4, 0xef, 0x3f, 0xf0, 0x2e, 0x7a, 0x12, 0x97, 0x9d, 0xc0, 0x49, 0x85, 0x8b, 0x56, 0xd2,
	0x5b, 0x53, 0x8a, 0x85, 0x71, 0xfb, 0x9c, 0x93, 0x61, 0x20, 0x19, 0x5a, 0x5f, 0x88, 0xb2, 0xc9,
	0x97, 0x8d, 0xe7, 0xf1, 0x26, 0xa6, 0x22, 0xdb, 0xfe, 0xd0, 0x5a, 0x6b, 0xf5, 0x40, 0x2f, 0x69,
	0xb0, 0xd7, 0x23, 0x4c, 0xc6, 0x81, 0x40, 0xb3, 0x74, 0xdd, 0x3d, 0x50, 0x7a, 0x56, 0xec, 0xed,
	0x8d, 0xbb, 0xb3, 0x17, 0x44, 0x9c, 0xd5, 0x2d, 0x87, 0x89, 0x08, 0xfb, 0x02, 0x03, 0x01, 0x00,
	0x01, 0xa3, 0x53, 0x30, 0x51, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14,
	0xf6, 0x66, 0x14, 0xdb, 0x7b, 0x56, 0xdb, 0x3b, 0x28, 0x9a, 0x42, 0x93, 0x01, 0x76, 0xab, 0x8e,
	0xbd, 0xaf, 0x8e, 0xeb, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80,
	0x14, 0xf6, 0x66, 0x14, 0xdb, 0x7b, 0x56, 0xdb, 0x3b, 0x28, 0x9a, 0x42, 0x93, 0x01, 0x76, 0xab,
	0x8e, 0xbd, 0xaf, 0x8e, 0xeb, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04,
	0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
	0x01, 0x01, 0x0b, 0x05, 0x00, 0x03, 0x82, 0x02, 0x01, 0x00, 0x36, 0x32, 0x01, 0x32, 0xba, 0x30,
	0x60, 0xd0, 0x9b, 0x84, 0x02, 0x5d, 0x3f, 0xb7, 0x61, 0x96, 0x14, 0xf6, 0x45, 0x41, 0x51, 0x75,
	0xe4, 0x54, 0x24, 0x3d, 0x08, 0xc6, 0xb1, 0xff, 0x86, 0x4b, 0xdb, 0xea, 0x6c, 0x87, 0x1e, 0x72,
	0xbc, 0x9c, 0xe6, 0x1e, 0xcc, 0x53, 0xe3, 0x52, 0x59, 0x91, 0x29, 0x48, 0x0d, 0x10, 0x3b, 0x80,
	0xc5, 0xb9, 0xd7, 0x67, 0x33, 0xdd, 0x09, 0x13, 0x55, 0xf5, 0x5d, 0xa6, 0x4a, 0x16, 0xd7, 0xbc,
	0x2c, 0xa2, 0x0d, 0x8e, 0xd6, 0x09, 0x01, 0x36, 0x06, 0x7e, 0x38, 0xcf, 0x6e, 0x8e, 0xd2, 0xe5,
	0x95, 0x93, 0xee, 0xc3, 0x34, 0xd2, 0xc7, 0xf4, 0x19, 0xe4, 0xc1, 0x4b, 0x4e, 0x9c, 0xcf, 0x4f,
	0xc2, 0xd9, 0x83, 0xf6, 0x98, 0x56, 0x7b, 0x19, 0xb8, 0xab, 0x61, 0xa7, 0x4e, 0xc8, 0x8b, 0xe9,
	0x49, 0x7a, 0x73, 0x2d, 0x10, 0x95, 0x32, 0x56, 0x29, 0x52, 0xc4, 0x51, 0x04, 0x3a, 0xc9, 0xd6,
	0xb9, 0xf3, 0x67, 0xb6, 0xdc, 0x9d, 0x40, 0x5e, 0xab, 0x6a, 0x15, 0xca, 0x5f, 0xa0, 0x4d, 0xf8,
	0x1f, 0x76, 0x9f, 0x12, 0x21, 0xb2, 0xf3, 0xcd, 0x9b, 0xf9, 0x90, 0x62, 0xc2, 0x47, 0x95, 0xfa,
	0x8a, 0xba, 0x5d, 0x51, 0x7c, 0xb0, 0x5c, 0xab, 0xf7, 0x36, 0x2b, 0xbf, 0xd0, 0xaf, 0x59, 0x36,
	0x25, 0x92, 0x94, 0xd0, 0x7c, 0xb4, 0xd9, 0x4a, 0xc8, 0x0f, 0x74, 0x41, 0xd8, 0x55, 0xc8, 0xef,
	0xc5, 0x0d, 0x83, 0xf9, 0x7c, 0x83, 0x47, 0x46, 0x91, 0x2d, 0x19, 0x6f, 0xc5, 0x46, 0xbd, 0x74,
	0x71, 0x85, 0x1c, 0xb2, 0x02, 0x1b, 0x7e, 0x09, 0xba, 0xae, 0x40, 0x8b, 0xa9, 0x4c, 0xd4, 0x4b,
	0x28, 0x0f, 0xc1, 0xd2, 0xb0, 0x9a, 0x4c, 0x72, 0x6a, 0xc7, 0xec, 0xc5, 0xb0, 0xd9, 0xc2, 0xa4,
	0xba, 0x30, 0xb7, 0xac, 0xc7, 0x45, 0x4e, 0xdb, 0x5e, 0xf3, 0x7c, 0x05, 0xd6, 0xeb, 0x85, 0xe0,
	0x58, 0xd4, 0x0b, 0xbd, 0xbe, 0x4a, 0x67, 0x10, 0x37, 0xb0, 0x37, 0xf3, 0xa0, 0x42, 0xfe, 0x79,
	0x36, 0x4d, 0x3b, 0x09, 0x6b, 0x04, 0xc3, 0xce, 0xac, 0x0e, 0xbb, 0xf5, 0x5d, 0x66, 0xfd, 0xa0,
	0xd5, 0x6a, 0x53, 0x1e, 0x5b, 0xa6, 0x94, 0x29, 0x59, 0x78, 0xff, 0x86, 0xfe, 0x39, 0x12, 0xc8,
	0x3c, 0x2a, 0x36, 0x74, 0xee, 0xd5, 0xaa, 0x1d, 0x0e, 0x65, 0x1a, 0xe3, 0x16, 0x68, 0x75, 0xf8,
	0x4f, 0xd4, 0x75, 0x8f, 0xc1, 0x42, 0x85, 0x72, 0xaf, 0x28, 0x42, 0xbd, 0x78, 0xf1, 0x06, 0x00,
	0x00, 0xe9, 0x5b, 0x50, 0xe2, 0x50, 0x53, 0xb4, 0x30, 0x45, 0x67, 0x75, 0x55, 0xb9, 0xf0, 0x84,
	0x3b, 0x50, 0x59, 0x70, 0xbd, 0xd8, 0x0d, 0xb0, 0xd6, 0x7f, 0xf1, 0x91, 0x94, 0x91, 0xd4, 0x13,
	0x3f, 0x35, 0x44, 0x83, 0x86, 0x40, 0x52, 0x51, 0x4d, 0x56, 0x8c, 0xc6, 0xd6, 0x83, 0xa1, 0xa0,
	0x9a, 0x72, 0x19, 0x2d, 0x17, 0xab, 0x40, 0x2b, 0xb5, 0x3a, 0x8c, 0xeb, 0xf3, 0xba, 0xce, 0x42,
	0xa4, 0x1a, 0x90, 0xf9, 0x32, 0xb7, 0xc0, 0x54, 0x48, 0xd2, 0xb7, 0x2b, 0x8d, 0xa3, 0xda, 0xa7,
	0x1f, 0x84, 0x03, 0x8d, 0x75, 0x19, 0x7c, 0x1e, 0xaf, 0x10, 0xb3, 0x9a, 0x6e, 0xa7, 0x2f, 0xac,
	0xf2, 0xc7, 0x42, 0x18, 0x39, 0x70, 0x47, 0x72, 0x4d, 0x08, 0xcb, 0xfa, 0xbb, 0x8f, 0x0e, 0x2b,
	0xce, 0xc5, 0xe2, 0x67, 0x08, 0xc6, 0x19, 0x12, 0x79, 0xf1, 0x49, 0x50, 0x52, 0x08, 0xdb, 0x9a,
	0x42, 0x18, 0xde, 0x56, 0xb4, 0x4e, 0x29, 0xe6, 0x5f, 0xbd, 0x72, 0x73, 0xb5, 0x1a, 0xb2, 0x17,
	0x7b, 0x61, 0xe5, 0xff, 0xb3, 0x34, 0x73, 0xf9, 0x5b, 0x67, 0x81, 0x6f, 0x5e, 0x00, 0x11, 0x95,
	0xec, 0x76, 0xae, 0x48, 0x12, 0xd0, 0xa6, 0xb4, 0xe8, 0x71,
}, key_der[] = {
	0x30, 0x82, 0x09, 0x43, 0x02, 0x01, 0x00, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
	0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82, 0x09, 0x2d, 0x30, 0x82, 0x09, 0x29, 0x02, 0x01,
	0x00, 0x02, 0x82, 0x02, 0x01, 0x00, 0xa3, 0x62, 0xdb, 0x96, 0x68, 0x80, 0x82, 0x63, 0x4b, 0x49,
	0x3e, 0xe6, 0xf1, 0xa4, 0x88, 0x08, 0x2f, 0xe5, 0x96, 0x9b, 0x3f, 0xdf, 0x98, 0xaf, 0x08, 0x42,
	0xbd, 0x75, 0x5a, 0xd7, 0x9e, 0xeb, 0xf2, 0x14, 0xc9, 0x49, 0x68, 0xe4, 0x8e, 0xb4, 0xda, 0x6a,
	0xb5, 0xa9, 0xc2, 0xe1, 0x4f, 0xf9, 0x26, 0xa6, 0x84, 0x7c, 0x0e, 0x2d, 0xc3, 0x02, 0x61, 0xca,
	0x9d, 0x25, 0x9d, 0x3d, 0x6b, 0x67, 0xd4, 0x1b, 0x57, 0x2c, 0x4a, 0xcb, 0x95, 0x48, 0x87, 0x81,
	0x90, 0xeb, 0x65, 0x62, 0x27, 0x98, 0x40, 0x63, 0x28, 0xcd, 0x43, 0x65, 0xff, 0x82, 0xbc, 0xd1,
	0x99, 0xf8, 0x4c, 0xcf, 0x80, 0x1b, 0xf9, 0x9d, 0x37, 0xa4, 0x2d, 0x67, 0x1f, 0x23, 0x96, 0x59,
	0xb6, 0x81, 0xae, 0x20, 0xfd, 0x43, 0x97, 0xf2, 0x24, 0x34, 0x3c, 0x3c, 0xcc, 0x5c, 0xf8, 0x72,
	0x98, 0x8c, 0x7b, 0xf0, 0x45, 0x19, 0xe9, 0xb2, 0xc5, 0xd1, 0xe1, 0x2e, 0xb2, 0x87, 0x4a, 0x6f,
	0x04, 0xa3, 0xe9, 0xd3, 0xef, 0x7e, 0x2d, 0x22, 0xd9, 0xc7, 0x29, 0x3f, 0xe6, 0xe8, 0x34, 0x94,
	0xd3, 0x19, 0x59, 0xd7, 0x77, 0x7a, 0x7a, 0x12, 0xd1, 0x9b, 0xbf, 0xfe, 0x37, 0x1e, 0x3b, 0x33,
	0x75, 0xcc, 0x4d, 0x11, 0xf9, 0xa8, 0xa3, 0xff, 0xed, 0x34, 0xc4, 0xda, 0xcd, 0x14, 0xeb, 0xe3,
	0x34, 0xb6, 0xc1, 0x88, 0xdb, 0x3a, 0x51, 0x8b, 0xe9, 0xba, 0x8f, 0x38, 0x4d, 0xc8, 0xc0, 0x53,
	0x27, 0x5b, 0xb9, 0xf2, 0xa0, 0x1e, 0xdd, 0x95, 0xb9, 0xff, 0xe6, 0x00, 0x8a, 0xe6, 0x58, 0x00,
	0x1e, 0xa7, 0xe5, 0xb8, 0x54, 0xa7, 0x8a, 0x05, 0xb8, 0x1e, 0x70, 0x61, 0xb7, 0x01, 0xcb, 0x05,
	0x51, 0xf2, 0xe8, 0xc8, 0x9e, 0x91, 0x7c, 0x6e, 0xe5, 0x90, 0x52, 0x3c, 0xb9, 0x37, 0xca, 0x52,
	0x36, 0x9e, 0xec, 0xcd, 0xd6, 0x2c, 0x9c, 0xb2, 0x69, 0xbc, 0x07, 0x74, 0xb2, 0x26, 0xeb, 0x34,
	0xf8, 0xc2, 0xd0, 0x54, 0x02, 0x36, 0xba, 0x4d, 0x8e, 0x02, 0x66, 0x20, 0xad, 0xfe, 0x98, 0xa9,
	0x38, 0x91, 0x75, 0xfb, 0x65, 0x3c, 0x1e, 0x7e, 0x80, 0x33, 0x4c, 0xae, 0x25, 0xda, 0x91, 0xcd,
	0xb8, 0x2e, 0x77, 0x41, 0x57, 0x3f, 0x10, 0x5f, 0xbe, 0x18, 0x12, 0xc0, 0xc6, 0x6b, 0xc2, 0x0e,
	0xaf, 0x59, 0xa4, 0xc2, 0x18, 0x8b, 0xb3, 0xa6, 0xce, 0x49, 0x00, 0x28, 0xa0, 0xbd, 0x51, 0xee,
	0x84, 0x7f, 0x6d, 0x7b, 0x2c, 0x54, 0x02, 0x14, 0x80, 0x4a, 0x23, 0x3b, 0xfd, 0x72, 0x08, 0xbd,
	0x7f, 0x03, 0xcc, 0x2e, 0x1a, 0xca, 0x95, 0xea, 0x15, 0x44, 0xdb, 0x1e, 0x70, 0x1b, 0x02, 0x3f,
	0x9e, 0xbd, 0x5a, 0x02, 0x57, 0x85, 0x49, 0xf0, 0x7f, 0x69, 0x68, 0x9f, 0x87, 0xc4, 0x66, 0xbd,
	0xfe, 0xbd, 0x1b, 0x9c, 0xf6, 0xc8, 0x5f, 0xaa, 0x75, 0x74, 0x9c, 0xf3, 0x75, 0x20, 0xc4, 0xa7,
	0xcd, 0x70, 0x9a, 0xb2, 0xde, 0xc8, 0xd9, 0xf8, 0xae, 0x45, 0x77, 0x48, 0xcf, 0xde, 0x8a, 0x8e,
	0x51, 0x90, 0xa4, 0xfe, 0x17, 0x7c, 0xd5, 0x40, 0xf9, 0x11, 0x8b, 0xed, 0xa3, 0x27, 0x58, 0xe1,
	0x48, 0x69, 0x5a, 0xca, 0x58, 0xbc, 0xc0, 0xb6, 0x0c, 0xe8, 0x18, 0xc4, 0xef, 0x3f, 0xf0, 0x2e,
	0x7a, 0x12, 0x97, 0x9d, 0xc0, 0x49, 0x85, 0x8b, 0x56, 0xd2, 0x5b, 0x53, 0x8a, 0x85, 0x71, 0xfb,
	0x9c, 0x93, 0x61, 0x20, 0x19, 0x5a, 0x5f, 0x88, 0xb2, 0xc9, 0x97, 0x8d, 0xe7, 0xf1, 0x26, 0xa6,
	0x22, 0xdb, 0xfe, 0xd0, 0x5a, 0x6b, 0xf5, 0x40, 0x2f, 0x69, 0xb0, 0xd7, 0x23, 0x4c, 0xc6, 0x81,
	0x40, 0xb3, 0x74, 0xdd, 0x3d, 0x50, 0x7a, 0x56, 0xec, 0xed, 0x8d, 0xbb, 0xb3, 0x17, 0x44, 0x9c,
	0xd5, 0x2d, 0x87, 0x89, 0x08, 0xfb, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02, 0x82, 0x02, 0x00, 0x55,
	0x9e, 0xf0, 0xc4, 0x19, 0x6f, 0x7e, 0xe4, 0xda, 0x07, 0x40, 0x57, 0x76, 0x3a, 0x6a, 0xaf, 0x1f,
	0xaa, 0x89, 0x0a, 0x42, 0xa6, 0xc2, 0x34, 0xb7, 0x77, 0x82, 0x21, 0x85, 0xc1, 0x89, 0x1e, 0xcc,
	0x75, 0xe8, 0x25, 0xf8, 0x3a, 0x0e, 0x2e, 0xe8, 0x67, 0x13, 0x5c, 0x2b, 0x2c, 0x37, 0xe4, 0xb1,
	0x44, 0x82, 0x19, 0x20, 0xb5, 0x0a, 0x84, 0xad, 0x0a, 0xa8, 0xdf, 0x95, 0x4f, 0x22, 0x81, 0xfe,
	0xbd, 0x75, 0x29, 0x58, 0xe8, 0xe7, 0x0a, 0x63, 0x38, 0x9a, 0xe1, 0x40, 0xf7, 0xf7, 0x17, 0xea,
	0x66, 0x0c, 0x73, 0xc4, 0xe6, 0x26, 0xc8, 0x34, 0x7b, 0x02, 0xdd, 0x04, 0x23, 0x99, 0x57, 0x0f,
	0xb0, 0x3c, 0x00, 0x65, 0x6a, 0xac, 0xfe, 0xd1, 0x43, 0xa2, 0x48, 0xc3, 0x1f, 0xb6, 0x99, 0x3d,
	0x7f, 0x3f, 0x49, 0xc0, 0x67, 0x7c, 0x11, 0x1c, 0x81, 0xb1, 0x3f, 0xad, 0x93, 0x74, 0x22, 0xe8,
	0x3d, 0x2f, 0x3d, 0x95, 0x6c, 0x0b, 0x52, 0xaa, 0xc7, 0x12, 0xff, 0x73, 0x02, 0x05, 0x77, 0x71,
	0xdf, 0xd9, 0x90, 0x6d, 0x25, 0x77, 0xb4, 0x28, 0x19, 0xf5, 0xa6, 0x4b, 0x56, 0x86, 0xde, 0x40,
	0x2a, 0xac, 0x7d, 0x9a, 0x57, 0x76, 0x3a, 0xf9, 0x7b, 0x36, 0x38, 0x22, 0x0b, 0x51, 0x71, 0xf6,
	0xbf, 0x9f, 0x67, 0x0f, 0xe2, 0x39, 0xa6, 0xc5, 0x17, 0x04, 0x00, 0xe1, 0xda, 0xfe, 0x47, 0xc9,
	0x84, 0x30, 0xaf, 0xfb, 0x6d, 0xde, 0x15, 0x5d, 0xf4, 0x35, 0xa3, 0xf4, 0x06, 0x19, 0xb3, 0x13,
	0x1b, 0xeb, 0xa5, 0x16, 0xbb, 0x22, 0x0f, 0x23, 0xfe, 0xac, 0x12, 0x00, 0x68, 0x60, 0xb4, 0x8b,
	0xb8, 0x03, 0x8c, 0xb0, 0x08, 0x05, 0x07, 0x83, 0x84, 0xfe, 0x34, 0xf5, 0x98, 0x6c, 0xc0, 0x81,
	0x1c, 0xfc, 0x60, 0x6d, 0x38, 0x35, 0x37, 0xef, 0x66, 0xb6, 0x09, 0x02, 0xbf, 0xbb, 0x84, 0x3f,
	0x1c, 0x14, 0x2f, 0xb8, 0x1b, 0x4a, 0x14, 0xd9, 0x06, 0x52, 0x8a, 0x0b, 0x80, 0x20, 0x9b, 0x17,
	0x1c, 0xe0, 0x35, 0x41, 0x9c, 0xf3, 0x71, 0x81, 0xff, 0xa2, 0x30, 0x6c, 0x43, 0x3b, 0x47, 0x9b,
	0x97, 0xaa, 0xc1, 0x62, 0x13, 0xbd, 0x4b, 0xa6, 0x6a, 0xe8, 0x0f, 0x28, 0xca, 0x4e, 0x54, 0x3c,
	0x61, 0x99, 0x29, 0x21, 0xc2, 0xcd, 0x54, 0xbc, 0x34, 0xba, 0xca, 0x06, 0x60, 0x71, 0x66, 0xda,
	0xbb, 0xc2, 0xc8, 0x45, 0x65, 0x7e, 0xc1, 0x37, 0x51, 0xbf, 0x1c, 0x17, 0x24, 0xc5, 0x93, 0x9d,
	0x12, 0x78, 0xe7, 0x05, 0xd9, 0x02, 0xf6, 0xc7, 0x32, 0xa6, 0x99, 0xb6, 0x44, 0xa5, 0x78, 0x25,
	0xc4, 0x11, 0xd1, 0xd2, 0x18, 0xe0, 0xa2, 0x7d, 0x08, 0x28, 0x90, 0xc6, 0x7e, 0x8a, 0xf8, 0x6c,
	0x73, 0xbb, 0x36, 0xdf, 0xb5, 0x11, 0xc7, 0xbc, 0xbb, 0x6a, 0x13, 0x10, 0xab, 0xe9, 0xcf, 0x96,
	0x88, 0x9f, 0x8e, 0x0e, 0x78, 0x2e, 0x66, 0x02, 0x94, 0x46, 0xcb, 0xcd, 0xff, 0xd1, 0xbb, 0xec,
	0x7a, 0xc9, 0xd6, 0x8c, 0x31, 0x3f, 0x6c, 0x6a, 0x68, 0x4f, 0xca, 0x85, 0xbb, 0x2f, 0xb4, 0xba,
	0xb0, 0xc4, 0x3c, 0xd2, 0x1d, 0xe3, 0x85, 0xdc, 0x26, 0x6d, 0x48, 0x44, 0x89, 0x46, 0xe7, 0xa1,
	0x2b, 0xc4, 0x2d, 0xe5, 0xd2, 0xcd, 0x75, 0xc2, 0xb2, 0x29, 0x4e, 0x65, 0xd7, 0x72, 0x4a, 0xb0,
	0xcc, 0x54, 0x7d, 0xb3, 0x6c, 0xfb, 0x7f, 0x4c, 0xe3, 0x7b, 0x2c, 0x6a, 0x66, 0x0e, 0x0d, 0x4c,
	0xf2, 0x3b, 0xc2, 0x43, 0x37, 0x33, 0xc0, 0x57, 0x96, 0xfa, 0x76, 0x19, 0x30, 0x48, 0x7a, 0x8c,
	0x6b, 0x58, 0x1e, 0x15, 0xdd, 0x80, 0x2b, 0xc2, 0xef, 0x10, 0x17, 0xcd, 0x10, 0x06, 0x05, 0x73,
	0x9a, 0x01, 0xe5, 0xdb, 0x89, 0xd3, 0x83, 0x4d, 0x14, 0x1f, 0x53, 0xa3, 0x66, 0xc0, 0x01, 0x02,
	0x82, 0x01, 0x01, 0x00, 0xce, 0xc5, 0xfb, 0x52, 0x0d, 0xb4, 0xaa, 0x1b, 0x2b, 0x5c, 0x5a, 0xa3,
	0xd8, 0x3f, 0x74, 0x99, 0x1c, 0x05, 0x83, 0x03, 0x43, 0xb8, 0x00, 0x21, 0x0c, 0xf9, 0xe0, 0xb0,
	0x6a, 0xef, 0x40, 0x4a, 0xeb, 0x65, 0xd0, 0x80, 0xe5, 0x34, 0x33, 0x09, 0xf2, 0x70, 0xb6, 0xa6,
	0x1d, 0xb9, 0x04, 0xc7, 0xb9, 0x84, 0x70, 0xd6, 0xa7, 0x67, 0x06, 0x40, 0x9a, 0x20, 0xee, 0x96,
	0x7f, 0xde, 0xa4, 0x28, 0x81, 0x08, 0x68, 0xda, 0x05, 0x27, 0x88, 0xa0, 0xe2, 0x7c, 0xde, 0xfb,
	0xe3, 0x44, 0x1d, 0xca, 0x49, 0x65, 0x4f, 0x34, 0xd5, 0x44, 0xea, 0xa6, 0x3f, 0xcf, 0x9e, 0x7e,
	0xb7, 0x88, 0xbe, 0xa9, 0x73, 0x1e, 0x6b, 0xaa, 0x68, 0x67, 0xc6, 0xb3, 0x9a, 0x13, 0x91, 0x96,
	0x96, 0x8f, 0x9b, 0x2e, 0xf8, 0x1f, 0x9b, 0x4f, 0xef, 0x6b, 0x23, 0x06, 0x5c, 0xc1, 0xfb, 0x39,
	0x61, 0x12, 0x0d, 0x85, 0x04, 0x71, 0xd7, 0xba, 0x9a, 0xfb, 0xec, 0x61, 0xe6, 0x67, 0xc4, 0xdb,
	0x97, 0x3e, 0x33, 0xd7, 0xe2, 0x20, 0x14, 0xe2, 0x35, 0x2a, 0x38, 0x95, 0x3c, 0x56, 0x30, 0x14,
	0xa1, 0x9c, 0xaf, 0x31, 0xac, 0x66, 0x8c, 0x12, 0x63, 0x7b, 0x5b, 0x4a, 0x93, 0x31, 0xb1, 0x47,
	0x3e, 0x04, 0x33, 0xe4, 0x57, 0x31, 0x46, 0x30, 0x82, 0xab, 0x01, 0xe2, 0x97, 0x03, 0x41, 0x78,
	0xb0, 0xd3, 0xa7, 0xf6, 0x44, 0x08, 0x40, 0x7b, 0xcb, 0x7e, 0x24, 0x85, 0x58, 0x79, 0xdf, 0x59,
	0x81, 0x13, 0x69, 0x8d, 0xcd, 0x25, 0x48, 0x41, 0xc1, 0x99, 0x3f, 0x52, 0x3f, 0x0e, 0xf5, 0xe3,
	0x5b, 0xb5, 0x14, 0x35, 0xd8, 0x05, 0xc2, 0x28, 0xbf, 0x19, 0x6f, 0xba, 0x33, 0x4b, 0x94, 0x0f,
	0x2d, 0xb7, 0x51, 0x54, 0x29, 0x6c, 0x5c, 0xdc, 0x57, 0xca, 0x35, 0x0b, 0x69, 0xd9, 0x73, 0x81,
	0x5b, 0xe3, 0x3c, 0x01, 0x02, 0x82, 0x01, 0x01, 0x00, 0xca, 0x48, 0x99, 0x05, 0xc3, 0x0b, 0x91,
	0x9d, 0xa5, 0x49, 0x4b, 0xa5, 0xb1, 0x38, 0xa8, 0xd7, 0xf0, 0xc0, 0xae, 0xf7, 0xf7, 0x0a, 0x3e,
	0x7c, 0x01, 0xbf, 0x69, 0xa6, 0x23, 0x68, 0xe0, 0x1b, 0x11, 0xd3, 0xc3, 0x9b, 0x2b, 0xdd, 0xa8,
	0x66, 0x17, 0x97, 0x93, 0x6f, 0xc6, 0x68, 0xd7, 0xd0, 0x68, 0xc3, 0x2b, 0x4d, 0xfa, 0xda, 0xfa,
	0xd9, 0x91, 0x68, 0x20, 0x10, 0x3d, 0x51, 0xb7, 0x3d, 0x7a, 0xc1, 0x00, 0x53, 0xc9, 0x77, 0x7e,
	0x08, 0x1d, 0x7c, 0xcf, 0x36, 0x72, 0xe4, 0x7d, 0xb0, 0x67, 0x1f, 0x41, 0x5a, 0x02, 0x87, 0xcb,
	0x4c, 0x83, 0xa0, 0x4f, 0xf0, 0x80, 0x4b, 0x3a, 0x66, 0xd2, 0x52, 0x13, 0x77, 0x3c, 0x6d, 0xa6,
	0xdf, 0xd2, 0x3c, 0xd3, 0x6b, 0xb4, 0x7c, 0x53, 0x55, 0x40, 0x22, 0x4a, 0x87, 0x1d, 0x66, 0xd4,
	0xc1, 0x45, 0x2c, 0xeb, 0xbb, 0x95, 0x57, 0x03, 0x4b, 0xd2, 0x4d, 0xfa, 0x86, 0x15, 0x3d, 0xbe,
	0x8c, 0x0d, 0xf0, 0x4b, 0x9b, 0x98, 0xce, 0x88, 0xfb, 0x98, 0x90, 0x56, 0x78, 0x80, 0x7e, 0xfd,
	0x27, 0xb8, 0x17, 0x23, 0x4f, 0xd8, 0x2a, 0x16, 0x89, 0xef, 0x25, 0xed, 0x85, 0x85, 0x64, 0x76,
	0xb4, 0x85, 0xe8, 0x4a, 0x28, 0x7a, 0xbe, 0x11, 0x66, 0x09, 0x9a, 0xeb, 0x60, 0xdd, 0xd5, 0x53,
	0x73, 0x4a, 0xad, 0xc9, 0x06, 0x8e, 0xab, 0x62, 0x31, 0x7b, 0x2e, 0xf7, 0x7e, 0x47, 0x00, 0xc2,
	0x47, 0x5b, 0x61, 0x1e, 0xb9, 0x9f, 0xfc, 0x85, 0xe9, 0x97, 0x1a, 0x4d, 0x56, 0x4a, 0x0c, 0x57,
	0x1b, 0x73, 0x6e, 0xba, 0xdb, 0x82, 0x70, 0xb6, 0xe5, 0x09, 0xaf, 0x45, 0x87, 0x34, 0xae, 0x54,
	0xbf, 0x92, 0xf3, 0x38, 0xc9, 0x08, 0x4c, 0x1f, 0x77, 0x80, 0xec, 0x8c, 0x9c, 0x0d, 0x93, 0x29,
	0x63, 0xed, 0x31, 0x9b, 0xb2, 0x3b, 0x8d, 0x34, 0xfb, 0x02, 0x82, 0x01, 0x00, 0x62, 0xb3, 0x28,
	0x83, 0x03, 0x5d, 0xd0, 0xb1, 0x05, 0x62, 0xa1, 0x35, 0x82, 0x7c, 0xcf, 0xb8, 0x62, 0x22, 0xd3,
	0x65, 0xd4, 0x86, 0x59, 0x31, 0x6d, 0x93, 0x3d, 0x48, 0x98, 0xd2, 0xb9, 0x7a, 0xc9, 0xa0, 0xa1,
	0x05, 0x55, 0xe3, 0x33, 0xd5, 0xb4, 0xaf, 0x4e, 0xd0, 0x3e, 0x71, 0xd9, 0xb1, 0x48, 0x81, 0xca,
	0xa6, 0xfb, 0xe3, 0x76, 0x9d, 0x91, 0xb4, 0xd4, 0x8e, 0x6c, 0x5d, 0x27, 0x38, 0xda, 0x56, 0xdc,
	0x4d, 0xed, 0x95, 0xf0, 0x66, 0xf3, 0x95, 0xad, 0x8e, 0xc8, 0xed, 0xf3, 0xd6, 0x62, 0x70, 0x84,
	0x7d, 0x70, 0xab, 0xe3, 0xe2, 0x15, 0xa5, 0x92, 0x3f, 0x64, 0x76, 0x56, 0xa4, 0x65, 0xfa, 0x08,
	0x64, 0xa0, 0x4f, 0xa1, 0x0e, 0x8c, 0x26, 0x79, 0x21, 0x4b, 0x9f, 0x22, 0xf1, 0x29, 0xa9, 0x54,
	0xa6, 0xb4, 0x5f, 0x0c, 0xa9, 0xf5, 0xce, 0xf6, 0x8f, 0x6e, 0x21, 0x82, 0xe8, 0x92, 0xb5, 0x90,
	0xc7, 0x57, 0x41, 0x97, 0x95, 0x27, 0xb9, 0x32, 0xc3, 0xab, 0x0f, 0x1b, 0x0a, 0x1a, 0xbb, 0x3b,
	0x9c, 0xba, 0xc9, 0xfb, 0x96, 0x68, 0xe5, 0xaf, 0x2f, 0xb9, 0xf1, 0x23, 0xc3, 0x6f, 0x4a, 0xc7,
	0xe3, 0xe3, 0x2e, 0xb7, 0xe6, 0x02, 0x1a, 0xff, 0x47, 0x45, 0x78, 0x16, 0x19, 0x11, 0xf1, 0xc8,
	0x52, 0x51, 0x9d, 0x35, 0x5a, 0x26, 0xc1, 0x7c, 0x18, 0x13, 0x38, 0x04, 0xfd, 0xcd, 0x7d, 0xae,
	0xe2, 0x28, 0xc1, 0x7e, 0xc7, 0x53, 0xf3, 0x60, 0xc4, 0xc5, 0x93, 0x31, 0x98, 0x69, 0x6b, 0x39,
	0x71, 0x81, 0xeb, 0x17, 0xc9, 0xb7, 0xa5, 0xf9, 0x83, 0x5c, 0x7c, 0x34, 0x38, 0x7b, 0x74, 0x4c,
	0x38, 0xcc, 0xf7, 0x64, 0x58, 0x9a, 0x31, 0xa2, 0x6c, 0x18, 0x63, 0x5f, 0xe3, 0xef, 0x9d, 0xf5,
	0x39, 0x8c, 0x82, 0x4e, 0x0d, 0xb3, 0xaa, 0x03, 0xb3, 0xa4, 0xdb, 0xf4, 0x01, 0x02, 0x82, 0x01,
	0x01, 0x00, 0x96, 0x33, 0x77, 0xe4, 0x8e, 0x62, 0x8d, 0xba, 0x88, 0x1b, 0xb7, 0x9f, 0x0d, 0xcb,
	0xeb, 0x9b, 0x84, 0x7a, 0x1e, 0xb1, 0xa2, 0xef, 0x29, 0x5c, 0x7d, 0x13, 0xbb, 0x88, 0x10, 0xac,
	0xf4, 0x13, 0x45, 0x96, 0x7f, 0x9d, 0x3d, 0xe2, 0x36, 0x03, 0xb0, 0xaa, 0xed, 0x60, 0x46, 0xec,
	0x5c, 0xab, 0xb4, 0xce, 0x8e, 0xde, 0x35, 0x51, 0xda, 0x88, 0x28, 0xef, 0x2f, 0x37, 0xbf, 0xc0,
	0x68, 0x96, 0xaf, 0x0a, 0x96, 0x8a, 0xa0, 0x83, 0x28, 0xc3, 0x2f, 0xda, 0x18, 0x26, 0xef, 0x02,
	0xf8, 0xcd, 0x3e, 0x95, 0x37, 0xba, 0x75, 0x3c, 0x8d, 0xd9, 0x7f, 0xb7, 0x4f, 0x04, 0x5e, 0xce,
	0xfd, 0x4b, 0x92, 0x0a, 0x3d, 0xc8, 0x00, 0xc7, 0xce, 0xec, 0x4d, 0x38, 0xbb, 0x28, 0x33, 0x79,
	0x49, 0x8b, 0x78, 0xb6, 0xbd, 0xae, 0x3c, 0x47, 0xb9, 0xdc, 0xd4, 0xd7, 0xb9, 0x26, 0xad, 0x8a,
	0x51, 0xb9, 0x40, 0x2c, 0x84, 0xc4, 0x81, 0x0b, 0x3a, 0xec, 0xd6, 0x00, 0xc2, 0xb3, 0x83, 0xb0,
	0x80, 0x88, 0x89, 0x4d, 0x4b, 0xd7, 0xe8, 0x59, 0xe2, 0xf2, 0x56, 0x40, 0x60, 0x09, 0x0e, 0x92,
	0x99, 0xef, 0xcb, 0xf2, 0xd6, 0xbe, 0x99, 0x40, 0xf2, 0xdf, 0xb2, 0xba, 0xbc, 0x2d, 0xf8, 0x8e,
	0x1f, 0x6f, 0x2b, 0xdc, 0xab, 0xc0, 0x5e, 0x97, 0xe3, 0x82, 0x2d, 0x46, 0x83, 0x89, 0x69, 0xf0,
	0x9a, 0x55, 0xf1, 0x88, 0xfb, 0x5e, 0xf9, 0xab, 0xf7, 0x96, 0x72, 0xa4, 0xd7, 0xe2, 0xaf, 0x88,
	0x1b, 0x8b, 0x4a, 0x96, 0xce, 0x2c, 0x2f, 0x89, 0xa0, 0x38, 0x92, 0xea, 0xfa, 0xb6, 0xb9, 0xd1,
	0xa6, 0x0c, 0xc5, 0xb7, 0x2e, 0xa2, 0x69, 0x9c, 0xb4, 0xf3, 0x17, 0x53, 0xa0, 0xab, 0xad, 0x8c,
	0x90, 0xa4, 0xf4, 0xc7, 0x30, 0xd5, 0x43, 0x43, 0x2d, 0xad, 0xb4, 0x57, 0x6c, 0xab, 0xd8, 0x8a,
	0x4e, 0x77, 0x02, 0x82, 0x01, 0x01, 0x00, 0xc9, 0xad, 0xff, 0xcc, 0xaf, 0x3d, 0xf9, 0x52, 0xfb,
	0x1b, 0xf7, 0x92, 0x0f, 0xd9, 0x06, 0xf4, 0x7d, 0x24, 0x1d, 0x48, 0x9f, 0x69, 0xf7, 0xad, 0x40,
	0x98, 0x60, 0x3e, 0x3b, 0x45, 0xe2, 0x85, 0xa8, 0x9d, 0x37, 0x56, 0x6a, 0xb9, 0x0b, 0xd9, 0xd8,
	0xe7, 0xab, 0x3d, 0xc3, 0xb3, 0x94, 0x3b, 0xca, 0x5e, 0xac, 0x15, 0xe5, 0x25, 0x89, 0x8a, 0x65,
	0x08, 0x4e, 0xe3, 0x6f, 0x77, 0x96, 0xfc, 0x59, 0x0f, 0x62, 0x2a, 0xe0, 0xd7, 0x19, 0x6d, 0x54,
	0x82, 0x32, 0x81, 0xc0, 0x53, 0x38, 0x73, 0x63, 0x76, 0xeb, 0x76, 0x0b, 0x52, 0x23, 0x16, 0xb6,
	0x80, 0x6b, 0xde, 0x18, 0x07, 0xb3, 0x67, 0x7f, 0x2a, 0x28, 0x85, 0x36, 0xe9, 0xd9, 0x33, 0xed,
	0xd7, 0x84, 0x09, 0x8e, 0x2f, 0xae, 0xc4, 0x64, 0xc2, 0x1a, 0x53, 0x5b, 0x42, 0xc6, 0x54, 0x2a,
	0x63, 0x71, 0x0a, 0x1a, 0x2a, 0xfc, 0xa6, 0x02, 0x80, 0xa6, 0x02, 0xcf, 0x15, 0xda, 0x83, 0x2b,
	0x66, 0x2c, 0x35, 0x61, 0x0f, 0x6e, 0x39, 0x4a, 0x16, 0xc0, 0xea, 0xa6, 0xd7, 0x06, 0x6a, 0x99,
	0x57, 0x0e, 0x5e, 0xf3, 0xc8, 0x4b, 0x68, 0x16, 0x02, 0xcd, 0xdf, 0x42, 0x55, 0xa3, 0x1f, 0xd8,
	0x64, 0x71, 0x04, 0xcc, 0xb1, 0x46, 0x97, 0x40, 0x33, 0x83, 0xd1, 0xaa, 0xa4, 0x49, 0x8d, 0xc4,
	0x36, 0xa3, 0xaf, 0x6c, 0x25, 0x75, 0xfe, 0x85, 0x29, 0x46, 0x2d, 0xf4, 0xef, 0xa9, 0x21, 0x0a,
	0x80, 0x17, 0x23, 0x56, 0xca, 0x4a, 0x7f, 0xc0, 0xbd, 0x1d, 0xca, 0x0c, 0xfd, 0x78, 0x07, 0x9b,
	0x68, 0x1c, 0x8f, 0xc5, 0xe4, 0xe4, 0xd2, 0x12, 0x21, 0xa1, 0x84, 0x77, 0xac, 0x81, 0x1a, 0xec,
	0x7c, 0x1a, 0xe9, 0x11, 0x8d, 0x48, 0x01, 0x3b, 0x4f, 0xab, 0x5b, 0x5a, 0x05, 0x96, 0x68, 0x81,
	0x1a, 0x88, 0xde, 0xb3, 0xa4, 0x90, 0xf9,

};

void sigint_handler(int sig)
{
	interrupted = 1;
}

int main(int argc, const char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	const char *p;
	int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */, ret = 1;

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS minimal http server TLS | visit https://localhost:7681\n");

	signal(SIGINT, sigint_handler);

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */

	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
		       LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
		LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	if (lws_cmdline_option(argc, argv, "-h"))
		info.options |= LWS_SERVER_OPTION_VHOST_UPG_STRICT_HOST_CHECK;

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}

	info.port = 7681;
	info.mounts = &mount;
	info.error_document_404 = "/404.html";
	info.server_ssl_cert_mem		= cert_pem;
	info.server_ssl_cert_mem_len		= (unsigned int)strlen(cert_pem);
	info.server_ssl_private_key_mem		= key_pem;
	info.server_ssl_private_key_mem_len	= (unsigned int)strlen(key_pem);
	info.vhost_name = "first";

	if (!lws_create_vhost(context, &info)) {
		lwsl_err("Failed to create first vhost\n");
		goto bail;
	}

	info.port = 7682;
	info.mounts = &mount;
	info.error_document_404 = "/404.html";
	info.server_ssl_cert_mem		= cert_der;
	info.server_ssl_cert_mem_len		= (unsigned int)sizeof(cert_der);
	info.server_ssl_private_key_mem		= key_der;
	info.server_ssl_private_key_mem_len	= (unsigned int)sizeof(key_der);
	info.vhost_name = "second";

	if (!lws_create_vhost(context, &info)) {
		lwsl_err("Failed to create second vhost\n");
		goto bail;
	}

	while (n >= 0 && !interrupted)
		n = lws_service(context, 0);

	ret = 0;

bail:
	lws_context_destroy(context);

	return ret;
}
