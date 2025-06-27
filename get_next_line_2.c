/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azielnic <azielnic@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/26 18:25:26 by azielnic          #+#    #+#             */
/*   Updated: 2025/06/27 16:14:36 by azielnic         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*Repeated calls (e.g., using a loop) to your get_next_line() function should 
let you read the text file pointed to by the file descriptor, one line at a 
time.
• Your function should return the line that was read.
If there is nothing left to read or if an error occurs, it should return NULL.
• Make sure that your function works as expected both when reading a file and 
when reading from the standard input.
• Please note that the returned line should include the terminating \n 
character, except when the end of the file is reached and the file does not end 
with a \n character.
• Your header file get_next_line.h must at least contain the prototype of the
get_next_line() function.
• Add all the helper functions you need in the get_next_line_utils.c file.*/

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/*The  strlen() function calculates the length of the string pointed to by str, 
 excluding the terminating null byte ('\0').*/

size_t	ft_strlen(const char *str)
{
	size_t	i;

	i = 0;
	while (str[i] != '\0')
		i++;
	return (i);
}

/*The  bzero() function  erases  the data in the n bytes of the memory starting 
 at the location pointed to by s, by writing zeros (bytes containing '\0') to 
 that area.*/

void	ft_bzero(void *s, size_t n)
{
	unsigned char	*p_str;
	size_t			i;

	p_str = (unsigned char *) s;
	i = 0;
	while (i < n)
	{
		p_str[i] = (0);
		i++;
	}
}

/*The malloc() function allocates memory and leaves the memory uninitialized, 
whereas the calloc() function allocates memory and initializes all bits to zero.
calloc = contiguous allocation*/

//nmemb = number of members
//malloc(1) is returned in case of size 0 or nmemb 0 as the return value needs 
//to be unique and NULL is not unique. See yellow box in subject.
void	*ft_calloc(size_t nmemb, size_t size)
{
	void	*ptr;
	size_t	total;

	total = nmemb * size;
	if (nmemb == 0 || size == 0)
		return (malloc(1));
	if (total / size != nmemb)
		return (NULL);
	ptr = (void *)malloc(nmemb * size);
	if (!ptr)
		return (NULL);
	ft_bzero(ptr, (nmemb * size));
	return (ptr);
}

/*The memcpy() function copies n bytes from memory area src to memory area 
dest. The memory areas must not overlap. The memcpy() function returns a 
pointer to dest.*/

void	*ft_memcpy(void *dest, const void *src, size_t n)
{
	unsigned char	*p_dest;
	unsigned char	*p_src;
	size_t			i;

	p_dest = (unsigned char *) dest;
	p_src = (unsigned char *) src;
	i = 0;
	while (i < n)
	{
		p_dest[i] = p_src[i];
		i++;
	}
	return (dest);
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

/*Allocates memory (using malloc(3)) and returns a substring from the string 
’s’. The substring starts at index ’start’ and has a maximum length of ’len’.

s: The original string from which to create the substring.
start: The starting index of the substring within ’s’.
len: The maximum length of the substring.

Return value: The substring. NULL if the allocation fails.*/

char	*ft_substr(char const *s, unsigned int start, size_t len)
{
	char	*s_sub;
	size_t	s_len;
	size_t	i;

	s_len = ft_strlen(s);
	if (s_len < start)
		return (ft_strdup(""));
	if (len > s_len - start)
		len = s_len - start;
	s_sub = (char *) ft_calloc(len + 1, sizeof(char));
	if (s_sub == NULL)
		return (NULL);
	i = 0;
	while (i < len)
	{
		s_sub[i] = s[i + start];
		i++;
	}
	return (s_sub);
}
/*The strchr() function returns a pointer to the first occurrence of the 
 character c in the string s. Here "character" means "byte"; these functions 
 do not work with wide or multibyte characters.

 strchr = string character

 The strchr() and strrchr() functions return a pointer to the matched character 
 or NULL if the character is not found. The terminating null byte is 
 considered part of the string, so that if c is specified as '\0', these 
 functions return a pointer to the terminator.*/

//casting the int into an unsigned char is necessary to compare the char in
//string with the charater in int disregarding higher bits.
char	*ft_strchr(const char *str, int c)
{
	const unsigned char	*p_str;
	unsigned char		ch;
	size_t				i;

	p_str = (unsigned const char *)str;
	ch = (unsigned char) c;
	i = 0;
	while (p_str[i] && p_str[i] != ch)
		i++;
	if (p_str[i] == ch)
		return ((char *)&p_str[i]);
	return (NULL);
}

/*Allocates memory (using malloc(3)) and returns a new string, which is the 
result of concatenating ’s1’ and ’s2’.

s1: The prefix string.
s2: The suffix string.

Return value: The new string. NULL if the allocation fails.*/

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

char	*get_next_line(int fd)
{
	//check out struct could be useful for this
	
	static char *hold;
	char		*line;
	char		*buffer;
	int			BUFFER_SIZE = 6; //goes in header
	size_t		i;
	ssize_t		read_count;

	buffer = ft_calloc((BUFFER_SIZE + 1), sizeof(char));
	hold = NULL;
	i = 0;
	while ((read_count = read(fd, buffer, BUFFER_SIZE)) >= 0)
	{	
		if ((ft_strchr(buffer, '\n') != NULL))
		{
			while (buffer[i])
			{
				if (buffer[i++] == '\n')
					break;
			}
			line = ft_substr(buffer, 0, i + 1);
			hold = ft_substr(buffer, i, ft_strlen(buffer) - i);
			break;
		}
		else if ((ft_strchr(buffer, '\n') != NULL))
		{
			printf("else has been entered\n");
			line = ft_strdup(hold);
			hold = ft_strjoin(line, ft_substr(buffer, 0, BUFFER_SIZE));
			free(line);
			free(buffer);
		}
	}
	if (read_count < 0)
			return (free(buffer), NULL);
	free(buffer);
	return (line);
}

#include <stdio.h>

int	main()
{
	//char *next_line = NULL;
	int	fd;
	int	i = 0;
	
	fd = open("text.txt", O_RDONLY);
	while(i < 500)
	{
		printf("|%s",  get_next_line(fd));
		i++;
	}
	close(fd);
	return (0);
}