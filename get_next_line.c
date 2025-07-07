/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azielnic <azielnic@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/27 15:53:43 by azielnic          #+#    #+#             */
/*   Updated: 2025/07/07 18:03:02 by azielnic         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line.h"

static char	*ft_substr(char const *s, unsigned int start, size_t len)
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

static ssize_t	read_into_hold(char **hold, int fd)
{
	char		*buffer;
	char		*temp;
	ssize_t		read_count;

	buffer = ft_calloc((BUFFER_SIZE + 1), (sizeof(char)));
	if (!buffer)
		return (-1);
	read_count = 0;
	while (!ft_strchr(*hold, '\n'))
	{
		read_count = read(fd, buffer, BUFFER_SIZE);
		if (read_count <= 0)
			break ;
		buffer[read_count] = '\0';
		temp = ft_strjoin(*hold, buffer);
		free(*hold);
		*hold = temp;
		if (!hold || !*hold)
			return (-1);
	}
	free(buffer);
	return (read_count);
}

// Function that splits at /n and returns up until incl that 
// new line character. Updates the content of hold.
// 
static char	*extract_line(char **hold) //explain above better why double poiter is used
{
	char	*line;
	char	*temp;
	size_t	i;

	if (!hold || !*hold)
		return (NULL);
	if (**hold == '\0')
		return (free(*hold), *hold = NULL, NULL);
	i = 0;
	while ((*hold)[i] != '\n' && (*hold)[i] != '\0')
		i++;
	if ((*hold)[i] == '\n')
		i++;
	line = ft_substr(*hold, 0, i);
	if (!line)
		return (NULL);
	temp = ft_substr(*hold, i, ft_strlen(*hold) - i);
	if (!temp)
		return (NULL);
	free(*hold);
	*hold = temp;
	return (line);
}

char	*get_next_line(int fd)
{
	static char	*hold = NULL;

	if (fd < 0 || BUFFER_SIZE <= 0)
		return (free(hold), NULL);
	if (!hold)
		hold = ft_calloc(1, 1);
	if (!hold)
		return (NULL);
	if (read_into_hold(&hold, fd) == -1)
		return (NULL);
	return (extract_line(&hold));
}
