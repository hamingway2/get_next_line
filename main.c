#include "get_next_line.h"
# include <stdarg.h>
# include <fcntl.h>
# include <stdio.h>

int	main(void)
{
	char	*line;
	int		fd = open("text.txt", O_RDONLY);

	if (fd < 0)
		return (perror("open"), 1);
	while ((line = get_next_line(fd)))
	{
		printf("%s|", line);
		free(line);
	}
	close(fd);
	return (0);
}
