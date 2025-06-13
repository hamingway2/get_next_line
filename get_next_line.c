/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azielnic <azielnic@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/04 18:10:18 by azielnic          #+#    #+#             */
/*   Updated: 2025/06/13 17:25:22 by azielnic         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

char	*get_next_line(int fd)
{
	static char *content; //will contain content of the whole file
	char	*buff = 0;
	int	buffer_size = 5; //user input

	malloc(sizeof(buff));
	content = read(fd, buff, buffer_size);

	return (buff);
}

int	main()
{
	#include <stdio.h>
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

