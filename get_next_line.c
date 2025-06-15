/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azielnic <azielnic@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/04 18:10:18 by azielnic          #+#    #+#             */
/*   Updated: 2025/06/15 23:33:44 by azielnic         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

char	*get_next_line(int fd)
{
	static char *content; //will contain content of the whole file
	char	*buff = 0; //has to be manually be null-terminated
	int	buffer_size = 5; //user input

	buff = malloc(sizeof(buffer_size + 1));
	content = read(fd, buff, buffer_size);

	return (buff); //has to return number of bytes read
}
#include <stdio.h>

int	main()
{
	//char *next_line = NULL;
	int	fd;
	int	i = 0;
	
	fd = open("text.txt", O_RDONLY);
	while(i < 3)
	{
		printf("%s", get_next_line(fd));
		i++;
	}
	close(fd);

	return (0);
}

