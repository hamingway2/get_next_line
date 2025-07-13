#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef BUFFER_SIZE
	# define BUFFER_SIZE 10
#endif



/* An individual inked list item. Each item never contains bytes from multiple lines */
typedef struct GNLChunk {
	ssize_t		len;					 // length read into the chunk
	struct 		GNLChunk *next;          // pointer to the next item
	char        buf[BUFFER_SIZE];        // the buffer itself
} GNLChunk;



/*
 * Holds the line currently being processed and some metadata about it
 *
 * Uses integers for references as they seem more convenient for fragmented memory
 *
 * head:   	pointer to the first element of the linked list. Can be NULL
 * tail:   	pointer to the last element of the linked list. Can be NULL
 * len:    	total size, in bytes, of the EFFECTIVE string, final \n included if present
 * pos:   	pointer to currrent read position in the head buffer
 * nl:   	pointer to byte after the earliest \n after pos.
 *			Might be several chunks ahead.
 *			NULL if there isn't any
 */
typedef struct GNLHolder {
	GNLChunk		*head;
	GNLChunk		*tail;
	ssize_t			len;
	char			*pos;
	char			*nl;
} GNLHolder;


/*
 * State of the get_next_line(). Meant to be used as a static.
 *
 * https://stackoverflow.com/questions/13251083/the-initialization-of-static-variables-in-c
 * So all statics are initialised as nullchars and null pointers
 *
 * Could be defined inline in the function
 * but I am not sure that the Norminette would like it
 *
 */
typedef struct GNLTracker {
	ssize_t  	max;              	// Number of the highest FD we're tracking
	GNLHolder 	**holders;          // Array of pointers to lines holders, indexed by FD number. Can be NULL
} GNLTracker;



// expands the catalog of file descriptor-indexed linked lists to accomodate for the highest FD number
// Returns the new pointer to the list of chunks, or NULL in case of failure
GNLHolder **pad_table(GNLTracker *track, int fd) {
GNLHolder **ret;
ssize_t i;
	if (fd < 0)
		return NULL;
	// static initialization sentinels
	ret = track->holders;
	track->max = ret ? track->max : 0;
	while (!fd || fd > track->max) { // account for stdin
		if (! (ret = (GNLHolder**)malloc(sizeof(GNLHolder*) * (track->max + 4096)))) {
			printf("malloc() failed for holders table\n");
			return NULL;
		}
		track->max += (i = track->max) + 4096;
		while (i < track->max) { // we pre allocate all the new holders. We'll never free them
			ret[i] = track->holders && (i < fd) ? track->holders[i] : malloc(sizeof(GNLHolder));
			if (! ret[i]) {
				printf("malloc() failed for holders with FD #%lu/%lu\n", i, track->max);
				return NULL;
			}
			ret[i]->len = 0;
			ret[i]->nl = NULL;
			i++;
		}
		free(track->holders);
		track->holders = ret;
	}
	return ret;
}

/*
 * Appends a linked list chunk to the holder
 * Returns the new pointer or NULL in case of failure.
 * If the head is NULL, the added item is just created
 */
GNLChunk *gnlalloc(GNLHolder *h) {
GNLChunk *ret;

	if (! (ret = (GNLChunk *) malloc(sizeof(GNLChunk)))) {
		printf("malloc() failed for chunk\n");
		return NULL;
	}
	if (h) {
		if (!h->head) {// initialise
			printf("NEW HEAD: %p\n", (void *) ret);
			h->tail = (h->head = ret);
			h->pos = h->head->buf; // initialise read position
		} else { // append
			printf("APPENDED CHUNK: %p\n", (void *) ret);
			h->tail->next = ret;
			h->tail = ret;
		}
	}
	return ret; // initialise
}

/* looks for the first line feed within len bytes.
 */
char *gnl_strchr(char *str, int len) {
ssize_t i;
	i = 0;
	while (i < len) {
		if (str[i] == '\n')
			return str + i;
		i++;
	}
	return NULL;
}

/*

	Traverses the linked list in the holder.

	If dest is not NULL, unwinds and concatenates buffers from the items,
	starting at from and stopping at the next \n or the end of data (not included).

	Elements in the list that are fully consumed are freed.

	head is changed to the first element with unconsumed data (NULL if there isn't any)

	from is changed to the address of the next \n that is
	encounered in the buffers (undefined behaviour if there was none)

	If dest is NULL, the copy is simlpy skipped and all elements are freed

	Returns:
		the number of bytes consumed
*/
int pack_and_free(char *dest, GNLHolder *h) {
ssize_t ret;
GNLChunk *o;

	ret = 0;
	while (h->nl || (h->head && h->head->next == h->head)) {
		/*printf("HEAD IS %p\n", (void *) h->head);
		printf("PACKER BEFORE:  L: %lu, NL: %i\n",
			h->len, h->nl != NULL
		);*/
		if (dest) {
			// stop at the end of chunk buffer, or right after the separator
			while (h->pos < h->head->buf + h->head->len) {
				printf("COPYING %lu/%lu: %p(`%c`) (`%p`)\n",
					h->pos - h->head->buf, h->head->len, h->pos, *h->pos, h->nl
				);
				*(dest++) = *(h->pos++);
				ret++;
				if (h->pos == h->nl + 1) {
					printf("FOUND LF AT CHUNK OFFSET %lu\n", h->nl - h->head->buf);
					h->nl = NULL; // found it
					break;
				}
				if ((*(h->pos - 1) & 0x80) || *(h->pos - 1) == 0)
					return -1;

			}

		}
		/*printf("PACKER AFTER:   L: %lu, NL: %i, (%lu bytes)\n",
			h->len, h->nl != NULL, h->pos - h->head->buf
		);*/
		// eventually free and swap if chunk is fully consumed, or if requested
		if (!dest || (h->pos == h->head->buf + h->head->len)) {
			printf("NO DEST OR CHUNK END REACHED FOR (%p)\n", (void *) h);
			o = h->head;
			if (o->next == h->head) {
				printf("DISCARDING HEAD: %p\n", (void *) h->head);
				h->nl = NULL;
				h->pos = NULL; // reset state
				h->head = NULL;
			} else {
				h->head = h->head->next;
				h->pos = h->head->buf;
				printf("HEAD SHIFTED %p->%p\n",
					(void *) o, (void *) h->head
				);
			}
			h->len -= o->len;
			printf("FREEING OLD HEAD: %p (len left: %lu)\n", (void *) o, h->len);
			free(o);
		}
	}
	if (dest) {
		if (h->head) {
			if (h->pos < h->head->buf + h->head->len) {
				h->nl = gnl_strchr(h->pos, (h->head->buf + h->head->len) - (h->pos)); // more separators?
				if (h->nl) {
					printf("Found \\n at chunk %p's offset %lu\n",
						(void *) h->head, h->nl - h->head->buf
					);
				} else {
					printf("Found chunk %p does not contain more \\n.\n",
						(void *) h->head
					);
				}
			}
		} else {
			printf("THERE IS NO REMAINING HEAD\n");
		}
	}
	printf("RETURNING: %lu(L: %p, NL: %lu)\n",
		ret, (void *) h->head, h->nl ? h->nl - h->head->buf : -1
	);
	return ret;
}


char *render(GNLHolder *h) {
char *ret;
ssize_t n;

	if (!h->len)
		return NULL;
	// add a byte for the final nullchar
	if (! (ret = malloc((h->len + 1) * sizeof(char)))) { // we always null-terminate
		printf("malloc failed for output string(%lu bytes)\n", h->len * sizeof(char));
		return NULL;
	}
	printf("Holder's length before packing: %lu\n", h->len);
	n = pack_and_free(ret, h);
	ret[n] = '\0';
	printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: %s\n", ret);
	printf("Packed %lu bytes. %lu unscanned bytes remaining in the holder\n",n, h->len);
	if (n < 0)
		return NULL;
	return ret;
}


char *get_next_line(int fd) {
static GNLTracker t;
GNLChunk *r_tgt; // read target


	if (! pad_table(&t, fd))
		return NULL;

	while (! t.holders[fd]->nl) { // we get more data only if we have no NL yet

		r_tgt = gnlalloc(t.holders[fd]);
		if (! r_tgt)
			return NULL; // state is corrupt anyway
		r_tgt->len = read(fd, r_tgt->buf, BUFFER_SIZE); // short read (we don't care about errors)
		printf("READ BUFFER: (%lu bytes)\n", r_tgt->len);
		if (r_tgt->len < BUFFER_SIZE || r_tgt->len == -1) { // it's unsigned, account for wraps
			if (r_tgt->len == 0) { // bypass creating the [last] empty member
				printf("NULL READ. FREEING CHUNK\n");
				free(r_tgt);
				if (t.holders[fd]->tail) // link EOF member to itself
					t.holders[fd]->tail->next = t.holders[fd]->tail;
				break;
			}
			r_tgt->next = r_tgt; // EOF member
		}
		t.holders[fd]->len += r_tgt->len;
		printf("New length of holder is %lu\n", t.holders[fd]->len);

		t.holders[fd]->nl = gnl_strchr(r_tgt->buf, '\n');
		if (t.holders[fd]->nl) {
			printf("FOUND NEWLINE AT GLOBAL OFFSET: %lu\n",
				t.holders[fd]->len - BUFFER_SIZE + t.holders[fd]->nl - r_tgt->buf
			);
			break;
		}
	}

	return render(t.holders[fd]);


}

int main(int argc, char *argv[]) {
int fd;
char *next_line;

	(void) argc; (void) argv;

	setvbuf(stdout, NULL, _IONBF, 0);

	if (0 > (fd = open("/etc/passwd", O_RDONLY))) {
		// open failure
		return 3;
	}

	while ((next_line = get_next_line(fd))) {
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!|%s|\n", next_line);
	}
	return 0;

	close(fd);
	//read(1, (char *) NULL, 1024);
	return 0;
}