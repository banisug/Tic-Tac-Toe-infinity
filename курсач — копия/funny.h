#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <time.h>


void fill_board(int s);
int enter_player();
void print_board();
int winner(char p);
int full(int s);
void make_move(int m, char p);
void cancel_move(int m);
int count_win_lines();
int minimax(int depth, int maximizing_player, int max_depth, int size);
int best_move(int max_depth, int size);
void easy_level();