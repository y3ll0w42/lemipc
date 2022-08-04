#include "lemipc.h"

int		play_game(struct ipc *ipc)
{
	show_game(ipc);
	sleep(100);
	return (EXIT_SUCCESS);
}

int		setup_chatbox(struct ipc *ipc)
{
	size_t chatbox_size = (CHAT_HEIGHT - 1) * ((WIDTH * 2) - 1) * sizeof(uint8_t);
	ipc->chatbox = malloc(chatbox_size);	
	if (!ipc->chatbox)
	{
		dprintf(STDERR_FILENO, "%s: malloc(): %s\n", PRG_NAME, strerror(errno));
		shmdt(ipc->game);
		return (EXIT_FAILURE);
	}
	memset(ipc->chatbox, ' ', chatbox_size);
	return (EXIT_SUCCESS);
}

int		create_game(struct ipc *ipc)
{
	ipc->game = shmat(ipc->shm_id, NULL, 0);
	ipc->game->nb_players = 0;
	memset(ipc->game->players, -1, sizeof(ipc->game->players));
	memset(ipc->game->map, ' ', sizeof(ipc->game->map));
	ipc->game->player_turn.team = 0;
	ipc->game->player_turn.pos_x = -1;
	ipc->game->player_turn.pos_y = -1;
	return (EXIT_SUCCESS);
}

int		join_game(struct ipc *ipc)
{
	struct game		*game;
	struct player	*player;
	pid_t			pid;
	time_t			t;

	player = &ipc->player;
	game = ipc->game;
	pid = getpid();
	sem_lock(ipc->sem_id);
	size_t i = 0;
	while (i < MAX_PLAYERS && game->players[i] != -1)
		i++;
	if (i == MAX_PLAYERS)
	{
		sem_unlock(ipc->sem_id);
		dprintf(STDERR_FILENO, "Game is full\n");
		return (EXIT_FAILURE);
	}
	game->nb_players++;
	game->players[i] = pid;
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	srand((unsigned) time(&t));
	do {
		player->pos_x = rand() % WIDTH;
		player->pos_y = rand() % HEIGHT;
	}
	while (game->map[player->pos_y][player->pos_x] != ' ');
	game->map[player->pos_y][player->pos_x] = player->team + '0';
	sem_unlock(ipc->sem_id);
	return (play_game(ipc));
}

int		exit_game(struct ipc *ipc)
{
	sem_lock(ipc->sem_id);
	if (ipc->game->nb_players)
		ipc->game->nb_players--;
	ipc->game->map[ipc->player.pos_y][ipc->player.pos_x] = ' ';
	if (!ipc->game->nb_players)
	{
		shmdt(ipc->game);
		semctl(ipc->sem_id, IPC_RMID, 0);
		shmctl(ipc->shm_id, IPC_RMID, 0);
		return (EXIT_SUCCESS);
	}
	shmdt(ipc->game);
	sem_unlock(ipc->sem_id);
	free(ipc->chatbox);
	return (EXIT_SUCCESS);
}
