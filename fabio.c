#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdio.h>

#ifndef BUFFER_SIZE
	# define BUFFER_SIZE 42
#endif


/* An individual inked list item. Each item never contains bytes from multiple lines */
typedef struct GNLChunk {
	ssize_t         len;                    // usable length (either offset of next \n, EOF or BUFFER_SIZE)
	struct GNLChunk *next;                  // pointer to the next item
	char            buf[BUFFER_SIZE];       // the buffer itself
} GNLChunk;
//typedef struct GNLChunk GNLChunk;


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
	int      max;              	    // Number of the highest FD we're tracking
	GNLChunk **latest;              // an array of pointers to the first element of the FD-owned linked list,
									// indexed by FD number.
									//
	                                // A given member is NULL if there were no leftovers (or on its first iteration)
									// gets allocated and copied dynamically as the highest FD number goes up
									// which wastes quite a bit of memory.
									//
	                                // This project is too small for a binary search effort
									// The memory to each leftover buffer is inevitably leaked if they don't
									// call us for a given file descriptor until EOF. Also
									// the memory for the array pointers is always leaked
} GNLTracker;


// 	return ret;

// }

// expands the catalog of file descriptor-indexed linked lists to accomodate for the highest FD number
// Returns the new pointer to the list of chunks, or NULL in case of failure
GNLChunk **expand_fd_catalog(GNLTracker *track, int fd) {
GNLChunk **ret;
int i;
	if (fd < 0)
		return NULL;
	// static initialization sentinels
	ret = track->latest;
	track->max = ret ? track->max : -1;
	while (fd > track->max) { // extend the tracking table and initialise it with null pointers
		if (! (ret = (GNLChunk**)malloc(sizeof(GNLChunk*) * (track->max + 4096))))
			return ret;
		track->max += 4096 + (i = 0);
		do { // these MUST be null pointers, so we double down on initialising memory
			ret[i] = track->latest && (i < fd) ? track->latest[i] : NULL;
			i++;
		} while (i < track->max);
		free(track->latest);
		track->latest = ret;
	}
	return ret;
}

/*
 * Appends a linked list chunk to the one in head (if it's not NULL)
 * Returns the new pointer or NULL in case of failure.
 * If the head is NULL, the added item is just created
 */
GNLChunk *chunk_add(GNLChunk *head) {
GNLChunk *ret;
	if (! (ret = (GNLChunk *) malloc(sizeof(GNLChunk)))) {
		return NULL;
	}
	if (head)
		head->next = ret;
	return ret;
}

/*
	If dest is NULL, simply releases the whole list.

	If dest is not NULL, unwinds and copies buffers in the linked list,
	for n bytes, into dest.
	Every element that is copied is freed.
	If there are more chunks after the element at n distance,
	the first that is left is returned as the new head.


	Returns:
		a pointer to the new head, or NULL in case of failure.
		always returns NULL if dest is passed as NULL
*/
GNLChunk *pack_and_free(char *dest, GNLChunk *head, ssize_t n) {
GNLChunk *ret;
ssize_t i;

	i = 0;
	while (dest && i < head->len) {
		dest[i] = head->buf[i];
		i++;
	}
	if (head->next) { // is there a follow up?
		ret = (i < n || !dest) ? pack_and_free(dest ? &dest[i] : NULL, head->next, n - i) : head->next;
	}
	if (i >= n || !dest) // member fully consumed, or they want to free anyway
		free(head);
	return ret;
}

char *get_next_line(int fd) {
static GNLTracker track;
ssize_t r_b; // bytes read
ssize_t lf_dist; // distance to the next LF in the linked list
GNLChunk *read_tgt; // read target

	if (! expand_fd_catalog(&track, fd))
		return NULL;

	lf_dist = 0;
	while ((read_tgt = chunk_add(NULL))) {
		if ((r_b = read(fd, read_tgt->buf, BUFFER_SIZE)) <= 0) {
			free(read_tgt); // useless allocation
			break;
		}
		// to avoid copying memory twice, we always read directly into the chunk.
		// If the list has no head, that is what read_tgt will be
		if ((track.latest[fd] = track.latest[fd] ? track.latest[fd] : read_tgt) != read_tgt)
			track.latest[fd]->next = read_tgt; // initiate or append to the list
		while (read_tgt->len++ < r_b) {
			if (read_tgt->buf[read_tgt->len] == '\n') {
				if (read_tgt->buf[read_tgt->len] >= r_b)
					read_tgt = chunk_add(read_tgt); // encountered newline, but there are more bytes
				printf("DIST_B: R=%lu M=%lu D=%lu S=`%s`\n", r_b, read_tgt->len, lf_dist, read_tgt->buf);
				break;
			}
			lf_dist++;
		}
	}

	if (r_b >= 0) {
	}
	
	//free_chunks(track.latest[fd]);
	return NULL;


}

int main(int argc, char *argv[]) {
int fd;
char *next_line;
	
	(void) argc; (void) argv;

	
	
	if (0 > (fd = open("/etc/passwd", O_RDONLY))) {
		// open failure
		return 3;
	}


	while ((next_line = get_next_line(fd))) {
		printf("L: %p\n", next_line);
	}
	return 0;
	
	close(fd);
	//read(1, (char *) NULL, 1024);
	return 0;
}
