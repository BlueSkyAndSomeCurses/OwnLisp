all: parsing.c
	cc -std=c99 -Wall parsing.c mpc.c -ledit -g -lm -o parsing
	./parsing

bonus: bonus.c
	cc -std=c99 -Wall bonus.c mpc.c -ledit -g -lm -o bonus
	# ./bonus
