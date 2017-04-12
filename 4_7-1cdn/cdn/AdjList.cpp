#include "AdjList.h"

AdjList::AdjList()
{
  for (int i = 0; i < nodeNumMax; i++) {
    index[i].id = -1;
    index[i].next = NULL;
  }
}

void AdjList::insert_edge(int x, int y, int c, int b)
{
  adjNode* tail = new adjNode;
  tail->id = y;
  tail->cap = c;
  tail->cost = b;
  tail->next = NULL;

  adjNode* temp = index[x].next;
  if (!temp) {
    //cout<<"in if"<<endl;
    index[x].next = tail;
    return;
  }
  adjNode* preTemp = temp;
  while (temp) {
    preTemp = temp;
    //cout<<"preTemp: "<<preTemp->id<<"temp: "<<temp->id<<endl;
    temp = temp->next;
    if(!temp){
      //cout<<"NULL"<<endl;
    }else{
      //cout<<"temp: "<<temp->id<<endl;
    }
  }//to find the position for new tail
  preTemp->next = tail;
}

void AdjList::delete_edge(int x, int y)
{
  adjNode* ptr = index[x].next;
  adjNode* prePtr = ptr;
  //cout<<"after init"<<endl;
  if(!ptr){
    cout<<"delete error edge1"<<endl;
    return;
  }
  if(!ptr->next)
  { //cout<<"delete 49"<<endl;
    if(ptr->id != y){
      cout<<"delete error edge2"<<endl;
      return;
    }
    index[x].next = NULL;
    free(ptr);
    return;
  }
  if (ptr->id == y) {
    //cout<<"find y"<<endl;
    index[x].next = ptr->next;
    free(ptr);
    return;
  }

  //cout<<"ptr->id "<<ptr->id<<". y: "<<y<<endl;

  while (ptr->id != y) {
    //cout<<"in while"<<endl;
    prePtr = ptr;
    ptr = ptr->next;
  }
  //cout<<"out while,prePtr: "<<prePtr->id<<" ptr: "<<ptr->id<<endl;
  prePtr->next = ptr->next;//666
  //cout<<"before free"<<endl;
  free(ptr);
  //cout<<"after free"<<endl;
  return;
}

int AdjList::get_cost(int x, int y)
{
  adjNode* ptr = index[x].next;
  while(ptr)
  {
    if (ptr->id == y) return ptr->cost;
    ptr = ptr->next;
  }
  cout<<"get_cost: wrong edge"<<endl;
  return -1;
}

int AdjList::get_incrmt(int x, int y)
{
  adjNode* ptr = index[x].next;
  while(ptr)
  {
    if (ptr->id == y) return ptr->incrmt;
    ptr = ptr->next;
  }
  cout<<"get_incrmt: wrong edge"<<endl;
  return -1;
}

void AdjList::update_flow(int x, int y, int minFlow)
{
  adjNode* temp1 = index[x].next;
  while(temp1)
  {
    if (temp1->id == y)
    {
      temp1->flow += minFlow;
      break;
    }
    temp1 = temp1->next;
  }//end while
  if(!temp1)
  {
    cout<<"update_flow1: wrong edge"<<endl;
    return;
  }

  adjNode*temp2 = index[y].next;
  while(temp2)
  {
    cout<<"temp: "<<temp2->id<<endl;
    if (temp2->id == x)
    {
      temp2->flow = 0 - temp1->flow;
      return;
    }
    temp2 = temp2->next;
  }//end while
  cout<<"update_flow2: wrong edge"<<endl;
}

void AdjList::change_cost(int x, int y, int c)
{
  adjNode* ptr = index[x].next;
  while (ptr)
  {
    if(ptr->id == y)
    {
      ptr->cost = c;
      return;
    }
    ptr = ptr->next;
  }//end while
  cout<<"change_cost: wrong edge x,y,c:"<<x<<" "<<y<<" "<<c<<endl;
}

void AdjList::update_flow_incrmt(int x, int y, int minFlow)
{
  adjNode* temp1 = index[x].next;
  while(temp1)
  {
    if (temp1->id == y)
    {
      temp1->flow += minFlow;
      break;
    }
    temp1 = temp1->next;
  }//end while
  if(!temp1)
  {
    cout<<"update_flow1: wrong edge"<<endl;
    return;
  }
  if (temp1->flow >= 0) {
    temp1->incrmt = temp1->cap - temp1->flow;
  }else{
    temp1->incrmt = 0 - temp1->flow;
  }

  adjNode*temp2 = index[y].next;
  while(temp2)
  {
    //cout<<"temp: "<<temp2->id<<endl;
    if (temp2->id == x)
    {
      temp2->flow = 0 - temp1->flow;
      return;
    }
    temp2 = temp2->next;
  }//end while
  if (temp2->flow >= 0) {
    temp2->incrmt = temp2->cap - temp2->flow;
  }else{
    temp2->incrmt = 0 - temp2->flow;
  }

  cout<<"update_flow2: wrong edge"<<endl;




}
