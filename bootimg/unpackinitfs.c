#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <errno.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#define mkdir(p, m) { wchar_t wp[256]; MultiByteToWideChar(CP_ACP, 0, p, -1, wp, 256); CreateDirectory(wp, NULL); }
#endif

#ifndef uint64_t
typedef uint64_t    __u64;
#endif

#ifndef uint32_t
typedef uint32_t    __u32;
#endif

#ifndef S_IFLNK
#define S_IFLNK        0xA000          /* link */
#else
#include <utime.h>
#endif

typedef struct ENTRY {
	char	c_magic[6];		// The string "070701" or "070702"
	char	c_ino[8];		// 8 bytes		 File inode number
	char	c_mode[8];		// 8 bytes		 File mode and permissions
	char	c_uid[8];		// 8 bytes		 File uid
	char	c_gid[8];		// 8 bytes		 File gid
	char	c_nlink[8];		// 8 bytes		 Number of links
	char	c_mtime[8];		// 8 bytes		 Modification time
	char	c_filesize[8];	// 8 bytes		 Size of data field
	char	c_maj[8];		// 8 bytes		 Major part of file device number
	char	c_min[8];		// 8 bytes		 Minor part of file device number
	char	c_rmaj[8];		// 8 bytes		 Major part of device node reference
	char	c_rmin[8];		// 8 bytes		 Minor part of device node reference
	char	c_namesize[8];	// 8 bytes		 Length of filename, including final \0
	char	c_chksum[8];	// 8 bytes		 Checksum of data field if c_magic is 070702; otherwise zero
} ENTRY;

#define MIN_FAT_TIME (60*60*24)*((365*10)+2)

static const char magic1[] = "070701";
static const char magic2[] = "070702";

int main_unpackinitfs(int argc, char** argv)
{
	FILE			*f, *fout;
	ENTRY			e;
	unsigned char	buffer[4096];
	char			fname[256];
	char			path[256];
	uint32_t		fname_len;
	uint32_t		file_len;
	uint32_t		file_mode;
	size_t			toread;
	size_t			fpos;
	int				i;
	char			*outdir;
	char			value[9];

	uint32_t		time;
	struct utimbuf	times;

	f = NULL;
	outdir = NULL;
	value[8] = 0;

	for (i=1; i<argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'd') {
				i++;
				outdir = argv[i];
				continue;
			}
		} else {
			if (f == NULL) {
				f = fopen(argv[1], "rb");
				if (f == NULL) {
					printf("Could not open %s\n\n", argv[1]);
					exit(1);
				}
				continue;
			}
		}

		fprintf(stderr, "Usage: %s [input_file | -d target directory kernel]\n\n", argv[0]);
		return 200;
	}

	if (f == NULL) {
		f = stdin;
#ifdef WIN32
		_setmode(_fileno(stdin), O_BINARY);
#endif
	}

	fpos = 0;
	while (fread(&e, 1, sizeof(e), f) == sizeof(e)) {
		fpos += sizeof(e);
		if (0 != memcmp(&(e.c_magic), magic1, sizeof(magic1)-1) &&
			0 != memcmp(&(e.c_magic), magic2, sizeof(magic2)-1))
			continue;

		memcpy(value, e.c_namesize, 8);
		fname_len = strtoul(value, 0, 16);
		if (fread(fname, 1, fname_len, f) != fname_len)	{
			fprintf(stderr, "Could not read filename. Input file corrupt?\n\n");
			exit(1);
		}
		fpos += fname_len;

		if (strcmp(fname, "TRAILER!!!") == 0)
			break;

		sprintf(path, "%s%s%s", outdir == NULL ? "" : outdir, outdir == NULL ? "" : "/", fname);
		memcpy(value, e.c_filesize, 8);
		file_len = strtoul(value, 0, 16);
		memcpy(value, e.c_mode, 8);
		file_mode = strtoul(value, 0, 16);

		while (fpos & 3) {
			fread(buffer, 1, 1, f);
			fpos++;
		}

		if ((file_mode & S_IFMT) == S_IFLNK) {
			if (fread(buffer, 1, file_len, f) != file_len) {
				fprintf(stderr, "Could not read link %s. Input file corrupt?\n\n", fname);
				exit(1);
			}
			fpos += file_len;

			strcat(path, "__lnk__");
			fout = fopen(path, "wb+");
			if (fout == NULL) {
				fprintf(stderr, "Could not create file %s\n\n", path);
				exit(1);
			}
			fwrite(buffer, 1, file_len, fout);
			fclose(fout);
		}
		else if ((file_mode & S_IFMT) == S_IFREG) {
			fout = fopen(path, "wb+");
			if (fout == NULL) {
				fprintf(stderr, "Could not create file %s\n\n", path);
				exit(1);
			}
			while (file_len) {
				toread = file_len;
				if (toread > sizeof(buffer))
					toread = sizeof(buffer);
				if (fread(buffer, 1, toread, f) != toread) {
					fprintf(stderr, "Could not read file %s. Input file corrupt?\n\n", fname);
					exit(1);
				}
				fpos += toread;
				fwrite(buffer, 1, toread, fout);
				file_len -= toread;
			}
			fclose(fout);
		}
		else if ((file_mode & S_IFMT) == S_IFDIR) {
			mkdir(path, file_mode & 0xfff);
		}

		/* Adjust time */
		memcpy(value, e.c_mtime, 8);
		time = strtoul(value, 0, 16);
		times.actime = time;
		times.modtime = times.actime;
		if (0 != utime(path,  &times) && time < MIN_FAT_TIME) {
			times.actime = MIN_FAT_TIME;
			times.modtime = times.actime;
			utime(path,  &times);
		}

		while (fpos & 3) {
			fread(buffer, 1, 1, f);
			fpos++;
		}
	}

	fclose(f);
	return 0;
}
