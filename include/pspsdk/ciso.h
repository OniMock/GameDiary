#ifndef PSP_CISO_H
#define PSP_CISO_H

#include <stdlib.h>
#include <stdint.h>

#define CSO_MAGIC 0x4F534943 // CISO
#define ZSO_MAGIC 0x4F53495A // ZISO
#define DAX_MAGIC 0x00584144 // DAX
#define JSO_MAGIC 0x4F53494A // JISO

#define ISO_SECTOR_SIZE 2048
#define DAX_BLOCK_SIZE 0x2000
#define DAX_COMP_BUF 0x2400

#define CISO_IDX_MAX_ENTRIES 2048 // will be adjusted according to CSO block_size

typedef struct _CISOHeader {
    uint32_t magic;  // 0
    uint32_t header_size;  // 4
    uint64_t total_bytes; // 8
    uint32_t block_size; // 16
    uint8_t ver; // 20
    uint8_t align;  // 21
    uint8_t rsv_06[2];  // 22
} CISOHeader;

typedef struct _DAXHeader{ 
    uint32_t magic;
    uint32_t uncompressed_size;
    uint32_t version; 
    uint32_t nc_areas; 
    uint32_t unused[4]; 
} DAXHeader;

typedef struct _JISOHeader {
    uint32_t magic; // [0x000] 'JISO'
    uint8_t unk_x001; // [0x004] 0x03?
    uint8_t unk_x002; // [0x005] 0x01?
    uint16_t block_size; // [0x006] Block size, usually 2048.
    // TODO: Are block_headers and method 8-bit or 16-bit?
    uint8_t block_headers; // [0x008] Block headers. (1 if present; 0 if not.)
    uint8_t unk_x009; // [0x009]
    uint8_t method; // [0x00A] Method. (See JisoAlgorithm_e.)
    uint8_t unk_x00b; // [0x00B]
    uint32_t uncompressed_size; // [0x00C] Uncompressed data size.
    uint8_t md5sum[16]; // [0x010] MD5 hash of the original image.
    uint32_t header_size; // [0x020] Header size? (0x30)
    uint8_t unknown[12]; // [0x024]
} JISOHeader;

typedef enum {
    JISO_METHOD_LZO    	= 0,
    JISO_METHOD_ZLIB    = 1,
} JisoMethod;

typedef struct {
    // this needs to be provided by user
    void* reader_arg;
    int (*read_data)(void* reader_arg, void* buf, uint32_t size, uint32_t offset);
    // either provide this or the buffers
    void* (*memalign)(unsigned int align, size_t size);
    void (*free)(void* ptr);
    // ciso data, used by reader
    void (*decompressor)(void* src, int src_len, void* dst, int dst_len, uint32_t topbit);
    uint8_t *block_buf; // align 64
    uint8_t *dec_buf; // align 64
    uint32_t *idx_cache; // align 64
    int idx_start_block; // initialize to -1
    uint32_t idx_cache_num;
    uint32_t header_size;
    uint32_t block_size;
    uint32_t uncompressed_size;
    uint32_t block_header;
    uint32_t align;
    uint32_t total_blocks;
    uint32_t com_size;
} CisoFile;

enum CisoErrors{
    CISO_BAD_ARGS = -1,
    CISO_IO_ERROR = -2,
    CISO_NOT_VALID = 0,
    CISO_OK,
};


/**
 * Before opening a ciso file, you must initialize the CisoFile structure with necessary data.
 * 
 * 1) read_data() is always needed but reader_arg depends on the user, it will be passed as first arg to read_data.
 * 2) either provive dynamic allocation functions or buffers for block_buf, dec_buf, and idx_cache
 * 
 * example 1: with dynamic allocation functions:
 * 
 *   CisoFile g_ciso_file = {
 *      .read_data = &read_raw_data,
 *      .memalign = &memalign,
 *      .free = &free,
 *   };
 * 
 * example 2: with statically allocated buffers
 * 
 * 
 *   unsigned char block_buf[DAX_COMP_BUF]; // 8K blocks
 *   unsigned char dec_buf[DAX_COMP_BUF];
 *   unsigned int idx_cache[CISO_IDX_MAX_ENTRIES];
 * 
 *   CisoFile g_ciso_file = {
 *      .read_data = &read_raw_data,
 *      .block_buf = block_buf,
 *      .dec_buf = dec_buf,
 *      .idx_cache = idx_cache,
 *      .idx_start_block = -1, // this is important when doing static buffers
 *      .idx_cache_num = CISO_IDX_MAX_ENTRIES,
 *   };
 * 
 */


int ciso_open(CisoFile* file);
int ciso_read(CisoFile* file, uint8_t* addr, uint32_t size, uint32_t offset);
int ciso_close(CisoFile* file);

#endif