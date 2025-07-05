#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdio.h>

#ifndef BUFFER_SIZE
	# define BUFFER_SIZE 42
#endif


/* An individual inked list item. Each item never contains bytes from multiple lines */
typedef struct GNLChunk {
	struct GNLChunk *next;                  // pointer to the next item
	char            buf[BUFFER_SIZE];       // the buffer itself
} GNLChunk;


/*
 * Holds the line currently being processed and some metadata about it
 *
 * Uses integers for references as they seem more convenient for fragmented memory
 *
 * head:  pointer to the first element of the linked list
 * lf:    pointer to the most recently discovered \n in the list
 */
typedef struct GNLHolder {
	GNLChunk		*head;
	char			lf;
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
	int      	max;              	// Number of the highest FD we're tracking
	GNLHolder 	**holders;          // Array of pointers to lines holders, indexed by FD number. Can be NULL
} GNLTracker;



// expands the catalog of file descriptor-indexed linked lists to accomodate for the highest FD number
// Returns the new pointer to the list of chunks, or NULL in case of failure
GNLHolder **expand_table(GNLTracker *track, int fd) {
GNLHolder **ret;
int i;
	if (fd < 0)
		return NULL;
	// static initialization sentinels
	ret = track->holders;
	track->max = ret ? track->max : -1;
	while (fd > track->max) { // extend the tracking table and initialise it with null pointers
		if (! (ret = (GNLHolder**)malloc(sizeof(GNLHolder*) * (track->max + 4096))))
			return ret;
		track->max += 4096 + (i = 0);
		do { // we pre allocate all the new holders. We'll never free them
			ret[i] = track->holders && (i < fd) ? track->holders[i] : malloc(sizeof(GNLHolder));
			i++;
		} while (i < track->max);
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
GNLChunk *append_chunk(GNLChunk *head) {
GNLChunk *ret;
long unsigned int i;
	if (! (ret = (GNLChunk *) malloc(sizeof(GNLChunk)))) {
		return NULL;
	}
	// we don't trust malloc
	i = 0;
	while (i < sizeof(GNLChunk)) {
		((char *)ret)[i] = 0;
		i++;
	}
	if (head)
		head->next = ret;
	return ret;
}

/*

	Traverses the linked list.

	If dest is not NULL, unwinds and concatenates buffers from the items,
	until it runs into lf's address (or it exhausted the buffers if that
	is NULL).

	Elements in the list are freed() even when dest is NULL,

	EXCEPT THE LAST/ONLY ONE: a pointer to it is returned instead

	Returns:
		a pointer to the new head, or NULL in case of failure.
		always returns NULL if dest is passed as NULL
*/
GNLChunk *pack_and_free(char *dest, GNLChunk *head, const char *lf) {
GNLChunk *ret;
char *p;

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
}


char *get_next_line(int fd) {
static GNLTracker t;
ssize_t r_b; // bytes read
GNLChunk *r_tgt; // read target
char *p;


	if (! expand_table(&t, fd))
		return NULL;

	while ((r_tgt = append_chunk(NULL))) {
		if ((r_b = read(fd, r_tgt->buf, BUFFER_SIZE)) <= 0) {
			free(r_tgt); // useless allocation
			break;
		}
		// to avoid copying memory twice, we always read directly into the chunk.
		// If the list has no head, that is what r_tgt will be
		if (! t.holders[fd]->head) { // initiate or append to the list
			t.holders[fd]->head = r_tgt;
		} else {
			t.holders[fd]->head->next = r_tgt;
		}
		p = r_tgt->buf;
		while (p - r_tgt->buf < r_b && '\n' != *p)
			p++;
		printf("R=%lu E=%lu S=`%s`\n",
			r_b, p - r_tgt->buf, r_tgt->buf
		);
		break;
	}

	
	//free_chunks(track.line[fd]);
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
