/*
 * CPU and cache-conserving version of get_next_line()
 * (by avoiding rescans & memory copies via read()s into linked list items).
 * The default buffer size of 42 is rather meager, so the performance gains
 * are questionable, but this implementation should be more or less O(N)
 * regarless of buffer size.
 *
 *
 * Terminates all lines in output with \0, although the assigment is unspecific
 * about it...
 *
 * WARNING!!! 	Avoid binary data for tests (see const declaration right before main()).
 *				The function requirements are bugged. This function
 * 				can't be correctly tested with any input that contains nullchars:
 *				one wouldn't be able to distinguish the end of the last line, if not
 *				terminated, from random out-of-bounds junk.

 *
 * Should be adaptable to Norminette restricitions by removing a few debug
 * statements and comments and condensing a few breaks and branch conditions that
 * exist only for clarity debugging into the condition itself,
 * with no further need for C assignment trickeries to fool the linter.
 * The norminette is really built around 80x25 terminals, nowadays that forces
 * people to write worse code in many cases ...
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
 * NOTE2:	main() is enormous and it uses many external libraries,
 *          globals and such (and so does the debugging infrastructure)
 *			That's because it's just a test harness
 *			The test simply calls get_next_line() in parallel against many
 *			file descriptors, and then hashes the original files and the
 *			resulting ones.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h> // for testing only
#include <stdio.h> // for testing only
#include <stdarg.h> // for testing only
#include <sys/stat.h> // for testing only
#include <string.h> // for testing only

#ifndef BUFFER_SIZE
	# define BUFFER_SIZE 42
#endif

#define HOLDERS_HEADROOM 256
#define DEBUG_MODE 0

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
 * Combined allocator and appender for chunks to the linked list.
 * If c passed as NULL, allocates a new chunk for a holder.
 * If c is a pointer to an already allocated chunk,
 * appends it to h and re-arranges tail, head and next accordingly.
 * If the chunk passed in A is smaller than a full buffer size, it is linked to
 * itself to indicate the end of the holder.
 * If the chunk has no lenght, the linked list not altered at all and the
 * !!!CHUNK IS FREED!!! and NULL is returned.
 *
 * If both are passed as NULL, both operations are performed (tail is not wrapped,
 * (however this is rarely useful).
 * In any case, it returns a pointer to c or NULL in case of NULL chunks
 * The function has no effect if both are passed as non-null
 */
GNLChunk *gnl_add_or_free(GNLChunk *c, GNLHolder *h) {

	if (!c) {
		if (! (c = (GNLChunk *) malloc(sizeof(GNLChunk)))) {
			p_debug("malloc() failed for chunk\n");
			return NULL;
		}
		c->len = 0;
		c->next = NULL;
		p_debug("ALLOCATED NEW CHUNK: %p\n", c);
	}
	if (h) {
		if (c->len < BUFFER_SIZE) {
			if (!c->len) {
				p_debug("REFUSING TO APPEND EMPTY CHUNK (FREEING): %p\n", (void *) c);
				free(c);
				return NULL;
			}
			p_debug("SELF LINKING TRUNCATED CHUNK: %p\n", c);
			c->next = c;
		}
		if (!h->head) {
			p_debug("CHUNK ASSIGNED AS HEAD: %p\n", (void *) c);
			h->tail = (h->head = c);
			h->pos = h->head->buf; // initialise read position
		} else { // append
			p_debug("CHUNK APPENDED TO %p: %p\n", (void *) c);
			h->tail->next = c;
			h->tail = c;
		}
		h->len += c->len;
		p_debug("HOLDER RESIZED: %lu -> %lu\n", h->len - c->len, h->len);
	}
	return c; // initialise
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
	// look for our NL pointer, until we encounter it or we drained the linked list
	while (h->head && !(h->nl && (h->pos == h->nl + 1))) {
		if (dest) {
			// stop at the end of chunk buffer, or right after the separator
			while (h->pos < h->head->buf + h->head->len) {
				p_debug("COPYING %lu/%lu: %p(0x%02x) (%p)\n",
					h->pos - h->head->buf, h->head->len, h->pos, *h->pos, h->nl
				);
				*(dest++) = *(h->pos++);
				ret++;
				if (h->nl && (h->pos == h->nl + 1)) {
					p_debug("REACHED LF POINTER AT CHUNK OFFSET %lu\n", h->nl - h->head->buf);
					// We don't exit the main loop yet. If we ALSO are at the end of the chunk,
					// the following cleanup/swap has to run.
					// We temporarily turn NL into a pointer to itself to indicate that
					// we found the NL and the main loop must exit
					break;
				}
			}
		}

		// Free and swap head if fully consumed, or if requested
		if (!dest || (h->pos == h->head->buf + h->head->len)) {
			p_debug("NO DEST OR CHUNK END REACHED FOR (%p)\n", (void *) h);
			o = h->head;
			if (o->next == h->head) {
				p_debug("HEAD IS LINKS TO ITSELF. DISCARDING: %p\n", (void *) h->head);
				// reset state
				h->head = (GNLChunk *)(h->nl = (h->pos = NULL));
			} else {
				h->pos = (h->head = o->next)->buf;
				p_debug("HEAD SHIFTED %p->%p\n",
					(void *) o, (void *) h->head
				);
			}
			h->len -= o->len;
			p_debug("FREEING OLD HEAD: %p (BYTES LEFT: %lu)\n", (void *) o, h->len);
			free(o);
		}
	}

	h->nl = NULL; // we right after a NL or at the end. Need to find the next one
	if (dest) {
		if (h->head) {
			if (h->pos < h->head->buf + h->head->len) {
				h->nl = gnl_strchr(h->pos, (h->head->buf + h->head->len) - h->pos); // more separators?
				if (h->nl) {
					p_debug("FOUNT FOLLOWING NL AT CHUNK %p, OFFSET %lu\n",
						(void *) h->head, h->nl - h->head->buf
					);
				} else {
					p_debug("Chunk %p does not contain more NLs\n",
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
	p_debug("RETURNING: %lu(L: %p, NL: %p)\n",
		ret, (void *) h->head, h->nl
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
	p_debug("HOLDER'S LENGTH BEFORE PACKING: %lu\n", h->len);
	n = pack_and_free(ret, h);
	ret[n] = '\0';
	p_debug("PACKED %lu BYTES. %lu TOTAL REMAINING BYTES\n",
		n, h->pos ? h->len - (h->pos - h->head->buf) : 0
	);
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

		r_tgt = gnl_add_or_free(NULL, NULL);
		if (! r_tgt)
			return NULL; // state is corrupt anyway
		r_tgt->len = read(fd, r_tgt->buf, BUFFER_SIZE); // short read (we don't care about errors)
		p_debug("READ BUFFER: (%lu bytes)\n", r_tgt->len);
		if (! gnl_add_or_free(r_tgt, lh))
			break;
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

// only for the test
const char *__test_files[] = {
    "/dev/null",
    "/etc/passwd",
    "/etc/hostname",
	"/etc/debconf.conf"
};
# define OUTPUT_FILE_TEMPLATE "/dev/shm/gnl_tmp_%d_%d" // PID, fd number
# define HASH_COMMAND_TEMPLATE "md5sum %s " OUTPUT_FILE_TEMPLATE

int main(int argc, char *argv[]) {
int fd_idx;
int fds_r[sizeof(__test_files) / sizeof(__test_files[0])];
int fds_w[sizeof(__test_files) / sizeof(__test_files[0])];
static char fn_buf[256];
char *next_line;
int fds_left;
int fd;

	(void) argc; (void) argv;
	(void) fds_r; (void) fds_w; (void) fn_buf; (void) fds_left; (void) fd;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	umask(077);


	// this for loop is only for the test. Opens all the files first
	for (fd_idx = 0; (long unsigned int)fd_idx < sizeof(fds_r) / sizeof(fds_r[0]); fd_idx++) {
		dprintf(2, "Opening `%s` for reading...\n", (__test_files[fd_idx]));
		if (0 > (fds_r[fd_idx] = open(__test_files[fd_idx], O_RDONLY))) {
			perror("..failed: ");
			return 3;
		}
		dprintf(2, "\n");
		// open a file in /tmp named after the file descriptor number
		if (0 > snprintf(fn_buf, sizeof(fn_buf), OUTPUT_FILE_TEMPLATE, getpid(), fds_r[fd_idx])) {
			write(2, "sprintf() failure\n", 18);
			return 3;
		}
		dprintf(2, "Opening `%s` for writing...\n", fn_buf);
		if (0 > (fds_w[fd_idx] = open(fn_buf, O_WRONLY | O_CREAT | O_TRUNC, 0666))) {
			perror("..failed: ");
			break;
		}

	}

	fds_left = sizeof(fds_r) / sizeof(fds_r[0]);
	dprintf(2, "Transferring lines...\n");
	dprintf(2, "WARNING!!! The requirements are bugged and this won't work with files that contain nullchars...\n");
	while (1) {
		for (fd_idx = 0; (long unsigned int)fd_idx < sizeof(fds_r) / sizeof(fds_r[0]); fd_idx++) {
			if (fds_w[fd_idx] < 0)
				continue;
			if ((next_line = get_next_line(fds_r[fd_idx]))) {
				/*dprintf(2, "Transferring %lu bytes from `%s` (%d file descriptors left)\n",
					strlen(next_line), __test_files[fd_idx], fds_left
				);*/
				write(fds_w[fd_idx], next_line, strlen(next_line));
				free(next_line);
				continue;
			}
			dprintf(2, "..Reached end of `%s`. Closing...\n", __test_files[fd_idx]);
			if (0 > close(fds_r[fd_idx])) {
				perror("....failed: ");
			}
			if (0 > snprintf(fn_buf, sizeof(fn_buf), OUTPUT_FILE_TEMPLATE, getpid(), fds_r[fd_idx])) {
				write(2, "sprintf() failure\n", 18);
				return 3;
			}
			dprintf(2, "....closing `%s`...\n", fn_buf);
			if (0 > close(fds_w[fd_idx])) {
				perror("......failed: ");
			}
			fds_w[fd_idx] = -1;
			fds_left -= 1;
		}
		if (! fds_left)
			break;

	}

	dprintf(2, "Comparing hashes...\n");
	for (fd_idx = 0; (long unsigned int)fd_idx < sizeof(fds_r) / sizeof(fds_r[0]); fd_idx++) {
		dprintf(2, "..%s\n", fn_buf);
		if (0 > snprintf(fn_buf, sizeof(fn_buf),
				HASH_COMMAND_TEMPLATE, __test_files[fd_idx], getpid(), fds_r[fd_idx]
		)) {
			write(2, "sprintf() failure\n", 18);
			return 3;
		}
		system(fn_buf);
	}



	// cleanup regardless. Files might not exist
	for (fd_idx = 0; (long unsigned int)fd_idx < sizeof(fds_r) / sizeof(fds_r[0]); fd_idx++) {
		if (0 > snprintf(fn_buf, sizeof(fn_buf), OUTPUT_FILE_TEMPLATE, getpid(), fds_r[fd_idx])) {
			write(2, "sprintf() failure\n", 18);
			return 3;
		}
		dprintf(2, "Deleting `%s`...\n", fn_buf);
		if (0 > unlink(fn_buf)) {
			perror("..failed: ");
		}
	}

	//~ (void) fds_r; (void) fds_w; (void) fn_buf; (void) fds_left;

	//~ if (0 > (fd = open("/etc/passwd", O_RDONLY))) {
		//~ // open failure
		//~ return 3;
	//~ }

	//~ while ((next_line = get_next_line(fd))) {
		//~ printf("%s", next_line);
		//~ free(next_line);
	//~ }
	//~ close(fd);



	//read(1, (char *) NULL, 1024);
	return 0;
}