#ifndef __ROUTE_H__
#define __ROUTE_H__

#include "lib_io.h"
#include "AdjList.h"

#define INF 0x7ffffff


void deploy_server(char * graph[MAX_EDGE_NUM], int edge_num, char * filename);

void init_graph(char * graph[MAX_EDGE_NUM], int edge_num);

void init_graphList();

void update_resGraph();

int min_cost();

bool spfa(int x, int y);

int judge(int x); //return 1 if x > 0, else return -1

int ifpos(int x);

void testing();

void write_path();  //to write pathes to strPath

bool dfn_path(int v);  // to find path and return true once find one availavle path.

void SA();

void clear_flow(); //to clear flow of graph[][][];

void record_flow();  //to record flow of graph[][][] into bestFlow[][];

void change_server(int x);

void nla();

bool find_negative_circle(int v);

void BSA();

void add_server(int v);

void remove_server(int v);

void clear_all_server();

int aug(int v, int f);

bool modlabel();

int ZKW_min_cost();


#endif
