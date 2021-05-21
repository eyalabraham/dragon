/********************************************************************
 * sdfat32.c
 *
 *  SPI SD card reader that implements a minimal read-only driver
 *  for FAT32 file system, and an interface to read CAS and ROM
 *  files into emulator memory.
 *
 *  This is a minimal implementation, FAT32, read only, v1.0 SD card
 *  compliant driver. The goal is functionality not performance.
 *
 *  May 9, 2021
 *
 *******************************************************************/

#include    <string.h>

#include    "rpi.h"
#include    "sdfat32.h"

/* -----------------------------------------
   Module definition
----------------------------------------- */

#define     FAT32_SEC_SIZE            512       // Bytes
#define     FAT32_MAX_SEC_PER_CLUS  16          // *** 1, 2, 4, 8, 16, 32, 64, 128
#define     FAT32_END_OF_CHAIN      0x0ffffff8

#define     FILE_ATTR_READ_ONLY     0b00000001
#define     FILE_ATTR_HIDDEN        0b00000010
#define     FILE_ATTR_SYSTEM        0b00000100
#define     FILE_ATTR_VOL_LABEL     0b00001000
#define     FILE_ATTR_DIRECTORY     0b00010000
#define     FILE_ATTR_ARCHIVE       0b00100000
#define     FILE_ATTR_LONG_NAME     0b00001111

#define     FILE_LFN_END            0x40

/* -----------------------------------------
   Module types
----------------------------------------- */
/* Partition table structure
 */
typedef struct
    {
        uint8_t   status;
        uint8_t   first_head;
        uint8_t   first_sector;
        uint8_t   first_cylinder;
        uint8_t   type;
        uint8_t   last_head;
        uint8_t   last_sector;
        uint8_t   last_cylinder;
        uint32_t  first_lba;
        uint32_t  num_sectors;
    } __attribute__ ((packed)) partition_t;

/* boot sector and BPB
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BPB
 */
typedef struct
    {
        uint16_t  bytes_per_sector;         // DOS 2.0 BPB
        uint8_t   sectors_per_cluster;
        uint16_t  reserved_sectors;
        uint8_t   fat_count;
        uint16_t  root_directory_entries;
        uint16_t  total_sectors;
        uint8_t   media_descriptor;
        uint16_t  sectors_per_fat;
        uint16_t  sectors_per_track;        // DOS 3.31 BPB
        uint16_t  heads;
        uint32_t  hidden_sectors;
        uint32_t  total_logical_sectors;
        uint32_t  logical_sectors_per_fat;  // FAT32 extensions
        uint16_t  drive_desc;
        uint16_t  version;
        uint32_t  cluster_number_root_dir;
        // ... there is more.
    } __attribute__ ((packed)) bpb_t;

typedef struct
{
    char        short_dos_name[8];
    char        short_dos_ext[3];
    uint8_t     attribute;
    uint8_t     user_attribute;
    uint8_t     delete_attribute;
    uint16_t    create_time;
    uint16_t    create_date;
    uint16_t    last_access_date;
    uint16_t    fat32_high_cluster;
    uint16_t    last_mod_time;
    uint16_t    last_mod_date;
    uint16_t    fat32_low_cluster;
    uint32_t    file_size_bytes;
} __attribute__ ((packed)) dir_record_t;

typedef struct
    {
        uint32_t    first_lba;
        uint32_t    fat_begin_lba;
        uint32_t    cluster_begin_lba;
        uint32_t    sectors_per_cluster;
        uint32_t    root_dir_first_cluster;
    } fat_param_t;

/* -----------------------------------------
   Module functions
----------------------------------------- */
static fat_error_t fat32_read_cluster(uint8_t *buffer, int buffer_len, uint32_t cluster_num);
static uint32_t    fat32_get_next_cluster_num(uint32_t cluster_num);

static int         dir_get_sfn(dir_record_t *dir_record, char *name, int name_length);
static int         dir_get_lfn(dir_record_t *dir_record, char *name, int name_length);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static  fat_param_t fat32_parameters;

/* -------------------------------------------------------------
 * fat32_init()
 *
 *  Initialize FAT32 module
 *
 *  Param:  None
 *  Return: Driver error
 */
fat_error_t fat32_init(void)
{
    uint8_t     data_block_buffer[FAT32_SEC_SIZE]; // One sector
    uint8_t    *sector;
    partition_t partitions[4];
    bpb_t       bpb;

    /* Read data block
     */
    if ( rpi_sd_read_block(0, data_block_buffer, FAT32_SEC_SIZE) != SD_OK )
        return FAT_SD_FAIL;

    /* Analyze boot sector and partition table
     */
    sector = data_block_buffer;
    memcpy(partitions, &sector[446], sizeof(partitions));

    if ( sector[510] != 0x55 || sector[511] != 0xaa )
    {
        return FAT_BAD_SECTOR_SIG;
    }

    /* Read file system
     */

    if ( partitions[0].type != 0x0c )
    {
        return FAT_BAD_PARTITION_TYPE;
    }

    fat32_parameters.first_lba = partitions[0].first_lba;

    if ( rpi_sd_read_block(fat32_parameters.first_lba, data_block_buffer, FAT32_SEC_SIZE) != SD_OK )
        return FAT_SD_FAIL;

    /* Analyze BPB of first partition
     */
    sector = data_block_buffer;
    memcpy(&bpb, &sector[11], sizeof(bpb_t));

    if ( sector[510] != 0x55 || sector[511] != 0xaa )
    {
        return FAT_BAD_SECTOR_SIG;
    }

    if ( bpb.sectors_per_cluster > FAT32_MAX_SEC_PER_CLUS )
    {
        return FAT_BAD_SECTOR_PER_CLUS;
    }

    /* Required FAT32 parsing parameters
     */
    fat32_parameters.fat_begin_lba = fat32_parameters.first_lba + bpb.reserved_sectors;
    fat32_parameters.cluster_begin_lba = fat32_parameters.fat_begin_lba + (bpb.fat_count * bpb.logical_sectors_per_fat);
    fat32_parameters.sectors_per_cluster = bpb.sectors_per_cluster;
    fat32_parameters.root_dir_first_cluster = bpb.cluster_number_root_dir;

    return FAT_OK;
}

/* -------------------------------------------------------------
 * fat32_parse_dir()
 *
 *  Parse directory listing from input cluster into
 *  directory caching array
 *
 *  Param:  Directory's first cluster number, pointer to directory caching array and its length
 *  Return: Count of parsed items, '-1'=error
 */
int fat32_parse_dir(uint32_t start_cluster, dir_entry_t *directory_list, int dir_list_length)
{
    static uint8_t  cluster_buffer[FAT32_MAX_SEC_PER_CLUS*FAT32_SEC_SIZE];

    dir_record_t   *dir_record;
    uint32_t        dir_next_cluster;
    int             max_dir_records_per_cluster;
    int             cached_dir_records;
    int             long_filename_flag;
    int             i;

    /* Parse root directory cluster
     */
    max_dir_records_per_cluster = fat32_parameters.sectors_per_cluster * FAT32_SEC_SIZE / sizeof(dir_record_t);
    dir_next_cluster = start_cluster;
    long_filename_flag = 0;
    cached_dir_records = -1;

    for (;;)
    {
        if ( fat32_read_cluster(cluster_buffer, sizeof(cluster_buffer), dir_next_cluster) != FAT_OK )
            break;

        for ( i = 0, cached_dir_records = 0; (i < max_dir_records_per_cluster && cached_dir_records < dir_list_length); i++ )
        {
            dir_record = (dir_record_t*) (cluster_buffer + (i * sizeof(dir_record_t)));
            directory_list[cached_dir_records].sfn[0] = 0;
            directory_list[cached_dir_records].lfn[0] = 0;

            // Done
            if ( dir_record->short_dos_name[0] == 0 )
                break;

            // Skip deleted file
            if ( dir_record->short_dos_name[0] == 0xe5 )
                continue;

            // Skip volume label
            if ( dir_record->attribute == FILE_ATTR_VOL_LABEL )
            {
                continue;
            }

            // Flag long file name
            if ( (dir_record->attribute & FILE_ATTR_LONG_NAME) == FILE_ATTR_LONG_NAME )
            {
                long_filename_flag = 1;
                continue;
            }

            directory_list[cached_dir_records].is_directory = ((dir_record->attribute & FILE_ATTR_DIRECTORY) ? 1 : 0);
            dir_get_sfn(dir_record, directory_list[cached_dir_records].sfn, FAT32_DOS_FILE_NAME);

            if ( dir_record->short_dos_name[0] == '.' )
            {
                directory_list[cached_dir_records].lfn[0] = '.';
                directory_list[cached_dir_records].lfn[1] = 0;
                if ( dir_record->short_dos_name[1] == '.' )
                {
                    directory_list[cached_dir_records].lfn[1] = '.';
                    directory_list[cached_dir_records].lfn[2] = 0;
                }
            }

            if ( long_filename_flag )
            {
                dir_get_lfn(dir_record, directory_list[cached_dir_records].lfn, FAT32_LONG_FILE_NAME);
                long_filename_flag = 0;
            }
            else
            {
                strncpy(directory_list[cached_dir_records].lfn,
                       directory_list[cached_dir_records].sfn, FAT32_DOS_FILE_NAME);
            }

            directory_list[cached_dir_records].cluster_chain_head = ((uint32_t)dir_record->fat32_high_cluster << 16) + dir_record->fat32_low_cluster;
            /* This adjustment seems necessary because I noticed that
             * the first sub-directory level has a '..' file with a
             * '0' cluster number.
             */
            if ( directory_list[cached_dir_records].cluster_chain_head == 0 )
                directory_list[cached_dir_records].cluster_chain_head = 2;

            directory_list[cached_dir_records].file_size = dir_record->file_size_bytes;

            cached_dir_records++;

        }

        dir_next_cluster = fat32_get_next_cluster_num(dir_next_cluster);
        if ( dir_next_cluster >= FAT32_END_OF_CHAIN )
            break;
    }

    return cached_dir_records;
}

/* -------------------------------------------------------------
 * fat32_get_file()
 *
 *  Read the contents of a file return it in caller
 *  buffer.
 *
 *  Param:  Files's first cluster number, file size in bytes
 *  Return: Bytes read, -1=error
 */
int fat32_get_file(uint32_t file_start_cluster, uint32_t file_size, uint8_t *buffer, int buffer_length)
{
    static uint8_t  cluster_buffer[FAT32_MAX_SEC_PER_CLUS*FAT32_SEC_SIZE];

    uint32_t    file_next_cluster;
    int         file_byte_count, i;

    /* Read file's clusters and output printable
     * characters to terminal
     */
    file_next_cluster = file_start_cluster;
    file_byte_count = 0;

    for (;;)
    {
        if ( fat32_read_cluster(cluster_buffer, sizeof(cluster_buffer), file_next_cluster) != FAT_OK )
        {
            file_byte_count = -1;
            break;
        }

        for ( i = 0; i < (fat32_parameters.sectors_per_cluster * FAT32_SEC_SIZE) &&
                     file_byte_count < file_size &&
                     file_byte_count < buffer_length; i++, file_byte_count++ )
        {
            /* Fill user buffer
             */
            buffer[file_byte_count] = cluster_buffer[i];
        }

        file_next_cluster = fat32_get_next_cluster_num(file_next_cluster);
        if ( file_next_cluster >= FAT32_END_OF_CHAIN )
            break;
    }

    return file_byte_count;
}

/* -------------------------------------------------------------
 * fat32_read_cluster()
 *
 *  Read a cluster from SD to buffer.
 *  Buffer must be at least sectors-per-cluster long
 *
 *  Param:  Buffer and its length, the cluster number to read and FAT's parameters
 *  Return: Driver error
 */
static fat_error_t fat32_read_cluster(uint8_t *buffer, int buffer_len, uint32_t cluster_num)
{
    int         i;
    int         buffer_index;
    uint32_t    base_cluster_lba;

    base_cluster_lba = fat32_parameters.cluster_begin_lba + (cluster_num - 2) * fat32_parameters.sectors_per_cluster;

    for ( i = 0, buffer_index = 0; i < fat32_parameters.sectors_per_cluster; i++, buffer_index += FAT32_SEC_SIZE )
    {
        if ( buffer_index > buffer_len )
            break;

        if ( rpi_sd_read_block(base_cluster_lba + i, buffer + buffer_index, FAT32_SEC_SIZE) != SD_OK )
            return FAT_SD_FAIL;
    }

    return FAT_OK;
}

/* -------------------------------------------------------------
 * fat32_get_next_cluster_num()
 *
 *  Given a cluster number, scan the FAT32 table and find
 *  the next cluster number in the chain.
 *
 *  Param:  Cluster number to start scan
 *  Return: Next cluster number, or 0x0ffffff8 for end
 */
static uint32_t fat32_get_next_cluster_num(uint32_t cluster_num)
{
    uint32_t    fat32_sector_lba;
    uint32_t    fat32_sector_offset;
    uint8_t     data_block_buffer[FAT32_SEC_SIZE]; // One sector

    fat32_sector_lba = fat32_parameters.fat_begin_lba + cluster_num / 128;
    fat32_sector_offset = (cluster_num % 128) * sizeof(uint32_t);

    if ( rpi_sd_read_block(fat32_sector_lba, data_block_buffer, FAT32_SEC_SIZE) != SD_OK )
        return FAT32_END_OF_CHAIN;

    return *((uint32_t*)&data_block_buffer[fat32_sector_offset]);
}

/* -------------------------------------------------------------
 * dir_get_sfn()
 *
 *  Extract short (DOS 8.3) file name from directory record
 *
 *  Param:  Directory record, buffer for file name and its length
 *  Return: Characters extracted
 */
static int dir_get_sfn(dir_record_t *dir_record, char *name, int name_length)
{
    char   *record;
    int     i, c;

    if ( name_length < FAT32_DOS_FILE_NAME )
        return 0;

    record = (char *) dir_record;

    for ( i = 0, c = 0; i < (FAT32_DOS_FILE_NAME-2); i++ )
    {
        if ( record[i] == 0x20 )
            continue;

        if ( i == 8)
        {
            name[i] = '.';
        }

        name[i] = record[i];
        c++;
    }

    name[c] = 0;

    return c;
}

/* -------------------------------------------------------------
 * dir_get_lfn()
 *
 *  Extract long file name from directory record
 *
 *  Param:  Directory record, buffer for long file name and its length
 *  Return: Characters extracted
 */
static int dir_get_lfn(dir_record_t *dir_record, char *name, int name_length)
{
    static int  char_locations[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

    char       *record;
    int         i, j;

    if ( name_length < (FAT32_LONG_FILE_NAME-1) )
        return 0;

    j = 0;

    do
    {
        dir_record--;
        record = (char *) dir_record;

        for ( i = 0; i < 13; i++, j++ )
        {
            if ( record[char_locations[i]] == 0xff )
                break;

            name[j] = record[char_locations[i]];
        }
    }
    while ( (dir_record->short_dos_name[0] & 0x40) == 0 );

    name[j] = 0;

    return j;
}
