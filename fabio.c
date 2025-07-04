#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdio.h>

#ifndef BUFFER_SIZE
	# define BUFFER_SIZE 42
#endif


/* An individual inked list item */
typedef struct GNLChunk {
	ssize_t         mark;                   // usable length (either offset of the next \n or len at EOF)
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


/*
	Unwinds a linked list into dest, backwards in order to be able to
	bring back the last element at each iteration, and eventually free
	that one too if there is no data left

	Frees the list elements backwards from the first one containing 

	Stops at the first \n (included) or the end of the last buffer

	If there is remaining data after \n in the last

	NOTE: frees the very pointer in the linked list that is passed to it

	Returns:
		Total number of bytes written, including the final \n
*/
// size_t ll_actions(char *dest, GNLChunk *head) {
// size_t ret;

// 	ret = 0;
// 	if (head->next != NULL) {
// 		// look ahead into the list if this is not the end.
// 		// Copy
// 		ret += assemble_and_free(dest + BUFFER_SIZE, head->next, free_only);
// 	}
// 	if (NULL != dest)
// 		ret += (head->buf += gnl_copy(
// 			dest, head->buf + head->read, BUFFER_SIZE - head->read
// 		));
// 	
// 	free(head);

// 	return ret;

// }

// expands the catalog of file descriptor-indexed linked lists to accomodate for the highest FD number
// Returns the new pointer to the list of chunks, or NULL in case of failure
GNLChunk **expand_fd_catalog(GNLTracker *track, int fd) {
GNLChunk **ret;
int i;

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

char *get_next_line(int fd) {
static GNLTracker track;
ssize_t r_b; // bytes read
ssize_t lf_dist; // distance to the next LF in the linked list
GNLChunk *read_tgt; // read target

	expand_fd_catalog(&track, fd);
	lf_dist = 0;
	while ((read_tgt = (GNLChunk *) malloc(sizeof(GNLChunk)))) {
		if ((r_b = read(fd, read_tgt->buf, BUFFER_SIZE)) <= 0) {
			free(read_tgt);
			break; // error or EOF. We'll handle that later
		}
		// to avoid copying memory, we always read directly into the chunk.
		// If the list has no head, that is what read_tgt will be
		if ((track.latest[fd] = track.latest[fd] ? track.latest[fd] : read_tgt) != read_tgt)
			track.latest[fd]->next = read_tgt; // initiate or append to the list
		while (read_tgt->mark++ < r_b) {
			lf_dist++;
			if (read_tgt->buf[read_tgt->mark] == '\n') {
				printf("DIST_B: R=%lu M=%lu D=%lu S=`%s`\n", r_b, read_tgt->mark, lf_dist, read_tgt->buf);
				break;
			}
		}
	}

	if (r_b >= 0)
		;
	
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
