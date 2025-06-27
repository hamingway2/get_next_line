/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_3.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azielnic <azielnic@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/27 15:53:43 by azielnic          #+#    #+#             */
/*   Updated: 2025/06/27 18:02:37 by azielnic         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

size_t	ft_strlen(const char *str)
{
	size_t	i;

	i = 0;
	while (str[i] != '\0')
		i++;
	return (i);
}

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

char	*get_next_line(int fd)
{
	/*1. Allocate space for variable which read reads into
	2. Read into variable
	3. If read is done no longer read
	4. Check variable from previous read 
	5. Take more characters of buffersize
	6. Allocate new space which is previous variable plus new characaters
	7. Connect both the variables in the newly allocated space
	8. If new line in there then return only up to new line
	9. repeat
	*/

	static char *hold; //keeps leftover chars after new line
	char *buffer; //contains content of read size from fd
	char *temp; //needed to temporarily store stuff and let allocation happen
	char *output;
	size_t i = 0; //iterates through string
	ssize_t read_count; //output of read if -1 than fail
	int	BUFFER_SIZE = 20;

	if (fd < 0 && BUFFER_SIZE < 0)
		return (free(hold), NULL);
	temp = ft_calloc(1, 1);
	buffer = ft_calloc(1, 1);
	output = ft_calloc(1, 1);
	buffer = ft_calloc((BUFFER_SIZE + 1), (sizeof(char)));
	read_count = read(fd, buffer, BUFFER_SIZE);
	if (read_count == -1)
		return (NULL);
	if (!hold)
		hold = "";
	temp = ft_strjoin(hold, buffer);
	if (ft_strchr(temp, '\n')) //return only up until \n not more
	{
		while (temp[i] != '\n' && temp[i] != '\0') //special case if newline is at index 0
			i++;
		if (temp[i] == '\n')
			i++;
		output = ft_substr(temp, 0, i);
		hold = ft_substr(temp, i, ft_strlen(temp));
	}
	printf("%s\n", hold);
	// now need a loop and the handling of hold and free all stuff
	free(buffer);
	return (output);
	
}

int main()
{
	char *line;
	int	fd = open("text.txt", O_RDONLY);
	int	i = 0;

	while (i < 1)
	{
		printf("%s|", line = get_next_line(fd));
		i++;
	}
}