/*
simple sort
Sort LF delimited file in ascending order.
2020/9/17
gcc-9 -O2 -o simplesort simplesort.c
gcc-9 -O0 -g -o simplesort simplesort.c
*/
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

typedef struct {
	unsigned char *datap;
	unsigned int len;
} RECIDX;

typedef struct {
	unsigned long data_size;
	unsigned char *datap;
	unsigned int record_num;
	RECIDX *recidxes;
	int exist_last_cr;
} RECORD_INFO;

#define LF 0x0a

void input_file(RECORD_INFO *recinfo, unsigned char *file)
{
	int fd;
	if (-1 == (fd = open(file, O_RDONLY))) {
		perror("error open input file");
		exit(1);
	}
	struct stat statBuf;
	if (stat(file, &statBuf) != 0) {
		perror("error read file size");
		exit(1);
	}
	unsigned char *mp;
	if (NULL == (mp = malloc(statBuf.st_size +2))) {
		perror("error malloc for file");
		exit(1);
	}
	ssize_t size = read(fd, mp, statBuf.st_size);
	if (size < 0) {
		perror("error read file");
		exit(1);
	}
	close(fd);
	if (size != statBuf.st_size) {
		printf("read size error (stat=%ld read=%ld)\n", statBuf.st_size, size);
		exit(1);
	}
	if (size > 0) {
		if ((mp[size -1]) != LF) {
			printf("WARN last char is not LF.\n");
			mp[size] = LF;
			size++;
		}
	}

	recinfo->data_size = size;
	recinfo->datap = mp;
}

void create_idx(RECORD_INFO *recinfo)
{
	recinfo->record_num = 0;
	if (recinfo->data_size == 0) return;

	int num = 0;
	unsigned long pos = 0;
	unsigned char *p = recinfo->datap;
	for (pos=0; pos < recinfo->data_size; pos++, p++) {
		if (*p == LF) {
			num++;
		}
	}
	if (*(p-1) != LF) {
		printf("WARN last char is not LF.\n");
		num++;
	}
	recinfo->record_num = num;

	if (NULL == (recinfo->recidxes = malloc(sizeof(RECIDX) *(num +1)))) {
		perror("error malloc for record index");
		exit(1);
	}

	recinfo->recidxes[0].datap = recinfo->datap;
	unsigned long ppos = 0;
	unsigned int idx = 0;
	p = recinfo->datap;

	for (pos=0; pos<recinfo->data_size; pos++, p++) {
		if (*p == LF) {
			recinfo->recidxes[idx].len = pos - ppos +1;
			idx++;
			recinfo->recidxes[idx].datap = p+1;
			ppos = pos +1;
		}
	}
	if (*(p-1) != LF) {
		recinfo->recidxes[idx].len = (pos -1) - ppos +1;
	}

	/*
	for(unsigned int i=0; i<recinfo->record_num; i++) {
		RECIDX *p = &(recinfo->recidxes[i]);
		printf("i=%d len=%d p=%lx\n", i, p->len, (long int)p->datap);
	}
	*/
}

int compare(const void* a, const void* b)
{
	//printf("-----\n");
	RECIDX *ra = (RECIDX *)a;
	RECIDX *rb = (RECIDX *)b;
	for(int i=0; i<ra->len-1 && i<rb->len-1; i++) {
		//printf("[%c] [%c]\n", ra->datap[i] ,rb->datap[i]);
		if (ra->datap[i] < rb->datap[i]) return -1;
		if (ra->datap[i] > rb->datap[i]) return 1;
	}
	if (ra->len < rb->len) return -1;
	if (ra->len > rb->len) return 1;
	return 0;
}

void output_data(RECORD_INFO *recinfo, unsigned char *file)
{
	FILE *fpo;
	if (NULL == (fpo = fopen(file, "w"))) {
		perror("error open output file");
		exit(1);
	}
	for(unsigned int i=0; i<recinfo->record_num; i++) {
		RECIDX *p = &(recinfo->recidxes[i]);
		if (fwrite(p->datap, p->len, 1, fpo) < 0) {
			perror("error write to file");
			exit(1);
		}
	}
	fclose(fpo);
}

void main(int argc, unsigned char **args)
{
	if (argc < 3) {
		printf("simplesort v1.0\n");
		printf("Usage: simplesort {input-file} {output-file}\n");
		exit(1);
	}
	unsigned char *infile = args[1];
	unsigned char *outfile = args[2];
	RECORD_INFO recinfo;

	input_file(&recinfo, infile);
	create_idx(&recinfo);
	qsort(recinfo.recidxes, recinfo.record_num, sizeof(RECIDX), compare);
	/*
	for(unsigned int i=0; i<recinfo.record_num; i++) {
		RECIDX *p = &(recinfo.recidxes[i]);
		printf("i=%d len=%d p=%lx\n", i, p->len, (long int)p->datap);
	}
	*/
	output_data(&recinfo, outfile);
	exit(0);
}
