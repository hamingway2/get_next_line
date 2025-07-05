#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef BUFFER_SIZE
	# define BUFFER_SIZE 42
#endif


int pdb(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    dprintf(2, "[DEBUG] ");
    return vdprintf(2, fmt, args);

    va_end(args);
}

/* An individual inked list item. Each item never contains bytes from multiple lines */
typedef struct GNLChunk {
	ssize_t			len;					// length read into the chunk
	struct GNLChunk *next;                  // pointer to the next item
	char            buf[BUFFER_SIZE];       // the buffer itself
} GNLChunk;


/*
 * Holds the line currently being processed and some metadata about it
 *
 * Uses integers for references as they seem more convenient for fragmented memory
 *
 * head:     pointer to the first element of the linked list. Can be NULL
 * head:     pointer to the last element of the linked list. Can be NULL
 * len:      total size, in bytes
 * pos:      read position in the head buffer
 * sep:      pointer to the most recently identified \n in the head buffer. Can be NULL
 */
typedef struct GNLHolder {
	GNLChunk		*head;
	GNLChunk		*tail;
	ssize_t			len;
	char			*pos;
	char			*sep;
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
			pdb("malloc() failed for holders table\n");
			return NULL;
		}
		track->max += (i = track->max) + 4096;
		while (i < track->max) { // we pre allocate all the new holders. We'll never free them
			ret[i] = track->holders && (i < fd) ? track->holders[i] : malloc(sizeof(GNLHolder));
			if (! ret[i]) {
				pdb("malloc() failed for holders with FD #%lu/%lu\n", i, track->max);
				return NULL;
			}
			i++;
		}
		free(track->holders);
		track->holders = ret;
	}
	return ret;
}

/*
 * Appends a linked list chunk to the one in head (if it's not NULL)
 * Returns the new pointer or NULL in case of failure.
 * If the head is NULL, the added item is just created
 */
GNLChunk *gnlalloc(GNLHolder *h) {
GNLChunk *ret;

	if (! (ret = (GNLChunk *) malloc(sizeof(GNLChunk)))) {
		pdb("malloc() failed for chunk\n");
		return NULL;
	}
	if (h) {
		if (!h->head) {// initialise
			h->tail = (h->head = ret);
		} else {
			h->tail->next = ret;
			h->tail = ret;
		}
	}
	return ret; // initialise
}

/* looks for the first line feed within len bytes. Returns NULL if not found */
char *find_lf(char *str, ssize_t len) {
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
	starting at from and stopping at the next \n or the end of data.

	Elements in the list that are fully consumed are freed.

	head is changed to the first element of the first element
	with unconsumed data (NULL if there isn't any)

	from is changed to the address of the next \n that is
	encounered in the buffers (undefined behaviour if there was none)

	If dest is NULL, the copy is skipped and all elements are freed

	Returns:
		the number of bytes consumed
*/
ssize_t pack_and_free(char *dest, GNLHolder *h) {
ssize_t ret;
GNLChunk *o;

	ret = 0;
	while (h->head) {
		// We copy bytes in the following cases (assuming dest is passed):
		// if sep is set; if this is the EOF (self linked item)
		if (dest && (h->sep || (h->head->next == h->head))) {
			// stop at the known separator, or at a line feed, or at the end of the chunk
			while (h->pos != h->sep && (h->pos < h->head->buf + h->head->len)) {
				*(dest++) = *(h->pos++);
				ret++;
				pdb("PACKER: P:%lu S:%s\n", h->pos - h->head->buf, dest- 3);
			}
			h->sep = find_lf(h->head->buf, h->head->buf + h->head->len - h->pos); // more separators?
		}
		// eventually free and swap if chunk is fully consumed, or if requested
		if (!dest || (h->sep >= h->pos)) {
			o = h->head;
			if (h->head->next != h->head)
				h->head = h->head->next;
			free(o);
		}
	}
	return ret;
	

	/*
	p = head->buf;
	while (p != lf && p - head->buf < BUFFER_SIZE) { // start is considered only for the first element
		*(dest + (p - head->buf)) = *p;
		p++;
	}
	if (head->next) { // is there a follow up?
		ret = pack_and_free(dest ? dest + (p - head->buf) : NULL, head->next, lf);
		free(head);
	} else {
		ret = head; // deliver last member
	}
	return ret;
	*/
}

ssize_t render(char **dest, GNLHolder *h) {
size_t n;

	if (! (dest = malloc(h->len * sizeof(char)))) { // we always null-terminate
		pdb("malloc failed for output string(%d bytes)\n", h->len * sizeof(char));
		return NULL;
	}
	pdb("Packing reported %d bytes\n", n);
	return pack_and_free(&dest, h);

}


char *get_next_line(int fd) {
static GNLTracker t;
ssize_t r_b; // bytes read
GNLChunk *r_tgt; // read target
char *ret;


	if (! pad_table(&t, fd))
		return NULL;

	if (r_b = render(ret, t.holders[fd])) {
		return ret;
	}

	while ((r_tgt = gnlalloc(t.holders[fd]))) {
		r_b = read(fd, r_tgt->buf, BUFFER_SIZE); // short read (we don't care about errors)
		pdb("R: %lu\n", r_b);
		if (r_b < BUFFER_SIZE) {
			pdb("SHORT READ\n");
			r_tgt->next = r_tgt; // EOF member
			if (r_b <= 0) { // bypass creating the [last] empty member
				pdb("NULL READ\n");
				free(r_tgt);
				if (t.holders[fd]->tail) // link EOF member to itself
					t.holders[fd]->tail->next = t.holders[fd]->tail;
				break;
			}
		}
		// to avoid copying memory twice, we always read directly into the chunk.
		// If the list has no head, that is what r_tgt will be
		if (! t.holders[fd]->head) { // initiate or append to the list
			t.holders[fd]->head = r_tgt;
		} else {
			t.holders[fd]->head->next = r_tgt;
		}
		if (!t.holders[fd]->pos) // alignment and accounting
			t.holders[fd]->pos = r_tgt->buf;
		t.holders[fd]->len += (r_tgt->len = r_b);

		if ((t.holders[fd]->sep = find_lf(r_tgt->buf, r_b))) // look for first separator
			break;
	}
	if (!r_tgt)
		return NULL; // state is corrupt anyway

	if (r_b = render(ret, t.holders[fd])) {
		return ret;
	}


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
		pdb("Next output line: |%s|\n", next_line);
		return 0;
	}
	return 0;
	
	close(fd);
	//read(1, (char *) NULL, 1024);
	return 0;
}
