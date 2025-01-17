/*
 * This software is Copyright (c) Peter Kasza <peter.kasza at itinsight.hu>,
 * and it is hereby released to the general public under the following terms:

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 */

#if FMT_EXTERNS_H
extern struct fmt_main fmt_cq;
#elif FMT_REGISTERS_H
john_register_one(&fmt_cq);
#else

#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#ifndef OMP_SCALE
#define OMP_SCALE 256	// core i7 no HT
#endif
#endif

#include "arch.h"
#include "misc.h"
#include "params.h"
#include "common.h"
#include "formats.h"
#include "options.h"
#include "memdbg.h"

#define FORMAT_LABEL        "cq"
#define FORMAT_NAME         "ClearQuest"
#define FORMAT_TAG          "$cq$"
#define TAG_LENGTH           (sizeof(FORMAT_TAG) - 1)
#define ALGORITHM_NAME      "CQWeb"
#define BENCHMARK_COMMENT   ""
#define BENCHMARK_LENGTH    0
#define PLAINTEXT_LENGTH    32
#define SALT_SIZE           64  // XXX double check this
#define SALT_ALIGN          MEM_ALIGN_NONE
#define BINARY_SIZE         4
#define BINARY_ALIGN        sizeof(uint32_t)
#define MIN_KEYS_PER_CRYPT  1
#define MAX_KEYS_PER_CRYPT  512

static struct fmt_tests cq_tests[] = {
	{"$cq$admin$a9db7ca6", ""},
	{"$cq$admin$10200218", "admin"},
	{"$cq$admin$4cfb73f2", "password"},
	{"$cq$clearquest$a279b184", "clearquest"},
	{NULL}
};

static char (*saved_key)[PLAINTEXT_LENGTH + 1];
static uint32_t (*crypt_key)[BINARY_SIZE / sizeof(uint32_t)];
static char saved_salt[SALT_SIZE];

unsigned int AdRandomNumbers[2048] = {
	0x7aa03f9e, 0x2be5e9c7, 0x1b5ceb7b, 0x32243048, 0x3cb12e04, 0xe90d2e8f, 0xace8842a, 0xdbc021e2,
	0xdb7e4414, 0x9414168d, 0xec94d186, 0xb0d45b52, 0xefa8a505, 0xee4ac734, 0xee7f3583, 0xf37a1bd0,
	0x258cd1c7, 0xa93a5bf7, 0x347a23f6, 0xbf68b272, 0x5c89e744, 0x4faa8fc0, 0x54fa4bc1, 0x8f4db7cc,
	0xcc78a54c, 0x84012379, 0xbb997725, 0x234612c5, 0x9b8a7120, 0xa2ea15c9, 0xa1b03515, 0xd68c512c,
	0x90cbbcb0, 0xa677c1d3, 0xad4edf73, 0x0fbb4c6c, 0x7a70637d, 0x920c3ef4, 0xc31f9edf, 0xc0c29c40,
	0xe0547468, 0x8af5778a, 0x4910f9e4, 0x553744df, 0xe9e5d82f, 0x9f648de3, 0xc366b6e0, 0xd2b1f83d,
	0xca04733f, 0xcf3603b1, 0x27a7e70f, 0x9981f60a, 0xdd4e2cf8, 0xbcb811b3, 0x5d74ecc1, 0xa48e7106,
	0x16539a54, 0xbdce7967, 0xf08c13d2, 0x2698c222, 0x3343d62d, 0xebf68da7, 0xc2bc9948, 0xea757b40,
	0x37f0de67, 0xa03a4d89, 0x2cfc3cb7, 0x89230393, 0x6711f91d, 0x3812fb31, 0x9a105ad2, 0xf713d767,
	0x4bd0d6c5, 0x4e4f9760, 0x9144e67b, 0x2b5b1540, 0xed6f8cd3, 0x1ab6de1d, 0x94fd67ae, 0x5fec29f0,
	0x9d2f6c66, 0x701797be, 0x7ecd184e, 0x0e58eba6, 0x189fd557, 0xe70a447f, 0x3140d5ce, 0x46d55d28,
	0xf77cb54a, 0x2eb11f3e, 0x331059a8, 0x1bcb4f0e, 0x253b1132, 0x9ed96195, 0xfadee553, 0x60939883,
	0x76ea1724, 0x64ad91a9, 0xce605783, 0x5b17c228, 0xbb45f357, 0xee888899, 0xe36bda7e, 0xb1e6fdb6,
	0xd47b22cb, 0xba2b4b6f, 0x31eb5d46, 0x72865ff5, 0xecc9077f, 0xe59c7a9a, 0xe2484234, 0x6c250954,
	0x75bae5d1, 0x7d8ffc5c, 0x14aa2302, 0x0ea92748, 0xfc9fe3aa, 0x28295969, 0x2f38760d, 0xd335cab1,
	0x7ffcaead, 0xcdeb00fc, 0x5aa4766f, 0xc57deab9, 0xfbeb9ac4, 0xfc59f737, 0xdc8a3d1d, 0xd5a04c8a,
	0x63afc85c, 0x5a062866, 0x7feadb01, 0x9d29586e, 0x4bc63a4e, 0xc2e7a653, 0x80d132b7, 0x306d04e9,
	0x27b4e1df, 0x2d4e5f39, 0xbc7f2f05, 0x8efb7bc4, 0x834368e0, 0xe37ac5c2, 0x07f7741c, 0x7d70f32b,
	0xd8fc568c, 0x52e86793, 0x8a95993e, 0xc086bd26, 0xa0d0c8f9, 0x2262519a, 0xc7a79adc, 0x658ee58a,
	0x94376223, 0x631fbdc3, 0x3433f9b0, 0xe469e054, 0x7b187772, 0x6ea94b34, 0x88515df2, 0xaa7c0e2b,
	0xd688308e, 0x58f4d8de, 0xb3df5108, 0xa977f53d, 0x2f8bf273, 0x98c61997, 0x4c4f7cbc, 0x81a5a959,
	0x8b38b887, 0x17c0d440, 0x03459240, 0x0e049148, 0xfa28dda2, 0xa364ab5b, 0x1f779ce8, 0x9013690f,
	0xbf9b1284, 0x7a1704ac, 0x9deb5e71, 0x1b97bc67, 0x8560a7c3, 0xf95dc105, 0x5be1c16f, 0xa6f93b31,
	0x9d540073, 0xbe36f269, 0xec1ad083, 0xb0d82bf0, 0xa362120c, 0x5d40e591, 0x4383ee00, 0xfc2bd2b3,
	0x03415c36, 0x3514e8f4, 0x39fce373, 0xd8b0083c, 0xfaaf9375, 0xad0cf95b, 0x7618f7bd, 0xa1172c4f,
	0xe4d6fc97, 0xd0d7d41c, 0x665d7461, 0x05a9ffd1, 0xdd7380d8, 0x2c334bab, 0xe89378a6, 0xe1b5a151,
	0xb3d52dec, 0x3298d083, 0x2ef54a8c, 0x06f48c54, 0x34172d75, 0xee7a4a5b, 0x0bcbb2c4, 0xedf58b49,
	0x8bbc49f3, 0x3c5c2876, 0xf8faa9d2, 0xd5c27925, 0x8b31e871, 0xce021186, 0xff86fbb1, 0xce65d1a6,
	0x2ddc62f1, 0x4be585e1, 0x69554205, 0x4a4144f0, 0xd8658572, 0x7ecdf4e5, 0xeca38dba, 0xba099ee0,
	0x9b776c4e, 0x406aa6c7, 0x1b2920d2, 0x20a4fe17, 0x7eefab23, 0xf6cedf61, 0x580739bc, 0xdb0d8ee2,
	0xbcceef67, 0xbfeaeedf, 0x396c4060, 0xccb400cc, 0x2e411cbb, 0x671672e1, 0x25a6139b, 0x9ee52aa1,
	0x7494b0f1, 0xd967d261, 0x7c3351f6, 0xcb1b564c, 0x3ebd0cbf, 0x1e451dcb, 0x197de7e2, 0xb988e765,
	0x4a56963b, 0x1c1c8571, 0xc77711b8, 0x56a92f0e, 0xb480334e, 0x283314d6, 0xa2905c9a, 0xfb7a96f1,
	0x033f43a9, 0xea71059a, 0x266c4710, 0x4528c417, 0x6f3a6608, 0x45699d70, 0xd72db23a, 0x411dfdeb,
	0xed52ad3d, 0x9c1cd588, 0x15e8e29d, 0xe36911aa, 0x2e65957b, 0x085d2e03, 0x419ee083, 0x0b257fb9,
	0x30aa0a12, 0xdc7358a7, 0xaa9d75b8, 0xcc040ac1, 0x178035c6, 0x53916d0e, 0x54f00e79, 0xf5de4034,
	0xd66ff28b, 0x77fd3f74, 0x512ca7d5, 0xd6e48b64, 0xbeb2e6e6, 0xfbbc9e4d, 0xfae7f661, 0x8d529091,
	0xd43abeab, 0xc445eab2, 0x31c276ea, 0x3b02aa3c, 0x104555ae, 0xf31fca4c, 0x7c86bda3, 0xbb8fa8e1,
	0x561ef23b, 0xc7c3ddac, 0xc049a3b2, 0xb55ce3c1, 0x793c8199, 0xaf9cf13b, 0xd1bedce3, 0x5c9d8d47,
	0x975973cd, 0xcc747711, 0xecc9b8cd, 0xc38edb0e, 0x10d46466, 0xd1742665, 0x34de2abc, 0x6af0415f,
	0x33eaabe7, 0xeb5b3c88, 0xe45dbb41, 0x46a60558, 0xec3ee7fb, 0x8648812f, 0x779004e2, 0x957369ad,
	0x2859ec9e, 0xd7500913, 0x7ffdbaff, 0x653c7581, 0xcb2fbe4e, 0x889a0521, 0xc2c47e53, 0x27357d53,
	0x6f8e3752, 0x14fed547, 0x78f92e55, 0x12168b52, 0x0cecbc88, 0xadc37f82, 0x5c65b902, 0x464adda3,
	0x51701de4, 0x94f581be, 0x3a8bfe19, 0xd928e9fc, 0x4e621fd7, 0x463c7b1a, 0xa5d610c6, 0x5225860f,
	0x31f71d52, 0x59da3e38, 0xf979401f, 0x457831b6, 0x19b19ff9, 0x212898b8, 0x9d9a330d, 0x9cbbc514,
	0xb1b28903, 0x92161213, 0xcd13324f, 0xa7d072ff, 0x314eb9e8, 0xbf0981ce, 0x00212756, 0xa4aa1438,
	0x4b8a6088, 0x35cfaec8, 0xdd1def33, 0x7a868772, 0x43faf1be, 0x7140cda5, 0x86a8ccf2, 0xaa95fc06,
	0x19eaaee0, 0x309e6b2a, 0x6943426b, 0x2c687aca, 0x9aa1e0dd, 0x5bf701cd, 0x129364b8, 0xeac81cbf,
	0x3a1193ec, 0xd224cb9d, 0x0bbd5135, 0x82eb9c9d, 0x3c965dbf, 0xf6779412, 0xc71f63da, 0x28eedaa1,
	0x63a4f5ba, 0x07cdd4e0, 0x072faeff, 0x21ff534d, 0x0d35cc3f, 0x4e0d4e2e, 0x75851a2f, 0x2e6779f2,
	0xda7a104a, 0xbf129c30, 0x8b6305e9, 0x8d5bd156, 0xc3143454, 0x01f988a1, 0x55930029, 0xfc2f1619,
	0xc4f9f598, 0x60c2ca05, 0x0c7138d9, 0xb3a34e45, 0x2f228b1a, 0x8ce79f7b, 0x0433eefb, 0x0bba45cc,
	0xd9b8abdc, 0x77798336, 0xd46b5757, 0xf3f4308a, 0x8e4984c1, 0x18d3dff2, 0x43cd6cbb, 0x475cab00,
	0xa0baa888, 0x7d688498, 0x33975596, 0xa8e3b619, 0x8258d065, 0x1e939418, 0xae47dcaf, 0x56a1311b,
	0x41650078, 0x9ca8280b, 0x2df5bb50, 0x8ca64fba, 0x840e8608, 0x0ea9643d, 0x64666095, 0x76ae3ba8,
	0x31409202, 0x5c076878, 0xa15154f5, 0xa33ad25c, 0x5d76ea5e, 0x89d98e0c, 0x7d0888c2, 0xce8ea846,
	0x5422cb9c, 0x4ed2233e, 0x7a956e89, 0x9f132d5d, 0xb21ecaf1, 0xaae3eeb0, 0x5b6ca2c8, 0x4dc9ee83,
	0x970b1474, 0x990ccca3, 0x223acb1d, 0x8136038e, 0xd3f3f287, 0x869f34e7, 0x11966837, 0x9952dab8,
	0xdb3077d8, 0x9fe3cdd8, 0x892915cf, 0x658ae998, 0x6fa07e4a, 0xe28746ed, 0xa7821e30, 0x59d88b18,
	0x1d1a7922, 0xe19a82ec, 0xd7e17b1b, 0x4d77aaee, 0x954c1f25, 0xad312a39, 0xb2fc818a, 0x29d17df3,
	0x498f9862, 0xbba03635, 0xa4be43d1, 0x712e234c, 0xee3d9921, 0x87d1e33d, 0x8606d3cf, 0xc252595e,
	0x6dd206de, 0xd381f5bb, 0xc0d9420f, 0x459e1991, 0x0c1892b8, 0xc79fd935, 0x416cdabe, 0xb57690a6,
	0x087ea862, 0x763aff63, 0x15337abd, 0x6455d3f6, 0x524034c7, 0x7bb42336, 0x41669b3f, 0x26574372,
	0xbc949e2e, 0x211af8fb, 0x7416c18e, 0x6c7c01c8, 0x20edce1e, 0xf858ab54, 0x2cde56af, 0xd4a31e32,
	0xb6947234, 0xa8694d27, 0x90f90227, 0x257413b1, 0xacd234a4, 0x17e19de7, 0x71b63409, 0x8d67fb39,
	0x0217d598, 0xcb548c2d, 0xce3c13da, 0x2d6c3984, 0xcc3c93db, 0x6d10ae7c, 0x893b3eb1, 0xcf58d7d6,
	0x8038f673, 0xf5d60b09, 0x85c6ea1c, 0xbe121052, 0x4519c0cd, 0xe4032569, 0xc0a8e700, 0x2a72e706,
	0x8534a64f, 0x9f4a1361, 0xd4ef416b, 0x43336420, 0x323e2a96, 0xd5ea02c9, 0x59782e1b, 0x4c4e02e2,
	0x75171d80, 0x6aa8fba1, 0xfdc8233c, 0x36eee3c0, 0x18643046, 0xce2d3fe4, 0x73ed0154, 0xfc8dfea4,
	0xd43c9f90, 0x2bbdf9fa, 0x0dd43b40, 0x74c54101, 0xd4dd41ed, 0xf06a79a5, 0xb85687ae, 0x5f85dfbd,
	0xd630cc07, 0xd7105cd7, 0x46f30505, 0x15575a70, 0x1b3dff40, 0x24a9e552, 0xcc52f6c4, 0xddca62fe,
	0x094da898, 0xbbf990e8, 0xd2531586, 0x79a6abe4, 0xd67bac65, 0xaf2e08a7, 0x36c28310, 0x85f3fb0a,
	0x188a8332, 0x7ed37c9c, 0x9fd8d6c6, 0xba87d2eb, 0x0331d15f, 0x11438530, 0x86848311, 0x81b555f1,
	0xf582213f, 0x1b39290e, 0x644ed913, 0x1a15eef2, 0x30df2149, 0x9aee5345, 0xdaf33a78, 0x1a36f066,
	0xbc6d937e, 0xf0553748, 0x3c4d51a4, 0xac14c751, 0xc691eb3a, 0xd08c9cde, 0x93a2716c, 0xd46bddb8,
	0x86504249, 0xd331a584, 0xbb0a34b0, 0xe095b710, 0x637512a4, 0x1b4b520d, 0xd51e5b11, 0x561b6ea5,
	0xf1d870ab, 0xed478d20, 0x63e61e6d, 0xf159f48a, 0xc5e9d72c, 0x5c5ce2e0, 0x65f1e7a6, 0x3d98331a,
	0x54596f93, 0x98a1fcef, 0xd603f018, 0xdf0a0fe9, 0x3133cc21, 0x1f66fa5a, 0x34d24f08, 0x3f6ffbdc,
	0x8f5a7faf, 0xb6f7f344, 0x44e8f0fc, 0xb84d7ede, 0x6a193aa5, 0xd7846b26, 0xdc93b0fe, 0x8eb61507,
	0x57502a82, 0xd0068b11, 0xbfc1a056, 0x5766badc, 0xcd80e8cb, 0x3ad80ff6, 0x6b07c122, 0x2fc821a0,
	0x924b7f93, 0x0cb37099, 0x8f078060, 0x74ea3ad4, 0x12006e5d, 0x513db72f, 0x9ed5dc6d, 0x0f8763e1,
	0xd3746c15, 0x69043f07, 0x2f1f0e29, 0x1e134f46, 0x1b673ace, 0x352869fa, 0xb903f872, 0xf44e83a9,
	0x0358ba50, 0xad94f27d, 0xe3799bc7, 0x1aa584be, 0xbf6496e8, 0x020edea1, 0x8f9f1e56, 0x335d8aad,
	0xb52f8536, 0x1047d008, 0x705fc7c5, 0x82a15021, 0x38ec73c5, 0x83bbeec3, 0xafe5dfad, 0x014d9399,
	0x132c1fdf, 0x90781f7f, 0x9fdf14ca, 0x4f9e619e, 0xcffb2139, 0xd868773d, 0x53ab6332, 0xc7139a87,
	0x069c6fa1, 0xaea8ddc8, 0xebf10a04, 0x594a74b2, 0xaf80e4a8, 0x4ea207a5, 0x0017f466, 0x780f7b67,
	0x35b7e34c, 0xd5c9b942, 0x55096faf, 0xf46db3d1, 0x4ab94cc8, 0x2b69bb09, 0xd95f0cbc, 0x97694c99,
	0xd1cfe216, 0xb21e227d, 0xea436ce4, 0xbd06f5f2, 0x69b7ed73, 0xdc368c1b, 0x6bc96d16, 0x00af13db,
	0x33bac433, 0xe3b28392, 0x985ce7fa, 0x49981994, 0x3acd1335, 0x8a72de1d, 0x41500863, 0xa2eba987,
	0x1928c7d1, 0xa599918a, 0x9b8ff4d9, 0x698b190b, 0xfdb34c56, 0x53fb5413, 0x4879c088, 0x3bef71a7,
	0x54f32abc, 0xe3fe3908, 0xa8b79d24, 0x50f69baf, 0xe87713e1, 0xceeaa849, 0xb5054c6e, 0x99076ad6,
	0x4cecff0b, 0x21880bcb, 0x78b035b6, 0x2ba89560, 0x82d35f09, 0xeb2456ed, 0xc404c4b3, 0xd88f0c49,
	0xd07cb42f, 0x37c1d01d, 0xb81701af, 0x1b435610, 0x6945f3b8, 0xd1d51325, 0x1b89be74, 0x67fb8124,
	0x48f5ee8e, 0x80181ba7, 0x10817a0c, 0x931890cb, 0xdd21311d, 0x8530cda9, 0x8e31ca20, 0x3bc790ba,
	0xbb769f75, 0xd87d8134, 0x1e586f4e, 0x0afef375, 0x0756d640, 0x7e89338e, 0x7ffd1904, 0x83dbd0c7,
	0xcfd2f15b, 0xfd2675d6, 0xab8cd735, 0x6c95ef68, 0x4e713995, 0x90835487, 0x193af1c9, 0xfe13ce8f,
	0x82beba7f, 0x8a9c42b3, 0x44635fa6, 0xb5b71efc, 0x12b4e48a, 0xf7af1338, 0xd1ac39a8, 0xbacc03c1,
	0xf24f7267, 0x8e0f9e90, 0x189cabb4, 0x3c8019a1, 0x4abecfee, 0x9d972bc8, 0xf5e931b0, 0xc26b7de1,
	0x1d56f5d3, 0x77079560, 0x04ae0fac, 0x03220a78, 0xe0b95f42, 0x92d112ce, 0x2bd2afad, 0x80755047,
	0x41a23ec9, 0xb49f602c, 0x3761a3f6, 0x935ee1cf, 0x1b64abe7, 0x006e19a9, 0x0b720985, 0xd8155ad3,
	0x15539fef, 0xe2a32706, 0x86ac3c63, 0x5b606e14, 0x529e4a4e, 0x5d862f33, 0x68db23a7, 0x276819b7,
	0x8c1e1a2d, 0xb8bdd15a, 0x7d0eb98e, 0xd3301dee, 0xeddd8b74, 0xb73dad26, 0xded6a141, 0x1f233c79,
	0x324da954, 0x3eb44363, 0x530f5631, 0x60aa4163, 0xa0a53376, 0x46447217, 0x21c4a239, 0xf2813423,
	0x72ec278c, 0x87b8bf19, 0x630ff66d, 0xd83cf3a0, 0x9f889ff6, 0xb99fc512, 0xce97a6ac, 0x018016d3,
	0xb6c29965, 0xc06b4718, 0x837bdedb, 0x4cf09f8e, 0xe9dc4763, 0x7696df71, 0x03182a80, 0x4437d055,
	0xea83a340, 0x749af1a9, 0xab377e67, 0x9c568a33, 0x15a7cce4, 0x7a0a58e1, 0xfa221945, 0x55bf880c,
	0x31607015, 0xb3780687, 0x5cb9bd63, 0xfec69f6b, 0xd2e21926, 0x8b3dad02, 0x4f3b41bc, 0x75e0e8cc,
	0x10589810, 0x5729ba38, 0xb0a588e1, 0x7c821c0a, 0xbc0075a0, 0x3ab20949, 0xaff6b176, 0xc8497f0b,
	0x1871d8dc, 0x1ff4a370, 0x824bcf3b, 0x6eca58a6, 0xa009169f, 0xf610f928, 0x23a867df, 0x383522b8,
	0xc5f83a93, 0x2c3cd773, 0x55dbf231, 0x7c64920b, 0x5ef0d8fa, 0x31db2bfe, 0x45439b02, 0x230e546f,
	0xc48477fc, 0x7e83db3c, 0x989f9765, 0x36d109b3, 0x22d9ae17, 0x5b19aa54, 0xec1161f1, 0x4abd5b16,
	0xf3264a50, 0x0488b52e, 0x3e0fba91, 0xd69b929c, 0x083c42ee, 0x4c37fc2d, 0xada375ec, 0xf4f80d69,
	0xe645063a, 0xa9aa2bf5, 0xfb3169fb, 0xc2a162ab, 0x794a6f5b, 0x68838c2c, 0xf40b2c40, 0x4ef814f1,
	0x4f847bc0, 0x5333a90e, 0x9b3f17a3, 0xa9af089f, 0xe1991375, 0x3f7384b6, 0x1fb942c7, 0x97365bba,
	0xd3b6523a, 0x2d76c616, 0x5d7b944c, 0x9dffc0d9, 0x6ef0ca09, 0x745d606e, 0xc25f0d1d, 0x68335b59,
	0xa5a9aa32, 0x5f609d6f, 0x58012e9c, 0xc138c974, 0x0c62589a, 0x54c7026b, 0x78576d4f, 0x7ab860fe,
	0x673810e2, 0x3cd73808, 0xa20364a5, 0xc750409a, 0xaff51879, 0xa4a598b0, 0xc4f113e9, 0x28da785e,
	0xf41e4752, 0xf0608ba0, 0xa3434cf2, 0x5591372b, 0x4aa08a13, 0x0ec92e82, 0x714a0880, 0x643b37f1,
	0x8a2bb7a2, 0x7db8db33, 0x6f800d7f, 0xce583d48, 0x07517baf, 0xdc1d9e7d, 0xab9a13cd, 0x6f2614d0,
	0xbf021e62, 0xbbd113d2, 0x426f03a7, 0xcd1cd97e, 0x321886e0, 0x894b01a2, 0x6ef2e9aa, 0x4aa8b1e0,
	0xc2c2ff9a, 0x37a2d9a6, 0x3e4b059b, 0x4635f62b, 0x0ea4b349, 0x4bf910ea, 0x24d44dae, 0xc60dba2b,
	0x017df1de, 0x7d7e20c1, 0x6db4c345, 0xfa704510, 0xcf23dded, 0x0c6f7db9, 0x863aea75, 0x2266ac92,
	0x6434e1ec, 0xbd1f2c9f, 0x3a1f0877, 0x45c04d11, 0xc8c0a4de, 0x435beef5, 0x02c271e8, 0x86b845ba,
	0x641c1fee, 0x33c0762f, 0x79d746f3, 0x2071528a, 0xd96a62b5, 0x6457d349, 0xbcf4079c, 0xb97d7c3f,
	0x7bd9b37c, 0x8d22cb6d, 0xd108173d, 0x0d681c0c, 0xcf551b4f, 0x2c39510d, 0x9ebc64c9, 0x1ef42072,
	0x12508fce, 0xb08f1e43, 0xe189dbf9, 0x210af02d, 0xce1ada8a, 0x5843de7d, 0xc8f2eb59, 0x97ebfc1f,
	0xde0b9b2a, 0xc7ca01e9, 0x0a605020, 0x9d3265f2, 0x0ad04e2b, 0x3d203fe2, 0xb1cb3883, 0x7e3ee25a,
	0x02483609, 0xac38260f, 0x6d028d74, 0xdffeb968, 0x1c1d8074, 0x53be10b2, 0x184ed1cb, 0x4e0bc9e4,
	0x7d600a82, 0xd5efa6f7, 0xbe35cb9d, 0xe7aab6ff, 0xd24efe88, 0x4f95ee51, 0x531a720f, 0x9281a4d1,
	0x349fe52e, 0x0372e4fb, 0xc0f4e1fe, 0xd22ca046, 0x7ad0d314, 0xb1917a84, 0x65ddfdc8, 0xf9e7d7f2,
	0x731c3ae6, 0x3ccf4730, 0x97503482, 0x9a75a545, 0xf3713091, 0x7074223f, 0x5ac2d900, 0xbb5bbda3,
	0xf3ac9fe5, 0xc5f432f7, 0xc06ba069, 0x59d175ee, 0xf6624a20, 0x5ad27321, 0x44ec92f7, 0x5cdcda3c,
	0xe4bfbaae, 0x61a61fdb, 0xeb78d322, 0x8a225c70, 0x5af8aa8e, 0x98dfebfa, 0x5e09f08a, 0x955bc9fa,
	0xaf28b29a, 0x9795588a, 0x3a880cb4, 0xacfb27cb, 0xfadfac71, 0x26f58cd2, 0x8a9a8481, 0x5c73cf8f,
	0x0a764745, 0x8e7525c6, 0x5a48f4bd, 0x9a038d74, 0xbe052e79, 0x50b8ab7b, 0x847b3fd6, 0xfd7af126,
	0x910b5700, 0x89f74a62, 0xeacb449c, 0xba8ab0ec, 0xfb0d55df, 0xc9000ded, 0xa0e4fdc0, 0xd7fdbcbc,
	0x776ad910, 0xf8ebe464, 0xb78641a8, 0x9e19f911, 0xa39f3c7f, 0x28798682, 0x770c0c67, 0xd6618589,
	0x3824cfb9, 0x284362cc, 0x6f801c7b, 0x44473f26, 0xeb97128c, 0x9f5e29f8, 0xae790aec, 0xda7ce4f2,
	0x2c37baad, 0xe0733abb, 0x2f79c97c, 0x56f014f5, 0x436f9427, 0x3fa2c41e, 0x03f4016e, 0xf29583a0,
	0xdaa06497, 0xaef999c4, 0xde6098ea, 0x49d744e4, 0x9ac363c4, 0x71b11295, 0x1056e853, 0xa50f1c79,
	0xf58aee75, 0xd2d98a60, 0xcc086a21, 0x633306d2, 0x43516bca, 0xf24c6958, 0x75bf4c20, 0xd1aa09ce,
	0xd98701cb, 0x6497d417, 0xf09334da, 0x74005f98, 0x72241183, 0x3b7be9f5, 0xf4e1cf5d, 0xb44fe087,
	0x9b347ba7, 0xb662e5db, 0xaf570647, 0x1b3d5eac, 0xf2d33276, 0xf694fa88, 0xfbf7ee8c, 0x722c4ccd,
	0xb164605e, 0xfa22d883, 0x716dbb44, 0x00289aec, 0xfeb07061, 0x82f6934c, 0x7b9f20ae, 0x3d2832b2,
	0x2127f401, 0x7e614a69, 0xb48b17be, 0x1e9061e2, 0xcdacbff9, 0x29519f11, 0xeb31bcdb, 0xc6a9a031,
	0xa1fc0693, 0xdaaacb3d, 0x00177418, 0xd3b045d4, 0x575792d3, 0x7cc871a9, 0x4107f556, 0xd5dd7111,
	0xecb8a31b, 0x88f6384e, 0x3e9559d2, 0xb87eab52, 0xddc8d45c, 0xcee4cadb, 0xb9672b94, 0x786129c5,
	0x8a238446, 0x5cc9ce36, 0x897b89a3, 0x248f233f, 0x93a01697, 0xa9e32583, 0x46318359, 0xb60bd733,
	0xcb344995, 0x93fe4889, 0x60477ed8, 0xe0baae66, 0x6e5f97b1, 0x36c221f3, 0xdecfa578, 0x45a9e7a5,
	0xee38c8e3, 0x94209438, 0x0850dc00, 0x21d4792c, 0x0bff88c7, 0xfa703b77, 0x30ce06ab, 0x77bd6475,
	0x07909087, 0xc8d5076a, 0x613d19bb, 0xc50da2d5, 0x77314616, 0x10f1ca59, 0xf5ed6e4e, 0x98132237,
	0xa67d0346, 0xd17c5af7, 0xe49b092a, 0x2c6ac00a, 0xd6a18085, 0xa608ec39, 0x6f93259e, 0xfcd1c805,
	0x119aa599, 0xee224e6a, 0xcd42adfc, 0x5c62cac2, 0x62739935, 0xbb2899f7, 0x3f708519, 0x6cbf5af7,
	0x23dab7fb, 0x4c335483, 0x30835257, 0xcb83d782, 0x60495aba, 0x9631c6ff, 0xcdab4568, 0xa9ddb16e,
	0x41f50aca, 0x12347371, 0x8d2d523a, 0xf5106c41, 0xb257c349, 0x8da2838f, 0xf1dd89f0, 0xac2752a4,
	0x55ea2767, 0x825e66f9, 0x70249c8e, 0x5c98333a, 0xb3bf59c7, 0x4600b593, 0xdb32d803, 0xa009b1a3,
	0x573c455f, 0x93c9610b, 0x0f076b69, 0x362b989e, 0x31d3ecfc, 0x86f3c663, 0x4b7db83c, 0x1333370e,
	0xef093b3c, 0x8fbfc042, 0x1299de1d, 0xb59f15e9, 0x70a306d4, 0xcdd49b7f, 0xe0e78ac4, 0x3c327e27,
	0x88f19b26, 0x0e979bc4, 0xedffe30f, 0x4f5001a8, 0xc4f15293, 0xd622c7ff, 0x51effe17, 0x50bba6a6,
	0xce0d506b, 0xb7652789, 0xc41720af, 0x53e907a8, 0x616fa770, 0x5d033835, 0x1039b042, 0x61e5b922,
	0xfe81924a, 0xf355025c, 0x6dc5a3f1, 0x3c8f6e63, 0x5b5fe252, 0xa8b6f433, 0xac6fbfa2, 0xf6b0e5b9,
	0x20f5b0b4, 0x70f68390, 0x61ad174c, 0x2d359dd8, 0xa4ad7281, 0x0e00eee7, 0x79abb5de, 0x3405bcf7,
	0x581d66b4, 0x0279f078, 0xd38722d7, 0x247fe970, 0x3c031225, 0xda242c14, 0x2a2b7ae6, 0x218cef99,
	0x4005f76a, 0x1ef3e30a, 0x03b957a3, 0x5f4bc0bb, 0x682bc52e, 0xfd640d9d, 0xbddf1fc4, 0xd8e6d767,
	0x69b15d06, 0x5f62cd28, 0x7119ada8, 0x1c0b776a, 0x59bd0273, 0xf6a6a315, 0x62b75968, 0x3f216017,
	0x10cb81c2, 0x055266ff, 0x7e4bab81, 0x4959ee06, 0x60876e87, 0x5d6ed71a, 0x8cf8c0fd, 0x7df4d611,
	0x3cc63f3f, 0x2fb7033e, 0x26154a95, 0x34057009, 0x822e5c53, 0x8867b292, 0x2d784f05, 0x3e9e7ea3,
	0x760d9500, 0x780a7bc4, 0xd36732b2, 0xf36d5b98, 0x20c71e2b, 0x2bc8e9a7, 0x096be252, 0xb9b9236b,
	0x8071dfd4, 0xacfc1e15, 0xcbbde7c9, 0x5f33b8f0, 0xbb63b209, 0x551eb7d6, 0x113e505f, 0xb076c142,
	0x43044856, 0xaf552da0, 0xd921227b, 0x10817109, 0xdf903cff, 0x1419a111, 0x620ee003, 0xb614f2da,
	0x321961d0, 0x650e8b24, 0xd5b909be, 0x79c7fbf6, 0x5849e0b0, 0x9342c0ba, 0x0fbf4e5e, 0xcd01430f,
	0xd80f501a, 0xcbde9f45, 0xd1f7c565, 0x20fd41c4, 0x5fef8bb7, 0x755e7eca, 0x611c1ad5, 0x3e40beb2,
	0x9e864b82, 0x69973554, 0x1d1d7c51, 0x3541f0a5, 0xa2aef7cd, 0xafa3c0e1, 0x1140c1b7, 0x82f9c5fd,
	0x0e53bda8, 0xeb48c46a, 0xe6e5f766, 0x686f4800, 0x84ff9918, 0x3a299c37, 0x7c3a324f, 0x25295743,
	0xb9131e22, 0xa9467eff, 0x88bd0285, 0xfe7b56c4, 0x3774cab6, 0x8b709b30, 0xe98eb68f, 0xf99ed1ae,
	0x21c8eda6, 0x8ceb5ee2, 0x24474545, 0x8709e6f9, 0x72b0e80f, 0x66d9c7f5, 0xe2ca210b, 0xdfbd7e43,
	0xc9c5335c, 0x68b085ce, 0x021f3760, 0x2805cab4, 0xc2817224, 0x33054445, 0x05fc88f5, 0x4ef25d45,
	0x1aed0636, 0xcaa0b185, 0xa8eb115b, 0x595bde5a, 0x7772b19b, 0x4dd2ca0d, 0xfc5c2185, 0x78c831dc,
	0x4ffef944, 0x78430100, 0x03850200, 0x58c49038, 0x5aeee65f, 0x818f6b1d, 0x21ac1b4b, 0xecf90ec1,
	0xfc678952, 0xe8340c7c, 0x51c1d8ca, 0xbfc33274, 0x1a83815c, 0x49512c1b, 0x4d262ce3, 0xab2674eb,
	0xa84710f8, 0x96c698e5, 0xd045f84b, 0xc2ff8580, 0xf7a42aa9, 0x6eee74eb, 0xfde04cd3, 0xaa18c506,
	0xcb3c95dc, 0x4761f04c, 0x01a75b02, 0x395cb0ce, 0x6269b03c, 0x7a4976a5, 0x982a10f3, 0x42cf92cd,
	0x8c110454, 0x055408da, 0x0ada639f, 0xf3d74ec6, 0x3525b6b4, 0x21d1437d, 0x5ec84bc1, 0x5a8256e9,
	0xe03bc870, 0xd4ec26ad, 0xff77bf3b, 0xf1af0c32, 0x8d8ed6df, 0x02fa5021, 0x7d04282b, 0x9d16455e,
	0x5656ed1c, 0x86015646, 0x4e911190, 0x97108eb4, 0xb95d284c, 0x9d379b17, 0xe7ea9203, 0x57092245,
	0xb821ff82, 0xefcef176, 0x4c7885d5, 0x36a3990d, 0x4949c392, 0x3a37d261, 0xe3542623, 0xb8f04501,
	0x49554eaa, 0x5784625e, 0xc5fdb9d5, 0x089b3dc0, 0xf4741645, 0x59c2b88b, 0x044c5ecf, 0xb5b895f4,
	0x5e643350, 0x59fd5974, 0x18af0a5e, 0x1690a41e, 0x7afd7df8, 0xce194b1d, 0x7348105f, 0x55bdac13,
	0x992f1f30, 0x7576c408, 0xd5986ca9, 0xfb5db4bb, 0xdd077a3e, 0x1555fd40, 0x8f43e64c, 0x0891c0b2,
	0xc034b002, 0xa178edf1, 0x73c6aa69, 0x43768cbf, 0x50518a88, 0x5cea403e, 0x66621f52, 0x6454ada9,
	0xfc5353d7, 0xb6367c33, 0x36083ea3, 0x7c6370d7, 0x871ce0df, 0x996a9db6, 0xfc85b23e, 0x4832a696,
	0xd9e633e3, 0x8d57a607, 0xb065c6f6, 0x6b14e612, 0x640a3223, 0x7d40f089, 0x00e856a5, 0x02b89be7,
	0xbb17308d, 0xf495ca2e, 0x36d73234, 0xe6efc3f6, 0x3e34538a, 0x50cae00d, 0xc659ceea, 0x8f9c4fd0,
	0xffd2a14b, 0x8f3c0302, 0xad6bf0e2, 0x7da1cdf3, 0x587bb565, 0xf5a45c06, 0x7b58dfac, 0x8d5689a9,
	0xdd8193b7, 0x0fea45f3, 0x849600fa, 0x71d1f8ee, 0xd9cd7337, 0x10ed0d73, 0xbfb107e3, 0xae79885f,
	0xbc424999, 0xefa0a006, 0x66fcd210, 0xb8221c2c, 0x4e4fcb90, 0xbc19bd01, 0x13a6165e, 0xb2456d5c,
	0x9de9927f, 0x5cad9384, 0xc5b8aca8, 0x709eaed3, 0x025d5327, 0x3334f98d, 0xc8775813, 0xa7e3a7b5,
	0x4900d83c, 0xdb3e88aa, 0x798cf72d, 0xc5201d9c, 0xd301c8a5, 0x24aba004, 0x7fb00d06, 0x96b07431,
	0x07e817d2, 0x5dc5e25c, 0xe3565527, 0x4e1527e2, 0x9b19ac07, 0xb97a792b, 0x810e2f87, 0xb7e67428,
	0xbdc1ad62, 0x4d68b464, 0x86743e97, 0x5ac3a913, 0x74a330bc, 0xedbd0163, 0x17c1ca06, 0x3762d54b,
	0x67cac477, 0x57fb1db4, 0xca5c3afa, 0xe9cdec2a, 0x3f517285, 0x6c123489, 0x8c9c347f, 0xdc352535,
	0xfd72d8b0, 0x615adae6, 0xd2149734, 0xb4d087d2, 0x50777523, 0x152794d7, 0xa1de388e, 0xacbfb4a7,
	0xcc792a3f, 0x0c9beff1, 0x4518932e, 0xbb10562a, 0x0231bbab, 0x34d6b5e4, 0xe56b9d51, 0x1179cf3a,
	0x7c9cca04, 0x7e6804dd, 0x43749e37, 0x474abbed, 0xeca9da66, 0xdbc9f38a, 0xbd90f4f2, 0xca3e3208,
	0xf6804e7b, 0x6c2f368b, 0x71865b53, 0x477e86cf, 0xf9556567, 0x99f1268d, 0x5b0d3742, 0x5c7759e9,
	0x329a93f4, 0x6edb40ef, 0xd8238cf4, 0x14c6653d, 0xda60ef87, 0x845f263d, 0x5ecb0b8a, 0xc0318caa,
	0xb6e9c384, 0xb375f88e, 0x8bbc46cc, 0x9cf69f27, 0x5ad28fa8, 0x458c2af7, 0xc848d9c1, 0xa8fc2598,
	0xef9d6e05, 0x72e2b656, 0x548e9bd8, 0x57c6446f, 0x1ed6fa9f, 0xee2aabba, 0xa0ad6aeb, 0x9cf39373,
	0xe1ef4e1e, 0x84aaf630, 0x51839901, 0x1fd11926, 0x7e8d187e, 0x3f0c9a19, 0xd15b2dd2, 0x87bcba56,
	0x4e4abd25, 0xfe14d638, 0x632c77f2, 0xdd2fb790, 0xa7172b76, 0x51b3b07e, 0xbc087c39, 0x08e4bfcd,
	0x835236c7, 0x2f55bb11, 0x01de9ed4, 0x3dfc98ce, 0x28776e03, 0x98fb6143, 0x3b05e401, 0x0999f936,
	0x673cda1e, 0x9075e503, 0x80dbd8a1, 0x1d113be7, 0x368624b0, 0xd06b7118, 0xdc0378f6, 0x0c6aee59,
	0x3c77c121, 0xb75108d5, 0xa1d6c1a7, 0xdacd595b, 0x71d9b0e5, 0x4afe1c59, 0x4a2cd1e8, 0x8c3eda88,
	0x97ae95c2, 0x6bfacf26, 0x47182448, 0xdbd882ef, 0xae660019, 0xb0986ca0, 0xe8daf1be, 0xb18b34dd,
	0x6ba59afa, 0x560c57ea, 0xf6be34ba, 0xcd89f600, 0x6403fcfe, 0x5e429947, 0x7d4886e1, 0x8018e754,
	0xc17251e7, 0x36eab7c6, 0x9067ad78, 0xe95f1fe6, 0x44961501, 0x8619621c, 0xdb13e77d, 0xd5d6124d,
	0xd36f8ac0, 0x9395356c, 0xf713d1e1, 0x2b3c9d15, 0x4dfb98f9, 0xb553ff7f, 0xd1675da5, 0x382dcfdb,
	0x659ed198, 0xa0bfe7d7, 0x0c1fe7f7, 0x6ae7194d, 0x1966c9fe, 0x369f1f79, 0x1137c2c2, 0xf7a06345,
	0xfe3f544b, 0xac35a2f4, 0xd7aea537, 0x9b37ff3b, 0xfc395a7b, 0xf3c2710a, 0xf7ec7804, 0x5f5820ca,
	0x72b2a99c, 0x6162f1ca, 0x9f4288a2, 0xa888ed1e, 0x02208839, 0xea56c569, 0xace682bc, 0x95096878,
	0xf33986ce, 0xbc3ab34e, 0x852fb06b, 0x4d809b7b, 0x3475e9c8, 0xe947baae, 0x18535080, 0x205c85fa,
	0x5792f851, 0xe029ec48, 0xd4403f27, 0x587471da, 0x3bd97278, 0x91f1a328, 0x65ea5d8a, 0xc0cfbf0f,
	0x135abf90, 0x62843a32, 0xdf6a7aa1, 0x79dc7616, 0xd091c454, 0x355e2d4a, 0xa54e04a6, 0x25719823,
	0x41bfa322, 0x94ec342e, 0xbcd06558, 0x52775497, 0xdcd0c726, 0x8c8f3975, 0xbc31513b, 0xb9b3acec,
	0x33bbeff5, 0xa3432872, 0xd8dd265e, 0xc1d9f64e, 0xe016c95d, 0x840b9c5e, 0xecdf0d3b, 0x9335eac7,
	0xe319c342, 0xaf8a83ca, 0xc2f11b65, 0x8d40c919, 0x3b91cd82, 0x70aad694, 0x341ff4ee, 0xb3fdc758,
	0x86e8e96c, 0x239e0308, 0x7daaf786, 0x6d9c3e2e, 0x028237e0, 0x652b0a79, 0x0f203491, 0xccb40e73,
	0xb1954681, 0x7ab03780, 0x9cd9ef50, 0x43e720b7, 0xe7de746d, 0xade36a14, 0xbb4f7503, 0x21ef55aa,
	0x45a0e8e3, 0x1156ea33, 0x1091d26b, 0xc8b9a8d0, 0x722df639, 0x92977f76, 0x8cb11fe6, 0x0aaeedc7,
	0xb1096093, 0xe45ea74f, 0xc2b54ff5, 0x190e549f, 0x222192cb, 0x695f9d7e, 0x926d466a, 0x0dc294aa,
	0x7e16a1b5, 0xa4bd2267, 0x19ee878a, 0xc5b8c83a, 0x001b6546, 0x6fbf7edf, 0xb3708024, 0x8e98402d,
	0x86af015f, 0x38dfd5b8, 0xc50808ef, 0x29a71185, 0x85f233d5, 0x9dea5939, 0xce3cf02c, 0x45d68b76,
	0x745a85b0, 0x6fe38075, 0x52e01b99, 0xc4693697, 0x2cd0bf67, 0x3edecb8f, 0xeb76f624, 0xe3eb94f4,
	0xf587d9e5, 0x813543ea, 0x93dfaced, 0x018c23ad, 0xb6781fd7, 0xbd564fc3, 0x962da3f0, 0x389ab6b1,
	0x5866fd23, 0x1f8b3979, 0xc10386bc, 0x4ef64f11, 0xb4572417, 0xa2f65667, 0xd68b6523, 0xa5512c00,
	0x2d5f663e, 0x3bf700ff, 0x09ef3396, 0x451bcb0d, 0xfee39ccd, 0xf606c443, 0xb46ffa45, 0x85bcfa16,
	0x7fc6efa9, 0x701ec2fe, 0xde98a301, 0xd9980668, 0xc5d004ae, 0xd03dbbb3, 0xb1d795c2, 0x2ab6203e,
	0xfa237b58, 0xbf425321, 0x000e019a, 0x2547dbbf, 0xfb97b8ed, 0x08b09edd, 0x42cd7b68, 0x32198a7f,
	0x87a2a72d, 0xe0a6a1aa, 0x2866775c, 0xc66e7ac5, 0x9edc41d3, 0x983a6ec5, 0xd3e9793a, 0xa53ad299,
	0x38d774ce, 0x75ce7fbd, 0xb353f86e, 0xc37bc25e, 0x5f2b5532, 0x1975cde4, 0xc8294de5, 0x47fbc7c1,
	0xb76b2789, 0xea88c920, 0x6c2145ba, 0x4c2211f2, 0x16a0e6de, 0xa2915469, 0xea2b14e2, 0x35202f43,
	0xaa9c69de, 0xbd00bd0e, 0xbea9e3f0, 0xbfb0bb47, 0x74347ce4, 0x1bc15e8f, 0xd9f70c10, 0x968f1e31,
	0xc55fa605, 0x47af7015, 0xe772ade0, 0x28a5c782, 0x0955a18f, 0x1054e43d, 0x5d7702b1, 0xd54a0e22,
	0x82f0f8be, 0x787b2bce, 0x243ce7b8, 0xaa160942, 0x5e7477e9, 0x45cd0df6, 0x5bdca3d7, 0x6b992304,
	0x4a8d3515, 0xfe356a57, 0xb011fc33, 0x0e2624c6, 0x0ba4ae36, 0x029fecca, 0x01ac36cc, 0x661010ac,
	0x7a8a378a, 0xf499b342, 0x973256e2, 0x1229865d, 0x03411f03, 0x7d925789, 0x6399fa28, 0x1859ddb8,
	0x83becb6d, 0x3b8322f5, 0xd0d5f97a, 0xc0fcca51, 0x6be07c35, 0x6072bdea, 0xc09b95e6, 0xa40826f8,
	0x1fb79832, 0xc401e83d, 0xcff57a0c, 0x834ee1c2, 0x59ab9f78, 0xad6e094a, 0xad2218bb, 0x3bbf864c,
	0xbb62b997, 0x23eed3ff, 0xd033de59, 0x031f0ec4, 0xbcfabf99, 0xac63ae10, 0xe4dc4aec, 0x5b98d623,
	0x0dad0246, 0xb5884cbb, 0xa39db5c7, 0x3c243f66, 0x5e69dbfb, 0x3384971d, 0x9145a99e, 0x87570b15,
	0xd6821205, 0x34fa05cf, 0xff4ac046, 0x8a98f678, 0x72add320, 0x910a51a5, 0xe78b8b93, 0xb28cd243,
	0x6a752e76, 0x16b4e6a9, 0x60b5e403, 0xc5c51f70, 0xbee86c57, 0x75a122c3, 0x3b7e773d, 0xfd8ab8ec,
	0x72839672, 0x6a713aa4, 0x18fd1c1d, 0x2ae1e7db, 0xa77453d7, 0x01e7e6c4, 0x31b08d49, 0x636c7119,
	0x736028e8, 0x75a31941, 0xcf080b2c, 0x4a92a8fb, 0x84f6b87a, 0x4f97e0dc, 0x8a7b11d2, 0x1ce7f369,
	0x056f3a69, 0x40393f83, 0xffc98a61, 0x80daf387, 0xc6a757b1, 0xa95790e2, 0x1c76cf02, 0xa1450bba,
	0x3a3150e5, 0x378e9844, 0x7c47420d, 0x617d2066, 0x8cbd025e, 0x252260a0, 0xd7ded568, 0x8e5400d7 };

unsigned int AdEncryptPassword(const char* username, const char* password) {
	unsigned int userlength;
	unsigned int passlength;
	unsigned int a = 0;
	int i;

	for (i = 0; username[i] != 0; i++) {
		a += AdRandomNumbers[(i + username[i]) & 0x7ff];
	}
	userlength = i;

	for (i = 0; password[i] != 0; i++) {
		a += AdRandomNumbers[(i + password[i] + userlength) & 0x7ff];
	}
	passlength = i;

	return AdRandomNumbers[(userlength + passlength) & 0x7ff] + a;
}

static void init(struct fmt_main *self)
{
#ifdef _OPENMP
	int omp_t = omp_get_max_threads();

	self->params.min_keys_per_crypt *= omp_t;
	omp_t *= OMP_SCALE;
	self->params.max_keys_per_crypt *= omp_t;
#endif
	saved_key = mem_calloc_align(sizeof(*saved_key),
		self->params.max_keys_per_crypt, MEM_ALIGN_WORD);
	crypt_key = mem_calloc_align(sizeof(*crypt_key),
		self->params.max_keys_per_crypt, MEM_ALIGN_WORD);
}

static void done(void)
{
	MEM_FREE(crypt_key);
	MEM_FREE(saved_key);
}

static int valid(char *ciphertext, struct fmt_main *self)
{
	char *p, *q, *tmpstr;
	int extra;

	if (strncmp(ciphertext, FORMAT_TAG, TAG_LENGTH))
		return 0;

	tmpstr = strdup(ciphertext);
	q = p = &tmpstr[TAG_LENGTH];
	p[-1] = 0;
	p = strrchr(p, '$');
	if (!p)
		goto Err;

	p += 1;

	if (hexlenl(p, &extra)  != BINARY_SIZE * 2 || extra)
		goto Err;

	if ((p - q) >= SALT_SIZE || p <= q)
		goto Err;

	MEM_FREE(tmpstr);
	return 1;
Err:;
	MEM_FREE(tmpstr);
	return 0;
}

static void *get_salt(char *ciphertext)
{
	static char salt[SALT_SIZE + 1];
	char *p, *q;

	memset(salt, 0, SALT_SIZE);
	p = ciphertext + TAG_LENGTH;
	q = strrchr(p, '$');
	memcpy(salt, p, q - p);

	return salt;
}

static void* get_binary(char *ciphertext)
{
	char *p;
	unsigned int* out = mem_alloc_tiny(BINARY_SIZE, MEM_ALIGN_WORD);

	p = strrchr(ciphertext, '$') + 1;

	*out = (unsigned int)strtoul(p, NULL, 16);
	return out;
}

static int salt_hash(void *salt)
{
    unsigned char *s = salt;
    unsigned int hash = 5381;
    unsigned int len = SALT_SIZE;

    while (len-- && *s)
        hash = ((hash << 5) + hash) ^ *s++;

    return hash & (SALT_HASH_SIZE - 1);
}

static void set_salt(void *salt)
{
	memcpy(saved_salt, salt, SALT_SIZE);
}

static void cq_set_key(char *key, int index)
{
	strncpy(saved_key[index], key, sizeof(saved_key[0]));
}

static char *get_key(int index)
{
	return saved_key[index];
}

static int crypt_all(int *pcount, struct db_salt *salt)
{
	const int count = *pcount;
	int index = 0;
#ifdef _OPENMP
#pragma omp parallel for
#endif
#if defined(_OPENMP) || MAX_KEYS_PER_CRYPT > 1
	for (index = 0; index < count; index++)
#endif
	{
		*crypt_key[index] = AdEncryptPassword(saved_salt, saved_key[index]);
	}

	return count;
}

static int cmp_all(void *binary, int count)
{
	int i = 0;

#if defined(_OPENMP) || MAX_KEYS_PER_CRYPT > 1
	for (i = 0; i < count; ++i)
#endif
	{
		if ((*(unsigned int*)binary) == *(unsigned int*)crypt_key[i])
			return 1;
	}

	return 0;
}

static int cmp_one(void *binary, int index)
{
	if ((*(unsigned int*) binary) == *(unsigned int*) crypt_key[index])
		return 1;

	return 0;
}

static int cmp_exact(char *source, int index)
{
	return 1;
}

#define COMMON_GET_HASH_VAR crypt_key
#include "common-get-hash.h"

struct fmt_main fmt_cq = {
	{
		FORMAT_LABEL,
		FORMAT_NAME,
		ALGORITHM_NAME,
		BENCHMARK_COMMENT,
		BENCHMARK_LENGTH,
		0,
		PLAINTEXT_LENGTH,
		BINARY_SIZE,
		BINARY_ALIGN,
		SALT_SIZE,
		SALT_ALIGN,
		MIN_KEYS_PER_CRYPT,
		MAX_KEYS_PER_CRYPT,
		FMT_CASE | FMT_8_BIT | FMT_OMP | FMT_OMP_BAD | FMT_NOT_EXACT,
		{ NULL },
		{ FORMAT_TAG },
		cq_tests
	}, {
		init,
		done,
		fmt_default_reset,
		fmt_default_prepare,
		valid,
		fmt_default_split,
		get_binary,
		get_salt,
		{ NULL },
		fmt_default_source,
		{
			fmt_default_binary_hash_0,
			fmt_default_binary_hash_1,
			fmt_default_binary_hash_2,
			fmt_default_binary_hash_3,
			fmt_default_binary_hash_4,
			fmt_default_binary_hash_5,
			fmt_default_binary_hash_6
		},
		salt_hash,
		NULL,
		set_salt,
		cq_set_key,
		get_key,
		fmt_default_clear_keys,
		crypt_all,
		{
#define COMMON_GET_HASH_LINK
#include "common-get-hash.h"
		},
		cmp_all,
		cmp_one,
		cmp_exact
	}
};

#endif
