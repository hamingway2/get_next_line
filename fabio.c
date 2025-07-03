#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdio.h>

#ifndef BUFFER_SIZE
	# define BUFFER_SIZE 42
#endif


/* An individual inked list item */
typedef struct GNLChunk {
	size_t          start;                  // starting position in the string. Used only in leftovers
	struct GNLChunk *next;                  // pointer to the next item
	char            buf[BUFFER_SIZE];       // the buffer itself
} GNLChunk;
//typedef struct GNLChunk GNLChunk;


/*
 * State of the get_next_line(). Meant to be used as a static.
 * https://stackoverflow.com/questions/13251083/the-initialization-of-static-variables-in-c
 * Could be defined inline in the function
 * but I am not sure that the Norminette would like it
 *
 */
typedef struct GNLTracker {
	int      count;              	// Size of the FD lookup table. Doubles as the initialization sentinel
	GNLChunk **latest;              // last read chunk, actually an array of pointers indexed by FD number.
	                                // A given member is NULL if there were no leftovers (or on its first iteration)
									// gets allocated and copied dynamically as the highest FD number goes up
									// which wastes a bit of memory.
	                                // This project is too small for a binary search effort
									// The memory to each leftover buffer is inevitably leaked if they don't
									// call us for a given file descriptor until EOF. Also
									// the memory for the array pointers is always leaked
	char     buf[BUFFER_SIZE];      // the data buffer that is read
} GNLTracker;


// Reimplementation of realloc, as realloc() is not in the list of allowed functions
// we do NOT use this for the buffers themselves, as it is guaranteed to copy memory.
void *gnl_realloc(void *ptr, size_t size) {
void *ret;
unsigned long i;

	if (! size) {
		free(ptr);
		return NULL;
	}

	ret = malloc(size);

	if ((NULL != ret) && (NULL != ptr)) { // our own malloc() might have failed...
		i = -1; // save on the adder in the loop. Unsigned, so it will wrap around
		while (++i < size)
			((char *)ret)[i] = ((char *)ptr)[i];
		free(ptr);
	}
	
	return ret;
}

/* Copies n bytes of src into dest, but also stops at the first \n (included)
 * The number of copied bytes is returned
 * Stops at BUFFER_SIZE anyway
 */
size_t gnl_copy(char *dest, char *src, size_t n) {
size_t ret;

	ret = 0;
	while (ret < n) {
		dest[ret] = src[ret];
		ret++;
		if (src[ret] == '\n')
			break;
	}
	return ret;
}

/*
	Recursively assemble a whole linked list starting at head,
    into dest.
	Stops at the first \n (included) or the end of the last buffer


	NOTE: frees the very pointer that is passed to it

	If free_only is set, only frees the list and returns 0

	Returns:
		Total number of bytes written, including the final \n
*/
size_t assemble_and_free(char *dest, GNLChunk *head, char free_only) {
size_t ret;

	ret = 0;
	if (head->next != NULL)
		ret += assemble_and_free(dest + BUFFER_SIZE, head->next, free_only);
	if (! free_only)
		ret += gnl_copy(dest, head->buf + head->start, BUFFER_SIZE - head->start);
	free(head);

	return ret;
}

int main(int argc, char *argv[]) {
	
	(void) argc; (void) argv;
	return 0;
}


char *get_next_line(int fd) {
static GNLTracker tracker;
int r_b;
GNLChunk *next;

	while (fd >= tracker.count) { // extend the tracking table and initialise it with null pointers
		r_b = tracker.count + 4096; // usurp this variable
		tracker.latest = (GNLChunk**)gnl_realloc(tracker.latest, sizeof(GNLChunk*) * (tracker.count + 4096));
		while (tracker.count < r_b)
			tracker.latest[tracker.count] = (GNLChunk*) NULL;
	}

	// to avoid copying memory, we always read directly into the leftovers,
	// so the linked list ends there
	// Its pointer might already be at the end of it, but who cares?
	if (NULL == tracker.latest[fd])
		

	while ((r_b = read(fd, tracker.latest[fd].buf, BUFFER_SIZE)) > 0) {
		
		if ((GNLTracker.latest[fd] = (GNLChunk *) malloc(sizeof(GNLChunk)))) {
			break;
		}
		head = (NULL == head) ? NULL : head;
		
	}

	(first != NULL) ? free_chunks(first) : NULL;
	

	return full_line;

}

int main(int argc, char *argv[]) {
int fd;
char *next_line;
	
	(void) argc; (void) argv;
	
	if (0 > (fd = open("/etc/passwd", O_RDONLY))) {
		// open failure
		return 3;
	}
	

	while (NULL != (next_line = get_next_line(fd))) {
		
	}
	
	close(fd);
	//read(1, (char *) NULL, 1024);
	return 0;
}
