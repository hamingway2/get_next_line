/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_utils.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azielnic <azielnic@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/13 16:58:37 by azielnic          #+#    #+#             */
/*   Updated: 2025/07/07 16:39:43 by azielnic         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line.h"

size_t	ft_strlen(const char *str)
{
	size_t	i;

	i = 0;
	while (str[i] != '\0')
		i++;
	return (i);
}

void	*ft_calloc(size_t nmemb, size_t size)
{
	void			*ptr;
	unsigned char	*p_str;
	size_t			total;
	size_t			i;

	total = nmemb * size;
	if (nmemb == 0 || size == 0)
	{
		ptr = malloc(0);
		if (ptr)
			*((unsigned char *)ptr) = 0;
		return (ptr);
	}
	if (total / size != nmemb)
		return (NULL);
	ptr = (void *)malloc(total);
	if (!ptr)
		return (NULL);
	i = 0;
	p_str = (unsigned char *)ptr;
	while (i < (total))
		p_str[i++] = (0);
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

char	*ft_strdup(const char *s)
{
	char	*s_dup;
	size_t	i;
	size_t	len;

	len = (ft_strlen(s) + 1);
	s_dup = ft_calloc(len, sizeof(char));
	if (!s_dup)
		return (NULL);
	i = 0;
	while (i < len)
	{
		s_dup[i] = s[i];
		i++;
	}
	return (s_dup);
}
