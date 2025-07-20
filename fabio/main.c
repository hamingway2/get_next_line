/*
 * CPU and cache-conserving (by avoiding repeated scans & memory copies) version of
 * get_next_line().
 * Implemented by read()ing directly into a buffer linked list
 *
 * BUGS:	File descriptors numbers can be dup2()ed into the billions even when the
 *			process has only a handful of them open.
 *			At the moment, the file descriptor table is dynamically allocate and
 *          linearly addressed.
 *			Such a high file descriptor number would, at best, cause a
 *			massive slowdown and allocate gigabytes of memory, at worst seize your
 *			computer (unless running in a memory-limited control group) and be met by
 *			the OOM killer anyway anyway.
 *			An indirect lookup table with binary search/content addressable memory
 *			could handle that problem, however the current limits imposed by the
 *			Norminette + limit to number of support functions in this assignment
 *			would make that particularly difficult.
 *
 * NOTE:	the file descriptor holders table leaves behind dynamic allocations.
 *			Those are not leaks. If done with statics,
 *       	the compiler would simply map that .data and .bss area at
 *			program start instead
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef BUFFER_SIZE
	# define BUFFER_SIZE 42
#endif

#define HOLDERS_HEADROOM 256
#define DEBUG_MODE 1

typedef int fd_t; // we also use it for array sizes and such

int p_debug(const char *format, ...) {
int w_c;
static char out_buf[1024] = "[DEBUG]: ";
va_list args;

	// TODO: 	check assembly and see if the compiler is smart enough
	//			to turn the whole function into no-code
#if !(defined(DEBUG_MODE) && DEBUG_MODE)
	return 0;
#endif

	out_buf[sizeof(out_buf) - 1] = '\n';
    va_start(args, format);
    if (0 > (w_c = vsnprintf(&out_buf[9], sizeof(out_buf) - 9, format, args))) {
		write(2, "snprintf() failed\n", 18);
		exit(3);
	}
    va_end(args);
	return write(1, out_buf, 9 + w_c);


}

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
 * Keep it small as it gets copied by find_holder()
 *
 */
typedef struct GNLTracker {
	fd_t  		count;              // Number of FD holders in the tracker
	GNLHolder 	**holders;          // Array of pointers to lines holders, indexed by FD number. Can be NULL
} GNLTracker;

/*
 * Appends a linked list chunk to the holder
 * Returns the new pointer or NULL in case of failure.
 * If the head is NULL, the added item is just created
 */
GNLChunk *gnlalloc(GNLHolder *h) {
GNLChunk *ret;

	if (! (ret = (GNLChunk *) malloc(sizeof(GNLChunk)))) {
		p_debug("malloc() failed for chunk\n");
		return NULL;
	}
	ret->len = 0;
	ret->next = NULL;
	if (h) {
		if (!h->head) {// initialise
			p_debug("NEW HEAD: %p\n", (void *) ret);
			h->tail = (h->head = ret);
			h->pos = h->head->buf; // initialise read position
		} else { // append
			p_debug("APPENDED CHUNK: %p\n", (void *) ret);
			h->tail = ret;
			h->tail->next = ret;
		}
	}
	return ret; // initialise
}

/*
 * Returns the pointer to the holder for the requested FD, or NULL in case of failure.
 * Expands the holders' lookup table if needed.
 * Failures leave "track" unchanged, but will leak memory for the holders
 * allocated thus far
 */
GNLHolder *find_holder(GNLTracker *track, int fd) {
GNLHolder *ret;
GNLTracker tmp;
fd_t i;

	if (!track || fd < 0)
		return NULL;
	if (track->holders && fd < track->count)
		return track->holders[fd];

	p_debug("EXPANDING TRACKERS TABLE SIZE TO %d ENTRIES\n", fd + 1 + HOLDERS_HEADROOM);

	// expansion needed. We count on integer division truncation
	tmp.count = fd + 1 + HOLDERS_HEADROOM; // stdin will never be used, but so be it
	tmp.holders = (GNLHolder**)malloc(sizeof(GNLHolder*) * tmp.count);
	if (! tmp.holders) {
		p_debug("malloc() failed for new holders table\n");
		return NULL;
	}
	i = 0;
	ret = NULL;
	while (i < tmp.count) { // we pre allocate all the new holders. We'll never free them by design
		if (i < track->count) { // copy old?
			tmp.holders[i] = track->holders[i];
			continue;
		}
		tmp.holders[i] = malloc(sizeof(GNLHolder));
		if (! tmp.holders[i]) {
			p_debug("malloc() failed for holder for FD #%d (out of %d)\n", i, tmp.count);
			return NULL;
		}
		tmp.holders[i]->len = 0;
		tmp.holders[i]->nl = (tmp.holders[i]->pos = NULL);
		tmp.holders[i]->tail = (tmp.holders[i]->head = NULL);
		if (i == fd)
			ret = tmp.holders[fd];
		i++;
	}
	if (track->holders)
		free(track->holders);
	*track = tmp; // just two small members copy
	return ret;
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
	while (h->nl || (h->head && h->head->next && h->head->next == h->head)) {
		/*p_debug("HEAD IS %p\n", (void *) h->head);
		p_debug("PACKER BEFORE:  L: %lu, NL: %i\n",
			h->len, h->nl != NULL
		);*/
		if (dest) {
			// stop at the end of chunk buffer, or right after the separator
			while (h->pos < h->head->buf + h->head->len) {
				p_debug("COPYING %lu/%lu: %p(0x%02x) (`%p`)\n",
					h->pos - h->head->buf, h->head->len, h->pos, *h->pos, h->nl
				);
				*(dest++) = *(h->pos++);
				ret++;
				if (h->pos == h->nl + 1) {
					p_debug("FOUND LF AT CHUNK OFFSET %lu\n", h->nl - h->head->buf);
					h->nl = NULL; // found it
					break;
				}
			}

		}
		/*p_debug("PACKER AFTER:   L: %lu, NL: %i, (%lu bytes)\n",
			h->len, h->nl != NULL, h->pos - h->head->buf
		);*/
		// eventually free and swap if chunk is fully consumed, or if requested
		if (!dest || (h->pos == h->head->buf + h->head->len)) {
			p_debug("NO DEST OR CHUNK END REACHED FOR (%p)\n", (void *) h);
			o = h->head;
			if (o->next == h->head) {
				p_debug("DISCARDING HEAD: %p\n", (void *) h->head);
				// reset state
				h->nl = (h->pos = NULL);
				h->head = NULL;
			} else {
				h->head = h->head->next;
				h->pos = h->head->buf;
				p_debug("HEAD SHIFTED %p->%p\n",
					(void *) o, (void *) h->head
				);
			}
			h->len -= o->len;
			p_debug("FREEING OLD HEAD: %p (len left: %lu)\n", (void *) o, h->len);
			free(o);
		}
	}
	if (dest) {
		if (h->head) {
			// more newlines in this chunk?
			if (h->pos < h->head->buf + h->head->len) {
				h->nl = gnl_strchr(h->pos, (h->head->buf + h->head->len) - (h->pos)); // more separators?
				if (h->nl) {
					p_debug("Found continuation NL at chunk %p's offset %lu\n",
						(void *) h->head, h->nl - h->head->buf
					);
				} else {
					p_debug("Found chunk %p does not contain more \\n.\n",
						(void *) h->head
					);
				}
			} else {
				p_debug("WE'RE AT THE END OF CHUNK %d\n", h->head);
			}
		} else {
			p_debug("THERE IS NO REMAINING HEAD\n");
		}
	}
	p_debug("RETURNING: %lu(L: %p, NL: %lu)\n",
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
		p_debug("malloc failed for output string(%lu bytes)\n", h->len * sizeof(char));
		return NULL;
	}
	p_debug("Holder's length before packing: %lu\n", h->len);
	n = pack_and_free(ret, h);
	ret[n] = '\0';
	p_debug("Packed %lu bytes. %lu unscanned bytes remaining in the holder\n",n, h->len);
	if (n < 0)
		return NULL;
	return ret;
}


char *get_next_line(int fd) {
static GNLTracker t; // C99 6.7.8, PT10 = for statics and globals, all members are initialised as NULL/0
GNLHolder *lh;
GNLChunk *r_tgt; // read target

	lh = find_holder(&t, fd);
	if (! lh)
		return NULL;

	while (! lh->nl) { // we get more data only if we have no NL yet

		r_tgt = gnlalloc(lh);
		if (! r_tgt)
			return NULL; // state is corrupt anyway
		r_tgt->len = read(fd, r_tgt->buf, BUFFER_SIZE); // short read (we don't care about errors)
		p_debug("READ BUFFER: (%lu bytes)\n", r_tgt->len);
		if (r_tgt->len < BUFFER_SIZE) {
			if (r_tgt->len == 0) { // bypass creating the [last] empty member
				p_debug("NULL READ/READ ERROR(%lu). FREEING CHUNK\n", r_tgt->len);
				if (lh->tail) // link EOF member to itself. Might be this very one
					lh->tail->next = lh->tail;
				free(r_tgt);
				break;
			}
			r_tgt->next = r_tgt; // EOF member
		}
		lh->len += r_tgt->len;
		p_debug("New length of holder is %lu\n", lh->len);

		lh->nl = gnl_strchr(r_tgt->buf, r_tgt->len);
		if (lh->nl) {
			p_debug("FOUND NEWLINE AT GLOBAL OFFSET: %lu\n",
				lh->len - r_tgt->len + lh->nl - r_tgt->buf
			);
			break;
		}
	}

	return render(lh);


}

const char *messages[] = {
    "/etc/passwd",
    "/etc/passwd",
    "Foo",
    "Bar"
};
int main(int argc, char *argv[]) {
int fd;
char *next_line;

	(void) argc; (void) argv;

	setvbuf(stdout, NULL, _IONBF, 0);

	if (0 > (fd = open("/dev/shm/ciccio", O_RDONLY))) {
		// open failure
		return 3;
	}

	while ((next_line = get_next_line(fd))) {
		printf("LLLLLLLLLLLLLLLLLLLLLLLLLLLL:|%s|\n", next_line);
		free(next_line);
	}
	close(fd);
	p_debug("FINISHED\n");
	//read(1, (char *) NULL, 1024);
	return 0;
}