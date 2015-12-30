#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "crc.h"

#define BAIL_IF(cond) ((cond) ? exit(EXIT_FAILURE) : 1)

/*
    A raw image is defined by the offset and length which tell you the position of the file
    in memory, as well as the checksum (CRC-32 over entire contents) and version number
*/
#pragma pack(1)
typedef struct {
    uint32_t offset;
    uint32_t len;
    uint32_t checksum;
    uint32_t version;
} arm_image_t;

// Enumeration that contains the values found in Withings' firmware updates.
typedef enum {
    FIRMWARE = 1,
    BOOTLOADER = 4
} IMG_IDENTIFIER;

#pragma pack(1)
typedef struct {
    uint16_t identifier;
    uint16_t type_size; // always sizeof(arm_image_t)
    arm_image_t img;
} image_t;

/*
    The header of a firmware update .bin captured directly from Withings' servers
    follows this structure:
    Following the table version, which should always be 1 is the table length, which
    does not include the checksum. This length is used to calculate the checksum of the
    table and is compared with the last field, the actual table checksum
*/
#pragma pack(1)
typedef struct {
    uint16_t table_ver;
    uint16_t table_len;
    image_t  images[2];
    uint32_t table_checksum;
} activite_img_t;

const char * const usage = "Usage: %s filename [--fixup]\n\
\tfilename needs to be a Withings Activité firmware image, either modified or unmodified.\n\
\t--fixup can be used to fix CRC checksums, allowing you to install the firmware on the actual device\n";

/*
    Simple convenience function that returns the size of a
    file in bytes
*/
int file_get_size(int fd)
{
    struct stat buf;
    fstat(fd, &buf);
    return buf.st_size;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf(usage, argv[0]);
        return -1;
    }

    bool fixup = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp("--fixup", argv[i]) == 0)
            fixup = true;
    }

    int fd = open(argv[1], O_RDWR);
    BAIL_IF(fd == -1);
    void * addr = mmap(NULL, file_get_size(fd), 
                       PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED,
                       fd, 0);
    BAIL_IF(addr == MAP_FAILED);

    activite_img_t * img = addr;

    // In all images encountered so far, there were exactly two separate images
    for (int i = 0; i < 2; ++i) {
        uint32_t img_crc = crc32(((uint8_t *)addr) + img->images[i].img.offset,
                             img->images[i].img.len);

        if (img->images[i].img.checksum == img_crc) {
            switch((IMG_IDENTIFIER)img->images[i].identifier) {
                case FIRMWARE:
                    puts("Verified firmware checksum");
                    break;
                case BOOTLOADER:
                    puts("Verified bootloader checksum");
                    break;
            }
        } else {
            if (fixup) {
                img->images[i].img.checksum = img_crc;
                puts("Recalculated checksum");
            } else {
                puts("One checksum invalid. Did not fix");
            }
        }    
    }

    const uint32_t header_crc = crc32(img, sizeof(activite_img_t) - sizeof(uint32_t));
    if (img->table_checksum == header_crc) {
        puts("Header checksum verified");
    } else {
        if (fixup) {
            img->table_checksum = header_crc;
            puts("Recalculated header checksum");
        } else {
            puts("Header checksum invalid. Did not fix.");
        }
    }

    munmap(addr, file_get_size(fd));
    close(fd);

    return EXIT_SUCCESS;
}
