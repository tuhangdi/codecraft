#ifndef __AdjList_H__
#define __AdjList_H__

#include <stdlib.h>
#include <iostream>

#define nodeNumMax 1200

using namespace std;
struct adjNode{
  int id;
  int cap;
  int flow;
  int cost;
  int incrmt;
  adjNode* next;
};//adjecent
struct VIndex{
  int id;
  adjNode *next;
};//index

class AdjList{
public:
  VIndex index[nodeNumMax];
  AdjList();
  ~AdjList(){};
  void insert_edge(int x, int y, int c, int b);
  void delete_edge(int x, int y);
  int get_cost(int x, int y);
  int get_incrmt(int x, int y);
  void update_flow(int x, int y, int minFlow);
  void change_cost(int x, int y, int c);
  void update_flow_incrmt(int x, int y, int minFlow);
};

#endif
