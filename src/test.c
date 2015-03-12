#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <cx_math/math.h>
#include <cx_thread/cx_thread.h>

#include <netdb.h>
#include <sys/socket.h>

#include <unistd.h>
#include <sys/types.h>
#include <memory.h>

static void 
get_next(u_char *p, int next[], int p_len) 
{
	next[0] = -1;
	int		 k = -1;
	int		 j = 0;
	while (j < p_len - 1) {
		if (k == -1 || p[j] == p[k]) {
			k++;
			j++;
			next[j] = k;
		} else {	
			k = next[k];
		}
	}
}

static int 
kmp_search(u_char *s, int s_len, u_char *p, int p_len)
{
	if (s_len < 1) return -1;
	if (p_len < 1) return -1;
	if (s_len < p_len) return -1;
	int		 		i = 0;
	int		 		j = 0;
	int		 		next[p_len + 1];
	memset(next, 0, p_len + 1);
	get_next(p, next, p_len);

	while (i < s_len && j < p_len) {
		// if j == -1 or S[i] == P[j], then i++, j++
		if (j == -1 || s[i] == p[j]) {
			i++;
			j++;
		} else {
			// if j != -1 and S[i] != P[j], then i stay, j = next[j]
			j = next[j];
		}	
	}
	if (j == p_len)
		return i - j;
	else 
		return -1;
}

int
ngx_http_prefetch_gzip_decompress(Byte *zdata, uLong nzdata, Byte *data, uLong *ndata)
{
    //if (uncompress(data, ndata, zdata, nzdata) != Z_OK) return -1;
    //else return 0;

    // return -1;
    int err = 0;
    z_stream d_stream;// = {0}; /* decompression stream */
    static char dummy_head[2] = 
    {
        0x8 + 0x7 * 0x10,
        (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
    };
    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;
    d_stream.next_in  = zdata;
    d_stream.avail_in = 0;
    d_stream.next_out = data;
    if(inflateInit2(&d_stream, 47) != Z_OK) return -1;
    while (d_stream.total_out < *ndata && d_stream.total_in < nzdata) {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        if((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) break;
        if(err != Z_OK )
        {
            if(err == Z_DATA_ERROR)
            {
                d_stream.next_in = (Bytef*) dummy_head;
                d_stream.avail_in = sizeof(dummy_head);
                if((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) 
                {
                    return -1;
                }
            }
            else return -1;
        }
    }
    if(inflateEnd(&d_stream) != Z_OK) return -1;
    *ndata = d_stream.total_out;
    return 0;

}

int gzip_test() {
	char 				*filename = "/home/crazymind/cache/sohu.html";
	FILE 				*file;
	uLong		 		 flen;
	unsigned char 		*fbuf = NULL;
	unsigned char 		*ubuf = NULL, *temp = NULL;
	uLong 				 clen;
	unsigned char		*cbuf = NULL; 
	struct timeval 		 tv, newtv;
	unsigned char 	 	 url[1000];	


	if ((file = fopen(filename, "rb")) == NULL) {
		printf("open error\n");
		return -1;
	}

	fseek(file, 0L, SEEK_END);
	flen = ftell(file);
	fseek(file, 0L, SEEK_SET);
	if ((fbuf = (unsigned char *)malloc(sizeof(unsigned char) * flen)) == NULL) {
		fclose(file);
		return -1;
	}
	fread(fbuf, sizeof(unsigned char), flen, file);

	if ((ubuf = (unsigned char *)malloc(sizeof(unsigned char) * flen)) == NULL) {
		fclose(file);
		return -1;
	}


	clen = compressBound(flen);
	if ((cbuf = (unsigned char *)malloc(sizeof(unsigned char) * clen)) == NULL) {
		fclose(file);
		return -1;
	}

	if (compress(cbuf, &clen, fbuf, flen) != Z_OK) {
		return -1;
	}
	fclose(file);

	printf("check\n");

	gettimeofday(&tv, NULL);

	int i = 0, index = 0, end = 0, k = 0;
	for ( ; i < 1; i++) {

		if (ngx_http_prefetch_gzip_decompress(cbuf, clen, ubuf, &flen) != Z_OK) {
		//if (uncompress(ubuf, &flen, cbuf, clen) != Z_OK) {
			return -1;
		}
		k = flen;
		temp = ubuf;
		if (0) {
			while (1) {
				temp += index;
				k -= index;
				if (k < 0) break;
				index = kmp_search(temp, k, (u_char *)"http:", 5);
				if (index != -1) {
					end++;
					memcpy(url, temp + index, 100);
					//printf("%s\n", url);
					index += 100;
				} else {
					break;
				}
			}
			
		}
		//printf("%s\n", ubuf);
	}
	gettimeofday(&newtv, NULL);

	printf("%d, %d, count:%d\n", newtv.tv_sec - tv.tv_sec, (newtv.tv_usec - tv.tv_usec) / 1000, end);

	return clen;
}

void *process(void *arg) 
{
    printf("thread 0x%x working ok task %d\n", pthread_self(), *(int *)arg);
    sleep(1);
    printf("task %d is end\n", *(int *)arg);
    return NULL;
}

void test_thread()
{
    cx_thread_pool_t    *tpt = cx_thread_pool_create(1, 3, 20);

    int i = 0;
    int *num = (int *)malloc(sizeof(int) * 20);
    for ( ; i < 20; i++) {
        num[i] = i;
        printf("add task %d\n", i);
        cx_thread_pool_add(tpt, process, (void *)&num[i]);
    }
    sleep(20);
    cx_thread_pool_destroy(tpt);
}

int main() 
{

	//int ret = gzip_test();
	test_thread();
	//printf("ret:%d\n", ret);
	return 0;
}









