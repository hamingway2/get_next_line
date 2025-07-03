/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azielnic <azielnic@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/04 18:10:18 by azielnic          #+#    #+#             */
/*   Updated: 2025/06/17 23:56:18 by azielnic         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*Repeated calls (e.g., using a loop) to your get_next_line() function should let
you read the text file pointed to by the file descriptor, one line at a time.
• Your function should return the line that was read.
If there is nothing left to read or if an error occurs, it should return NULL.
• Make sure that your function works as expected both when reading a file and when
reading from the standard input.
• Please note that the returned line should include the terminating \n character,
except when the end of the file is reached and the file does not end with a \n
character.
• Your header file get_next_line.h must at least contain the prototype of the
get_next_line() function.
• Add all the helper functions you need in the get_next_line_utils.c file.*/

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

char	*ft_strjoin(char const *s1, char const *s2)
{
	char	*joined_s;
	size_t	s_len;
	size_t	i;
	size_t	j;

	if (!s1 || !s2)
		return (NULL);
	i = 0;
	s_len = ft_strlen(s1) + ft_strlen(s2);
	joined_s = (char *) ft_calloc(s_len + 1, sizeof(char));
	if (!joined_s)
		return (NULL);
	while (s_len && s1[i])
	{
		joined_s[i] = s1[i];
		i++;
	}
	j = 0;
	while (s_len && s2[j])
	{
		joined_s[i + j] = s2[j];
		j++;
	}
	return (joined_s);
}

char	*ft_strdup(const char *s)
{
	char	*s_dup;

	s_dup = ft_calloc(ft_strlen(s) + 1, sizeof(char));
	if (!s_dup)
		return (NULL);
	s_dup = ft_memcpy(s_dup, s, (ft_strlen(s) + 1));
	return (s_dup);
}

char	*get_next_line(int fd)
{
		/*read into a static buffer up to buffersize long
	then check for /n if /n available return up to new line
	*/
	static char	buffer[BUFFER_SIZE];
	char	*output;
	char	*temp;
	int		count;
		
	if (BUFFER_SIZE < 0 || !BUFFER_SIZE)
		return (-1);
	count = read(fd, buffer, BUFFER_SIZE);
	if (count <= 0 || !count)
		return (-1);
	output = malloc(sizeof(BUFFER_SIZE + 1));
	if (!output)
		return (-1);
	output[BUFFER_SIZE] = '/0';
	output = ft_strjoin(output, temp);
	if (count < BUFFER_SIZE)
		return (&output);
		
		
	
	
	/*static char buffer[BUFFER_SIZE]; //will contain buffer_size or a multiple of it incl \n
	char *pre_stash;
	char	*line; //is supposed to be the cleaned up line ending with \n taken from the stash; has to be manually be null-terminated
	int	buffer_size = 5; //user input
	int	buffer_count;
	int	i;

	pre_stash = NULL;
	line = NULL;
	buffer_count = 0;
	i = 0;
	if (!fd || !buffer_size || buffer_size <= 0)
		return (NULL);
	pre_stash = malloc(sizeof(buffer_size + 1));
	buffer_count = read(fd, pre_stash, buffer_size);
	if (buffer_count < 0 || !buffer_count)
		return (-1);
	pre_stash[buffer_count] = '\0';
	//possible issue because I save stash in stash;
	stash = ft_strjoin(stash, pre_stash);
	free(pre_stash);
	while(stash[i])
	{
		if (stash[i] == '\n')
			return (&line);
		i++;
	}*/
	return (&line);
}
#include <stdio.h>

int	main()
{
	//char *next_line = NULL;
	int	fd;
	int	i = 0;
	char *next_line;

	fd = open("/etc/passwd", O_RDONLY);
	while (next_line = get_next_line(fd))
	{
		printf("%s", next_line);
		i++;
	}
	close(fd);

	return (0);
}

