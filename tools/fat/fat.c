#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>


typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t usize;
typedef ssize_t isize;


typedef struct fat12_bootsector {
	u8 boot_jump[3];
	u8 oem_identifier[8];
	u16 sector_bytes;
	u8 sectors_per_cluster;
	u16 reserved_sectors;
	u8 fat_count;
	u16 dentries;
	u16 total_sectors;
	u8 media_descriptor_table;
	u16 sectors_per_fat;
	u16 sectors_per_track;
	u16 heads;
	u32 hidden;
	u32 large;

	u8 drive_num;
	u8 _reserved;
	u8 sig;
	u32 volume_id;
	u8 vol_label[11];
	u8 system_id[8];
} __attribute__((packed)) bootsector;

typedef struct directory_entry {
	u8 name[11];
	u8 attrs;
	u8 _reserved;
	u8 create_time_tenths;
	u16 created_time;
	u16 created_date;
	u16 accessed_date;
	u16 fcluster_hi;
	u16 modified_time;
	u16 modified_date;
	u16 fcluster_lo;
	u32 size;
} __attribute__((packed)) dentry;


static bootsector sector = {};
static u8 *fat = {};
static dentry *root_dir = {};
static u32 root_dir_end = {};


int read_bootsector(FILE *d)
{
	return fread(&sector, sizeof(struct fat12_bootsector), 1, d) > 0;
}


int read_sectors(FILE *d, const u32 lba, const u32 count, void *buf_o)
{
	bool ok = true;
	ok = ok && (fseek(d, lba * sector.sector_bytes, SEEK_SET) == 0);
	ok = ok && (fread(buf_o, sector.sector_bytes, count, d) == count);
	return ok;
}


int read_fat(FILE *d)
{
	fat = malloc(sector.sectors_per_fat * sector.sector_bytes);
	return read_sectors(d,
	                    sector.reserved_sectors,
	                    sector.sectors_per_fat,
	                    fat);
}


int read_rootdir(FILE *d)
{
	const u32 lba = sector.reserved_sectors
	                + sector.sectors_per_fat
	                * sector.fat_count;

	const u32 sz = sizeof(dentry) * sector.dentries;
	u32 sectors = (sz / sector.sector_bytes);


	if (sz % sector.sector_bytes > 0) {
		sectors++;
	}

	root_dir_end = lba + sectors;
	root_dir = malloc(sectors * sector.sector_bytes);

	return read_sectors(d, lba, sectors, root_dir);
}


dentry *find_file(const char *name)
{
	for (u32 i = 0; i < sector.dentries; i++) {
		if (memcmp(name, root_dir[i].name, 11) == 0) {
			return &root_dir[i];
		}
	}

	return nullptr;
}


int read_file(const dentry *restrict entry, FILE *d, u8 *o)
{
	bool ok = true;
	u16 ccluster = entry->fcluster_lo;

	do {
		const u32 lba = root_dir_end + (ccluster - 2) * sector.sectors_per_cluster;

		ok = ok && read_sectors(d, lba, sector.sectors_per_cluster, o);
		o += sector.sectors_per_cluster * sector.sector_bytes;

		const u32 fidx = ccluster * 3 / 2;

		if (ccluster % 2 == 0) {
			ccluster = (*(u16 *)(fat + fidx)) & 0x0FFF;
		} else {
			ccluster = (*(u16 *)(fat + fidx)) >> 4;
		}

	} while (ok && ccluster < 0x0FF8);

	return ok;
}


int main(const int argc, char **argv)
{
	if (argc < 3) {
		printf("too little arguments!");
		return 1;
	}

	FILE *d = fopen(argv[1], "rb");
	if (!d) {
		fprintf(stderr, "error opening image: %s!\n", argv[1]);
		return -1;
	}

	if (!read_bootsector(d)) {
		fprintf(stderr, "bootsector read error: %s!\n", argv[1]);
		fclose(d);
		return 2;
	}

	if (!read_fat(d)) {
		fprintf(stderr, "FAT read error\n");
		goto err;
	}

	if (!read_rootdir(d)) {
		fprintf(stderr, "rootdir read error!\n");
		goto err_root;
	}

	dentry *entry = find_file(argv[2]);
	if (!entry) {
		fprintf(stderr, "file not found: %s\n", argv[2]);
		goto err_root;
	}

	u8 *b = malloc(entry->size + sector.sector_bytes);
	if (!read_file(entry, d, b)) {
		fprintf(stderr, "file read error!\n");
		goto err_file;
	}

	for (int i = 0; i < entry->size; i++) {
		if (isprint(b[i]))
			fputc(b[i], stdout);
		else
			printf("<%02x>", b[i]);
	}
	putc('\n', stdout);

	free(b);
	free(root_dir);
	free(fat);
	fclose(d);
	return 0;
err_file:
	free(b);
err_root:
	free(root_dir);
err:
	free(fat);
	fclose(d);
	return -1;
}
