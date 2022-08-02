#include "lemipc.h"

t_lemipc	g_lemipc;

size_t		align_up(size_t size, size_t align) {
	return (size + (align - (size % align)));
}

int			create_game(int fd)
{
	printf("Creating the game...\n");
	size_t	size = size = align_up(sizeof(t_game), getpagesize());
	if (ftruncate(fd, size) < 0)
	{
		dprintf(STDERR_FILENO, "%s: ftruncate(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
	{
		dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	uint8_t *map = (uint8_t *)((t_game *)addr)->map;
	for (size_t i = 0; i < sizeof(((t_game *)0)->map); i++)
			map[i] = '0';
	return (size);
}

int			join_game(int fd, size_t size, char *team_name)
{
	(void)team_name;
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
	{
		dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	for (size_t i = 0; i < sizeof(((t_game *)0)->map); i++)
		printf("%02x", ((char *)addr)[i]);
	printf("\n");
//	sprintf(addr, "Bonjour bonjour");
	sleep(10);
	//
	if (munmap(addr, size) < 0)
	{
		dprintf(STDERR_FILENO, "%s: munmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

void		signal_handler(int signum)
{
	(void)signum;
	if (g_lemipc.addr)
		munmap(g_lemipc.addr, g_lemipc.size);
	if (g_lemipc.fd >= 0)
		close(g_lemipc.fd);
//	TODO: only unlink if last player
//	shm_unlink(PRG_NAME);
	printf("\b\bLeaving the game...\n");
	exit(EXIT_SUCCESS);
}

int			lemipc(char *team_name)
{
	int		ret;
	struct	stat sb;

	if (signal(SIGINT, signal_handler) == SIG_ERR)
	{
		dprintf(STDERR_FILENO, "%s: signal(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if ((g_lemipc.fd = shm_open(PRG_NAME, O_RDWR | O_CREAT, 0644)) < 0)
	{
		dprintf(STDERR_FILENO, "%s: shm_open(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if (fstat(g_lemipc.fd, &sb) < 0)
	{
		dprintf(STDERR_FILENO, "%s: fstat(): %s\n", PRG_NAME, strerror(errno));
		ret = EXIT_FAILURE;
		goto end;
	}
	if (!sb.st_size)
	{
		if ((ret = create_game(g_lemipc.fd)) == EXIT_FAILURE)
			goto end;
		ret = join_game(g_lemipc.fd, ret, team_name);
	}
	else
		ret = join_game(g_lemipc.fd, sb.st_size, team_name);
end:
	close(g_lemipc.fd);
//	TODO: only unlink if last player
//	shm_unlink(PRG_NAME);
	return (ret);
}

int			main(int argc, char *argv[])
{
	if (argc != 2)
	{
		dprintf(STDERR_FILENO, "usage: ./%s team_name\n", PRG_NAME);
		return (EXIT_FAILURE);
	}
	g_lemipc.fd = -1;
	g_lemipc.size = 0;
	g_lemipc.addr = NULL;
	return (lemipc(argv[1]));
}
