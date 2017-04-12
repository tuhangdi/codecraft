#include "deploy.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <cstring>
#include <iostream>
#include <math.h>
#include <set>

using namespace std;

string res, strPath;	//res for outputing

int graph[nodeNumMax][nodeNumMax][3];	//0, Physical Network; 1, flows; 2,cost.
AdjList graphList;
int bestFlow[nodeNumMax][nodeNumMax];
int resGraph[nodeNumMax][nodeNumMax];	//Residual Network
int netnodeNum, edgeNum, consumerNum, totalNodeNum;;
int src, server;
int serverCost, consumerNeed;
bool ifChaos[nodeNumMax];
int pathNum, path[nodeNumMax/2];
bool readyWritePath = false, flagNlaMode = false;
time_t start,finish;

int costZKW;
bool visited[nodeNumMax];
int nodeMinCost[nodeNumMax];
int aug(int u, int f)
{ //enter aug
  if (u == server)
  {
    return f;
  }
  visited[u] = true;
  //int temp = f;
  adjNode* ptr = graphList.index[u].next;

  while (ptr) {
    int cap = ptr->incrmt, id = ptr->id;
    //cout<<"id :" << ptr->id<<endl;
    //cout << "cap :" << cap<<endl;
    if (cap && !visited[id] && nodeMinCost[u] == nodeMinCost[ptr->id] + ptr->cost) {
      int delta = aug(id, min(f, cap));
      //cout<<"delta: "<<delta<<endl;
      graphList.update_flow_incrmt(u, id, delta);
      //temp -= delta;
      if(delta != 0) return delta;
    }
    ptr = ptr->next;
  }
  return 0;
}

bool modlabel()
{// enter modlabel
  //cout<<"modlabel"<<endl;
  int delta = INF;
  adjNode* ptr = NULL;
  for (int  i = 0; i < totalNodeNum; i++)
  {//for 1
    if (visited[i])
    {
      ptr = graphList.index[i].next;
      while (ptr)
      {
        if (ptr->incrmt && !visited[ptr->id])
        {
          delta = min(delta, ptr->cost + nodeMinCost[ptr->id] - nodeMinCost[i]);
          //cout<<"70delta: "<<delta<<endl;
        }
        ptr = ptr->next;
      }//end while
    }//endif
  }//end for 1
  if (delta == INF) return false;
  for (int  i = 0; i < totalNodeNum; i++)
  {//for 2
    if (visited[i])
    {
      visited[i] = 0;
      nodeMinCost[i] += delta;
    }
  }//end for 2
  //costZKW += delta;
  return true;
}

int ZKW_min_cost()
{
  clear_flow();
  update_resGraph();
  int finishNeed = 0, ans = 0;
  memset(nodeMinCost, 0, sizeof(nodeMinCost));
  //cout<<"90before while"<<endl;
  while(1)
  {
    while (1) {
      memset(visited, 0, sizeof(visited));
      int temp = aug(src, INF);
      //cout<<"temp: "<<temp<<endl;
      if (!temp) break;
      finishNeed += temp;
      ans += temp * nodeMinCost[src];
      //cout<<"nodeMinCost[src]: "<<nodeMinCost[src]<<endl;
    }
    if (!modlabel()) {
      //cout<<"mod"<<endl;
      break;
    }
  }
  if (finishNeed != consumerNeed) {
    ans = INF;
  }
  //cout<<"ans: "<<ans<<endl;
  int numServer = 0;
  adjNode* ptrFindServer = graphList.index[server].next;
  while(ptrFindServer)
  {
    if(ptrFindServer->flow != 0) numServer++;
    ptrFindServer = ptrFindServer->next;

  }
  ans += numServer * serverCost;
  return ans;
}


void remove_server(int v)
{
  graphList.delete_edge(v, server);
  graphList.delete_edge(server, v);
}

void add_server(int v)
{
  graphList.insert_edge(v, server, INF, 0);
  graphList.insert_edge(server, v, INF, 0);
}

void clear_all_server()
{
  adjNode* ptr = graphList.index[server].next;
  adjNode* prePtr = ptr;
  while (ptr) {
    prePtr = ptr;
    ptr = ptr->next;
    graphList.delete_edge(prePtr->id, server);
    graphList.delete_edge(server, prePtr->id);
  }
}


bool cdn[nodeNumMax];
void BSA()//enter BSA
{
  memset(cdn, false, sizeof(cdn));
  bool flagOutput = false;
  bool flagBigSample = false;
  int runTimes = 0;
  int costNew = 0, costPre = 0, costBest = 0;
  double t0 = 100, alpha = 0.999;  //alpha: cooling down rate
  int k0 = 200, resetCondition = 50;
  if (netnodeNum > 299) {
    if (netnodeNum > 500) {
      t0 = 1;
      alpha = 1.008;
      k0 = 110;
      resetCondition = 12;
      flagBigSample = true;
    }else{
      t0 = 1;
      alpha = 1.008;
      k0 = 180;
      resetCondition = 45;
      //flagBigSample = true;
    }
  }


  set<int> newSolu;
  set<int> preSolu;
  set<int> bestSolu;
  set<int>::iterator it;
  srand((unsigned)time(NULL));

  //init new solution;
	adjNode* ptrServer = graphList.index[server].next;
	//adjNode* prePtrServer = NULL;
	//int tempSize = 0;
  int posServer = -1;
	while (ptrServer) {
    posServer = ptrServer->id;
    cdn[posServer] = true;
		newSolu.insert(posServer);
		ptrServer = ptrServer->next;
	}
	costPre = ZKW_min_cost();
  preSolu = newSolu;
  costBest = costPre;
  bestSolu = preSolu;

  int satisfaction = 0;
  int numSolu = 0;

  int  temp = 0;
  if (flagBigSample) {
    //while (bestSolu.size() > 130) {
      while(runTimes < 300){
      for (int l = 0; l < 1; l++) {
        if (newSolu.empty()) continue;
        temp = rand() % newSolu.size();
        it = newSolu.begin();
        for (int k = 0; k < temp; k++) it++;
        remove_server(*it);
        cdn[*it] = false;
        newSolu.erase(it);
      }
      costNew = ZKW_min_cost();
      runTimes++;

      if (costNew <= costPre) {
        costPre = costNew;
        preSolu = newSolu;
      }else{
        // if (double(rand()) / RAND_MAX < exp( - (costNew - costPre)/t0 ) ) {
        //   costPre = costNew;
        //   preSolu = newSolu;
        // }
        // else //did not accept
        // {
          clear_all_server();
          memset(cdn, false, sizeof(cdn));
          for (it = preSolu.begin(); it != preSolu.end(); it++) {
            add_server(*it);
            cdn[*it] = true;
          }
          newSolu = preSolu;
        // }
      }//end if (costNew < costPre)
      //cout<<"after accepting or not 2"<<endl;
      if (costPre < costBest) {
        costBest = costPre;
        bestSolu = preSolu;
      }
    }//end while
  }//end if bigSample




  while (1)//(t0 > 0.01)
  { runTimes++;
    satisfaction = 0;
    //cout<<"enter while cooling down"<<endl;
    //to get a new solution
    /*clear_all_server();
    memset(cdn, false, sizeof(cdn));
    newSolu.clear();

    int sizeBest = bestSolu.size();
    for (int i = 0; i < sizeBest; i++) {
      posServer = rand() % netnodeNum;
      if (!cdn[posServer]) {
        add_server(posServer);
        newSolu.insert(posServer);
        cdn[posServer] = true;
      }
    }*/
    //cout<<"after init"<<endl;
    int forTimes = 0;
    forTimes = bestSolu.size()/11;
    for (int i = 0; i < forTimes; i++) {
      if (newSolu.empty()) continue;
      temp = rand() % newSolu.size();
      it = newSolu.begin();
      for (int k = 0; k < temp; k++) it++;
      remove_server(*it);
      cdn[*it] = false;
      newSolu.erase(it);

      posServer = rand() % netnodeNum;
      while(cdn[posServer])
      {
        posServer = rand() % netnodeNum;
      }
      add_server(posServer);
      cdn[posServer] = true;
      newSolu.insert(posServer);
    }

    costNew = ZKW_min_cost();
    if (costNew < costPre) {
      costPre = costNew;
      preSolu = newSolu;
      if (costNew <= costBest) {
        costBest = costNew;
        bestSolu = newSolu;
      }
    }else
    {
      if (double(rand()) / RAND_MAX < exp( - (costNew - costPre)/t0 ) ) {
        costPre = costNew;
        preSolu = newSolu;
      }
      else //did not accept
      {
        clear_all_server();
        memset(cdn, false, sizeof(cdn));
        for (it = bestSolu.begin(); it != bestSolu.end(); it++) {
          add_server(*it);
          cdn[*it] = true;
        }
        newSolu = bestSolu;
      }
    }
    //cout<<"after accepting or not"<<endl;


    for (int j = 0; j < k0; j++) {// inner
      //cout<<"enter inner for"<<endl;
      //newSolu = preSolu;

      if (flagBigSample) {
        numSolu = rand() % 2 + 1;
      }else
      {
        numSolu = rand() % 4;
      }
      //cout<<"numSolu: "<<numSolu<<endl;
      switch (numSolu) {
        case 0:
          if (newSolu.empty()) continue;
          temp = rand() % newSolu.size();
          it = newSolu.begin();
          for (int k = 0; k < temp; k++) it++;
          remove_server(*it);
          cdn[*it] = false;
          newSolu.erase(it);

          posServer = rand() % netnodeNum;
          while(cdn[posServer])
          {
            posServer = rand() % netnodeNum;
          }
          add_server(posServer);
          cdn[posServer] = true;
          newSolu.insert(posServer);
          break;
        case 1:
          posServer = rand() % netnodeNum;
          while(cdn[posServer])
          {
            posServer = rand() % netnodeNum;
          }
          add_server(posServer);
          cdn[posServer] = true;
          newSolu.insert(posServer);
          break;
        //case 2:
        case 2:
          if (newSolu.empty()) continue;
          temp = rand() % newSolu.size();
          it = newSolu.begin();
          for (int k = 0; k < temp; k++) it++;
          remove_server(*it);
          cdn[*it] = false;
          newSolu.erase(it);
          break;
        case 3:
          if (newSolu.empty()) continue;
          temp = rand() % newSolu.size();
          it = newSolu.begin();
          for (int k = 0; k < temp; k++) it++;
          remove_server(*it);
          cdn[*it] = false;
          newSolu.erase(it);
          break;
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
          if (newSolu.empty()) continue;
          temp = rand() % newSolu.size();
          it = newSolu.begin();
          for (int k = 0; k < temp; k++) it++;
          remove_server(*it);
          cdn[*it] = false;
          newSolu.erase(it);

          posServer = rand() % netnodeNum;
          while(cdn[posServer])
          {
            posServer = rand() % netnodeNum;
          }
          add_server(posServer);
          cdn[posServer] = true;
          newSolu.insert(posServer);

          break;
        //case 4:
        //case 5:
        // case 4:
        //   temp = rand() % newSolu.size();
        //   it = newSolu.begin();
        //   for (int k = 0; k < temp; k++) it++;
        //   remove_server(*it);
        //   cdn[*it] = false;
        //   newSolu.erase(it);

          break;

      }//switch
      //cout<<"after switch, size:"<<newSolu.size()<<endl;

      costNew = ZKW_min_cost();
      runTimes++;

      if (costNew <= costPre) {
        costPre = costNew;
        preSolu = newSolu;
      }else{
        if (double(rand()) / RAND_MAX < exp( - (costNew - costPre)/t0 ) ) {
          costPre = costNew;
          preSolu = newSolu;
        }
        else //did not accept
        {
          clear_all_server();
          memset(cdn, false, sizeof(cdn));
          for (it = preSolu.begin(); it != preSolu.end(); it++) {
            add_server(*it);
            cdn[*it] = true;
          }
          newSolu = preSolu;
        }
      }//end if (costNew < costPre)
      //cout<<"after accepting or not 2"<<endl;

      if (costPre >= costBest) {
        satisfaction++;
        }else{
        costBest = costPre;
        bestSolu = preSolu;
        satisfaction = 0;
      }

      //cout<<"t0:"<<t0<<cout<<": costBest: "<< costBest<<"bestSolusize: "<<bestSolu.size()<<endl;
      //shanwo
      if (satisfaction > resetCondition) {
        //t0 *= alpha;  //cooling down
        break;
      }

      finish = clock();
  		if ((double)(finish - start)/CLOCKS_PER_SEC > 86.0)
      {
        flagOutput = true;
        break;
      }
    }// end for

    if(flagOutput) break;
    t0 *= alpha; //
  }//cooling down: while (t0 > 0.1)


  clear_all_server();
  memset(cdn, false, sizeof(cdn));
  cout<<"runTimes: "<<runTimes<<endl;
  cout<<"cdn: ";
  for (it = bestSolu.begin(); it != bestSolu.end(); it++) {
    cout<<*it<<" ";
    add_server(*it);
  }
  cout<<endl;

  int costOutput = ZKW_min_cost();
  cout<<"costOutput: "<<costOutput<<endl;
  record_flow();

}




int circlePoint, circleCost, preServer, afterServer, tempFlow = INF;
bool flagFindCircle = false, flagServerCircle = false, flagRemoveServer = false;
int pre[nodeNumMax];

bool find_negative_circle(int v)
{
  cout<<"v: "<<v<<endl;
  if (flagFindCircle) return true;

  finish = clock();
	if(finish - start >= 85000000) return false;

  adjNode* ptrAdj = graphList.index[v].next;
  while(ptrAdj)
  {
    if (ptrAdj->incrmt <= 0)
    {
      ptrAdj = ptrAdj->next;
      continue;
    }
    int i = ptrAdj->id;
    //cout<<"line 49, i: "<<i<<endl;
    if (!visited[i])
    {
      visited[i] = true;
      pre[i] = v;
      find_negative_circle(i);
      if (flagFindCircle)
      {
        cout<<"line 55: find circle"<<endl;
        return true;
      }
      visited[i] = false;
      ptrAdj = ptrAdj->next;
    }else if (i != pre[v])
    {//find one circle
      if (i == server)
      {
        flagServerCircle = true;
        preServer = v;
      }

      circleCost = 0;
      circleCost += graphList.get_cost(v,i);
      tempFlow = min(tempFlow, graphList.get_incrmt(v,i));
      int temp = v;
      while (pre[temp] != i)
      {
        circleCost += graphList.get_cost(pre[temp],temp);
        tempFlow = min(tempFlow, graphList.get_incrmt(pre[temp],temp));
        temp = pre[temp];
      }
      // the last edge of the circle
      if (flagServerCircle) afterServer = temp;
      circleCost += graphList.get_cost(pre[temp],temp);
      tempFlow = min(tempFlow, graphList.get_incrmt(pre[temp],temp));
      int tempCost = circleCost;
      if (flagServerCircle && tempFlow == graphList.get_incrmt(server, afterServer))
      {
          tempCost -= serverCost;
          flagRemoveServer = true;
      }

      if (tempCost < 0) {
        circlePoint = i;
        pre[i] = v;
        flagFindCircle = true;
        return true;
      }

    }//end if

    ptrAdj = ptrAdj->next;
  }//end while
  return false;

}//end find_negative_circle

void nla(int * totalCost)
{ //enter nla, negative loop algorithm
  cout<<"enter nla"<<endl;
  update_resGraph();
  memset(visited, false, sizeof(visited));
	memset(pre, -1, sizeof(pre));

  visited[server] = true;

  while (find_negative_circle(server))
  {//to update the negative circle by increment(minFlow)
    cout<<"find NCircle"<<endl;
    finish = clock();
  	if(finish - start >= 85000000) break;
    cout<<"circlePoint: "<<circlePoint<<endl;
    int temp = circlePoint;
    int minFlow = tempFlow;
    cout<<"pre[temp]: "<<pre[temp]<<",temp:"<<temp<<endl;
    graphList.update_flow(pre[temp], temp, minFlow);
    temp = pre[temp];
    while (temp != circlePoint) {
      cout<<"pre[temp]: "<<pre[temp]<<",temp:"<<temp<<endl;
      graphList.update_flow(pre[temp], temp, minFlow);
      temp = pre[temp];
    }
    *totalCost += circleCost * minFlow;
    if(flagRemoveServer)
    {
      *totalCost -= serverCost;
      graphList.change_cost(afterServer, server, serverCost);
    }
    cout<<"updated totalCost: "<<*totalCost<<endl;
    //ready for the next round;
    flagFindCircle = false;
    flagServerCircle = false;
    flagRemoveServer = false;
    tempFlow = INF;
    update_resGraph();
    memset(visited, false, sizeof(visited));
		visited[server] = true;
		memset(pre, -1, sizeof(pre));

  }
}

void change_server(int x)
{
  int xsize = 0;
  int candi = -1;

  //to get the new candidate
  adjNode* ptr = graphList.index[x].next;
  if(!ptr)
  {
    cout<<"x is not a server point"<<endl;
    return;
  }
  if(!ptr->next) {
    candi = ptr->id;
  }else{
    while(ptr)
    {
      xsize++;
      ptr = ptr->next;
    }
    candi = rand()% xsize + 1;
  }
  //update server
  graphList.delete_edge(x, server);
  graphList.delete_edge(server, x);
  graphList.insert_edge(candi, server, INF, 0);
  graphList.insert_edge(server, candi, INF, 0);

}

void record_flow()	//enter record_flow();
{
	adjNode* ptrAdj = NULL;
	for (int i = 0; i < totalNodeNum; i++) {
		ptrAdj = graphList.index[i].next;
		if(!ptrAdj) continue;
		while(ptrAdj)
		{
			bestFlow[i][ptrAdj->id] = ptrAdj->flow;
			ptrAdj = ptrAdj->next;
		}
	}
}

void clear_flow()	//enter clear_flow();
{
	adjNode* ptrAdj = NULL;
	for (int i = 0; i < totalNodeNum; i++) {
		ptrAdj = graphList.index[i].next;
		if(!ptrAdj) continue;
		while(ptrAdj)
		{
			ptrAdj->flow = 0;
			ptrAdj = ptrAdj->next;
		}
	}
}

void SA()//enter SA
{
	cout<<"sa"<<endl;
	int posServer = 0;
	int costNow = 0, costBest = 0;
	int k0 = 1000000;

	set<int> newSolu;
	set<int> oriSolu;
	set<int>::iterator it;
	srand((unsigned)time(NULL));
	//init new solution;

	adjNode* ptrServer = graphList.index[server].next;
	//adjNode* prePtrServer = NULL;
	int tempSize = 0;

	while (ptrServer) {
		newSolu.insert(ptrServer->id);
		ptrServer = ptrServer->next;
	}

	costBest = min_cost();
	record_flow();


	oriSolu = newSolu;
 	bool clockFlag = false;
	int costFlag = 0;
	int nodeSize = newSolu.size();

	for (int i = 0; i < k0 ; i++)
	{

		int p = rand() % 100;
		if(costFlag > nodeSize && costFlag < 1.5*nodeSize)
		{
			p = rand() % 60;
		}
		else if(costFlag >= 1.5*nodeSize )
		{
			p = rand() % 38;
		}

		if(p >=0 && p < 30)//change
		{
			if (newSolu.size() == 0) break;
			//int sizeSolu = rand() % (newSolu.size() /40 + 1) + 1;
			int sizeSolu = 1;
			for(int i = 0; i < sizeSolu; i ++)
				{
					int temp = rand()%newSolu.size();
					for (it=newSolu.begin();it!=newSolu.end();it++)
					{
						if(temp == 0)
						{
							graphList.delete_edge(*it, server);
							graphList.delete_edge(server, *it);
							newSolu.erase(*it);
							break;
						}
						temp--;
					}
				}
			for (int k = 0; k < sizeSolu; k++) {

				posServer = rand() % netnodeNum;
				tempSize = newSolu.size();
				newSolu.insert(posServer);
				if(newSolu.size() == tempSize) continue;

				graphList.insert_edge(posServer, server, INF, 0);
				graphList.insert_edge(server, posServer, INF, 0);
			}
		}

		if(p>=30 && p < 35)//add
		{
			//int temp = rand() % (newSolu.size()/30 + 1) + 1;
			int temp = 1;
			for (int k = 0; k < temp; k++) {
				posServer = rand() % netnodeNum;
				tempSize = newSolu.size();
				newSolu.insert(posServer);
				if(newSolu.size() == tempSize) continue;

				graphList.insert_edge(posServer, server, INF, 0);
				graphList.insert_edge(server, posServer, INF, 0);

			}
		}

		if(p >=35 && p < 40)	//delete
		{
			if (newSolu.size() == 0) break;
			//int sizeSolu = rand() % (newSolu.size() /40 + 2) + 1;
			int sizeSolu = 1;
			for(int i = 0; i < sizeSolu; i ++)
			{
				int temp = rand()%newSolu.size();
				for (it=newSolu.begin();it!=newSolu.end();it++)
				{
					if(temp == 0)	//if(temp == 0)???
					{
						graphList.delete_edge(*it, server);
						graphList.delete_edge(server, *it);
						newSolu.erase(*it);
						break;
					}
					temp--;
				}
			}
		}

		if(p>=40 && p < 45)//add
		{
			//int temp = rand() % (newSolu.size()/40 + 2) + 1;
			int temp = 1;
			for (int k = 0; k < temp; k++) {
				posServer = rand() % netnodeNum;
				tempSize = newSolu.size();
				newSolu.insert(posServer);
				if(newSolu.size() == tempSize) continue;

				graphList.insert_edge(posServer, server, INF, 0);
				graphList.insert_edge(server, posServer, INF, 0);
			}
		}

		if(p >=45)	//delete
		{
			if (newSolu.size() == 0) break;
			//int sizeSolu = rand() % (newSolu.size() /100 + 1) + 1;
			int sizeSolu = 1;
			for(int i = 0; i < sizeSolu; i ++)
			{
				int temp = rand()%newSolu.size();
				for (it=newSolu.begin();it!=newSolu.end();it++)
				{
					if(temp == 0)
					{
						graphList.delete_edge(*it, server);
						graphList.delete_edge(server, *it);
						newSolu.erase(*it);
						break;
					}
					temp--;
				}
			}
		}

		costNow = min_cost();

		cout<<"costBest: "<< costBest<<"newnodesize: "<<newSolu.size()<<endl;


		if (costNow < costBest)
		{
			//cout<<"164"<<endl;
			costBest = costNow;
		 	oriSolu = newSolu;
			record_flow();

		}
		else
		{	//cout<<"171"<<endl;
			int size = newSolu.size();
			for (int k = 0; k < size; k++) {
				graphList.delete_edge(*newSolu.begin(), server);
				graphList.delete_edge(server, *newSolu.begin());
				newSolu.erase(*newSolu.begin());
			}

			newSolu = oriSolu;
			for (int k = 0; k <  oriSolu.size(); k++) {

				graphList.insert_edge(*newSolu.begin(), server, INF, 0);
				graphList.insert_edge(server, *newSolu.begin(), INF, 0);
				newSolu.erase(*newSolu.begin());
			}
			newSolu = oriSolu;
			//cout<<"line 187"<<endl;
		}


		finish = clock();

		if ((double)(finish - start)/CLOCKS_PER_SEC > 86.0){
			break;
		}


	}	//k0 for

	cout<<"sa end, costBest: "<<costBest<<"oriSolu"<<oriSolu.size()<<endl;
}



bool dfs_path(int v) //??????
{
	if (v == server) return true;
	for (int i = 0; i < totalNodeNum; i++)
	{
		if (bestFlow[v][i]>0 && !visited[i])
		{
			visited[i] = true;
			pre[i] = v;
			dfs_path(i);
		}
	}
	if (visited[server] == false) return false;
}

void write_path() {	//enter write_path
	int ptrPath = 0;	//a pointer used for finding path
	char a[20];

	memset(visited, false, sizeof(visited));
	while (dfs_path(src))
	{
		pathNum++;
		ptrPath = 0; //init ptrPath
		memset(path, -1, sizeof(path));	//init path
		int minFlow = INF;

		int temp = server;
		while (temp != src)	//to find minFlow in bestFlow[][]
		{
			minFlow = min(minFlow, bestFlow[pre[temp]][temp]);
			temp = pre[temp];
			path[ptrPath++] = temp;
		}
		temp = server;
		while (src != temp)	//update bestFlow
		{
			bestFlow[pre[temp]][temp] -= minFlow;
			temp = pre[temp];
		}

		ptrPath -= 2;
		for (int i = 0; i < ptrPath; i++)	// to record this path
		{
			sprintf(a, "%d ", path[i]);
			strPath += a;
		}
		sprintf(a, "%d ", path[ptrPath] - netnodeNum);	//restore the value of real sonsumer node and write it.
		strPath += a;
		sprintf(a, "%d\n", minFlow);	//daikuan capacity
		strPath += a;

		memset(visited, false, sizeof(visited));
	}
}


int judge(int x)
{
	if (x >= 0) return 1;
	return -1;
}

int ifpos(int x)
{
	if (x > 0) return 1;
	return 0;
}

bool spfa(int x, int y)
{	//enter spfa

	queue<int> chaos;
	memset(ifChaos, false, sizeof(ifChaos));
	for (int i = 0; i < totalNodeNum; i++) nodeMinCost[i] = INF;

	nodeMinCost[x] = 0;
	chaos.push(x);
	ifChaos[x] = true;

	while (!chaos.empty())
	{
		int temp = chaos.front();
		chaos.pop();

		ifChaos[temp] = false;
		adjNode* ptrAdj = graphList.index[temp].next;
		while(ptrAdj)
		{
			if (ptrAdj->incrmt > 0 && nodeMinCost[ptrAdj->id] > nodeMinCost[temp] + ptrAdj->cost)
			{

				nodeMinCost[ptrAdj->id] = nodeMinCost[temp] + ptrAdj->cost;
				pre[ptrAdj->id] = temp;
				if (!ifChaos[ptrAdj->id])
				{
					chaos.push(ptrAdj->id);
					ifChaos[ptrAdj->id] = true;
				}
			}
			ptrAdj = ptrAdj->next;
		}//while(ptrAdj != NULL)

	}//while (!chaos.empty())


	if (nodeMinCost[y] == INF) return false;
	return true;
}

int min_cost()	//enter min_cost
{

	clear_flow();
	update_resGraph();
	int cost = 0;
	int finishNeed = 0; //the amount of meeting demand of consumer nodes.

	while (spfa(src, server))// i changed to src
	{
		int minFlow = INF;
		int vj = server;
		int vi = pre[vj];
		while (src != vj)	//to find minFlow (max availavle flow)
		{
			adjNode* temp = graphList.index[vi].next;
			while(temp->id != vj) {temp = temp->next;}
			minFlow = min(minFlow, temp->incrmt);

			vj = vi;
			vi = pre[vj];
		}// finding minFlow finished.
		finishNeed += minFlow;
		cost += minFlow * nodeMinCost[server];
		vj = server;
		vi = pre[vj];
		while (src != vj)	//update flow
		{
			adjNode* temp1 = graphList.index[vi].next;
			while(temp1->id != vj) {temp1 = temp1->next;}
			temp1->flow += minFlow;
			adjNode* temp2 = graphList.index[vj].next;
			while(temp2->id != vi) {temp2 = temp2->next;}
			temp2->flow = 0 - temp1->flow;
			vj = vi;
			vi = pre[vj];
		}
		update_resGraph();	//updating resGraph once the flows changes
	}//while (spfa(src, server))

	int numServer = 0;
	adjNode* ptrFindServer = graphList.index[server].next;
	while(ptrFindServer)
	{
    if(ptrFindServer->flow != 0) numServer++;
    ptrFindServer = ptrFindServer->next;

	}
	cost += numServer * serverCost;

	if (consumerNeed != finishNeed)
	{
		return INF;
	}
	return cost;


}

void init_graph(char * topo[MAX_EDGE_NUM], int line_num)
{ //enter init_graph
	char *c;
	int spaceCount = 0;

	//to read netnodeNum, edgeNum, consumerNum
	c = topo[0];
	while (*c != '\0' && *c != '\n' && *c != '\r')
	{
		if (*c == ' ')
		{
			c++;
			spaceCount++;
			continue;
		}
		switch (spaceCount)
		{
		case 0:
			{netnodeNum = *c - '0' + netnodeNum * 10;
			 break;
			}
		case 1:
			{edgeNum = *c - '0' + edgeNum * 10;
			 break;
			}
		case 2:
			{consumerNum = *c - '0' + consumerNum * 10;
			 break;
			}
		default:
			break;
		}
		c++;
	}

	totalNodeNum = netnodeNum + consumerNum + 2;

	c = topo[2];	//to read serverCost
	while (*c != '\0' && *c != '\n' && *c != '\r')
	{
		serverCost = *c - '0' + serverCost * 10;
		c++;
	}

	//initial graph
	int cap = 0, cost = 0, x = 0, y = 0;	//capacity
	for (int i = 4; i < 4+edgeNum; i++)
	{
		c = topo[i];	//read one line data
		spaceCount = cap = cost = x = y = 0;	//refresh recorders
		while (*c != '\0' && *c != '\n' && *c != '\r')
		{
			if (*c == ' ')
			{
				c++;
				spaceCount++;
				continue;
			}
			switch (spaceCount)
			{
				case 0:
				{x = *c - '0' + x * 10;
					break;
				}
				case 1:
				{y = *c - '0' + y * 10;
					 break;
				}
				case 2:
				{cap = *c - '0' + cap * 10;
					 break;
				}
				case 3:
				{cost = *c - '0' + cost * 10;
					break;
				}
				default:
					break;
			}
			c++;
		}
		graph[x][y][0] = graph[y][x][0] = cap;
		graph[x][y][2] = graph[y][x][2] = cost;
		//graphList.insert_edge(x,y,cap,cost);
	}

	//initial consumerNode
	src = totalNodeNum - 2;
	server = totalNodeNum - 1;
	for (int i = 1; i < consumerNum+1; i++)
	{
		c = topo[line_num-i];
		spaceCount = cap = cost = x = y = 0;
		while (*c != '\0' && *c != '\n' && *c != '\r')
		{
			if (*c == ' ')
			{
				c++;
				spaceCount++;
				continue;
			}
			switch (spaceCount)
			{
			case 1:
				{y = *c - '0' + y * 10;
				 break;
				}
			case 2:
				{cap = *c - '0' + cap * 10;
				 break;
				}
			default:
				break;
			}
			c++;
		}
		x = consumerNum - i + netnodeNum;
   	graph[x][y][0] = graph[y][x][0] = graph[x][src][0] = graph[src][x][0] = cap; //init src
		consumerNeed += cap;	//init consumerNeed
		graph[x][y][2] = graph[y][x][2] = 0;
		graph[y][server][0] = graph[server][y][0] = INF;
	}//for consumerNode

  //re-init server for nla
  // if (netnodeNum < 500) {
  //   flagNlaMode = true;
  // }//closed nla function

  // if (flagNlaMode) {//closed nla function
  //   for (int i = 0; i < netnodeNum; i++)
  //   {
  //     if (!graph[i][server][0]) {	//!!!significant
  //       graph[i][server][0] = graph[server][i][0] = INF;
  //       graph[i][server][2] = serverCost;
  //     }
  //   }//end for
  // }//end if

	init_graphList();
}

void init_graphList()
{
	for (int i = 0; i < totalNodeNum; i++) {
		for (int j = 0; j < totalNodeNum; j++) {
			if (graph[i][j][0]) {
				graphList.insert_edge(i,j,graph[i][j][0],graph[i][j][2]);
			}
		}
	}
}


void update_resGraph()
{	//enter update_resGraph
	for(int i = 0; i < totalNodeNum; i++)
	{
		adjNode* ptrAdj = graphList.index[i].next;
		while (ptrAdj)
		{
			if (ptrAdj->flow >= 0) {
				ptrAdj->incrmt = ptrAdj->cap - ptrAdj->flow;
			}else{
				ptrAdj->incrmt = 0 - ptrAdj->flow;
			}
			ptrAdj = ptrAdj->next;
		}
	}
  // adjNode* ptrAdj = graphList.index[server].next;
  // while (ptrAdj) {
  //   ptrAdj->incrmt =  0 - ptrAdj->flow;
  //   ptrAdj = ptrAdj->next;
  // }
}

void deploy_server(char * topo[MAX_EDGE_NUM], int line_num,char * filename)
{

	//initial the network

	start = clock();
	init_graph(topo, line_num);
  //cout<<"line 743"<<endl;
	//updating Residual Network
	update_resGraph();
  // int totalCost = 0;
  // cout<<"line 746"<<endl;
  // if (!flagNlaMode) {
  //   cout<<"line 748"<<endl;
  //   SA();
  // }else{
  //   cout<<"line 751"<<endl;
  //   totalCost = min_cost();
  //   cout<<totalCost<<endl;
  //   cout<<"line 753"<<endl;
  //   nla(&totalCost);
  //   record_flow();
  // }
  // cout<<totalCost<<endl;

  BSA();

  //int i = ZKW_min_cost(); record_flow();

//  cout<<"result: "<<i<<endl;

	write_path();

	char b[20];
	sprintf(b, "%d\n\n", pathNum);
	res = b;
	res += strPath;

	char * topo_file = (char *)res.c_str();
	//char * topo_file = (char *)strGraph.c_str(); //output the graph

	write_result(topo_file, filename);

}
