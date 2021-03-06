#include "deploy.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <cstring>
#include <iostream>
#include <math.h>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <bitset>
#include <random>
#include <vector>
#include <unordered_map>

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
bool readyWritePath = false;
time_t start,finish;

int costZKW;
int nodeMinCost[nodeNumMax];
bool visited[nodeNumMax];
unordered_set<int> btSolu;

unordered_map<string, int> geneList;
unordered_set<int> gaSolu;
int costGaBest = 0;
constexpr int MAXGENS = 20000;
constexpr int NVARS = 500;
constexpr int POPSIZE = 50;
double PCROSSOVER = 0.75;
double PMUTATION = 0.005;
int GENER = 0;
static default_random_engine e((unsigned)time(NULL));


struct GeneType {
	bitset<NVARS> gene;
	int fitness;
	double rfitness;
	double cfitness;

	GeneType() {}

	GeneType &operator=(const GeneType &rhs) {
		gene = rhs.gene;
		fitness = rhs.fitness;
		return *this;
	}
};

int aug(int u, int f)
{ //enter aug
  //cout<<"aug"<<endl;
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
      //    cout<<"70delta: "<<delta<<endl;
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
  //cout<<"zkw"<<endl;
  clear_flow();
  update_resGraph();
  int finishNeed = 0, ans = 0;
  memset(nodeMinCost, 0, sizeof(nodeMinCost));
//  cout<<"90before while"<<endl;
  while(1)
  {
    while (1) {
      memset(visited, 0, sizeof(visited));
      int temp = aug(src, INF);
    //  cout<<"temp: "<<temp<<endl;
      if (!temp) break;
      finishNeed += temp;
      ans += temp * nodeMinCost[src];
  //    cout<<"nodeMinCost[src]: "<<nodeMinCost[src]<<endl;
    }
    if (!modlabel()) {
    //  cout<<"mod"<<endl;
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




void elitist(vector<GeneType> &population) {
	int best = population[0].fitness;
	int worst = population[0].fitness;
	int best_idx = 0, worst_idx = 0;

	for (int i = 1; i < POPSIZE; ++i) {
		if (population[i].fitness > best) {
			best = population[i].fitness;
			best_idx = i;
		}

		if (worst > population[i].fitness) {
			worst = population[i].fitness;
			worst_idx = i;
		}
	}

	if (best > population[POPSIZE].fitness) {
		population[POPSIZE] = population[best_idx];
		population[POPSIZE].fitness = population[best_idx].fitness;
	}
	else {
		population[worst_idx] = population[POPSIZE];
		population[worst_idx].fitness = population[POPSIZE].fitness;
	}
}


void initialize(vector<GeneType> &population) {
	static uniform_int_distribution<int> u(0, netnodeNum - 1);
	static uniform_real_distribution<double> ud(0, 1);

	// population[0].fitness = 0;
	// population[0].rfitness = 0.0;
	// population[0].cfitness = 0.0;
	// adjNode* ptrServer = graphList.index[server].next;
	// while (ptrServer) {
	// 	gaSolu.insert(ptrServer->id);
	// 	population[0].gene.set(ptrServer->id);
	// 	ptrServer = ptrServer->next;
	// }

	double p = (double)consumerNum / (netnodeNum);
	for (int i = 0; i < POPSIZE; ++i) {

		int cnt = 0;
		population[i].fitness = 0;
		population[i].rfitness = 0.0;
		population[i].cfitness = 0.0;
		//int randPop= rand() % (consumerNum / 2) + 1;
		//population[i].gene = population[0].gene;
		for (int k = 0; k < netnodeNum; ++k) {
			if ( ud(e) < p) {
				population[i].gene.set(k);
			}
		}
	}

	population[POPSIZE].fitness = 0;
}

void evaluate(vector<GeneType> &population) {
	for (int i = 0; i < POPSIZE; ++i) {


		if (geneList.find(population[i].gene.to_string()) != geneList.end()){
			population[i].fitness = geneList[population[i].gene.to_string()];
		}
		else{

			string oriSolu = population[i].gene.to_string();
			int q = 0;
			for (int k = oriSolu.size() - 1; k >= NVARS - netnodeNum; --k) {
				if (oriSolu[k] == '1') {
					graphList.insert_edge(q, server, INF, 0);
					graphList.insert_edge(server, q, INF, 0);
					gaSolu.insert(q);
					btSolu = gaSolu;
				}
				q++;
			}

			population[i].fitness = INF - ZKW_min_cost();

			if (population[i].fitness > costGaBest) {
				costGaBest = population[i].fitness;
				record_flow();
			}

			geneList[population[i].gene.to_string()] = population[i].fitness;

			int size = gaSolu.size();
			for (int k = 0; k < size; k++) {
				graphList.delete_edge(*gaSolu.begin(), server);
				graphList.delete_edge(server, *gaSolu.begin());
				gaSolu.erase(*gaSolu.begin());
			}


		}


		//if (population[i].fitness == 0) population[i].fitness = INF/2;
		// cout << "evaluate " << i << " fitness " << population[i].fitness << endl;




		// adjNode* ptrServer = graphList.index[server].next;
		// while (ptrServer) {
		// 	gaSolu.insert(ptrServer->id);
		// 	// cout<<"line129 id: "<< ptrServer->id<<endl;
		// 	ptrServer = ptrServer->next;
		// }




	}
}

void select(vector<GeneType> &population) {
	static uniform_real_distribution<double> u(0, 1);

	vector<GeneType> newpopulation(population.size());

	long long sum = 0;
	for (int i = 0; i < POPSIZE; ++i) {
		sum =  sum + population[i].fitness;
	}

	for (int i = 0; i < POPSIZE; ++i) {
		population[i].rfitness = (long double)population[i].fitness / sum;
	}

	population[0].cfitness = population[0].rfitness;
	for (int i = 1; i < POPSIZE; ++i) {
		population[i].cfitness =
			population[i - 1].cfitness + population[i].rfitness;
	}

	for (int i = 0; i < POPSIZE; ++i) {
		double p = u(e);
		if (p < population[0].cfitness) {
			newpopulation[i] = population[0];
		}
		else {
			for (int j = 0; j < POPSIZE; ++j) {
				if (population[j].cfitness <= p && p < population[j + 1].cfitness) {
					newpopulation[i] = population[j + 1];
				}
			}
		}
	}

	for (int i = 0; i < POPSIZE; ++i) {
		population[i].fitness = newpopulation[i].fitness;
	}
}

void single_point_crossover(GeneType &p1, GeneType &p2) {
	static uniform_int_distribution<int> u(0, netnodeNum - 1);

	for (int i = u(e); i < netnodeNum; ++i) {
		bool t = p1.gene[i];
		p1.gene[i] = p2.gene[i];
		p2.gene[i] = t;
	}
}

void double_point_crossover(GeneType &p1, GeneType &p2) {
	static uniform_int_distribution<int> u(0, netnodeNum - 1);
	int i = u(e), j = u(e);
	for (int k = min(i, j); k < max(i, j); ++k) {
		bool t = p1.gene[k];
		p1.gene[k] = p2.gene[k];
		p2.gene[k] = t;
	}
}

void crossover(vector<GeneType> &population) {
	static uniform_real_distribution<double> u(0, 1);

	int first = 0;
	int one = 0;
	PCROSSOVER = 0.75 - (0.5*GENER/MAXGENS);
	for (int i = 0; i < POPSIZE; ++i) {
		if (u(e) < PCROSSOVER) {
			++first;

			if (first % 2 == 0) {
				single_point_crossover(population[one], population[i]);
			}
			else {
				one = i;
			}
		}
	}
}

void mutate(vector<GeneType> &population) {
	static uniform_real_distribution<double> ud(0, 1);
	static uniform_int_distribution<int> ui(0, netnodeNum - 1);
	static uniform_int_distribution<int> upop(0, POPSIZE - 1);
	for (int i = 0; i < POPSIZE; ++i) {
		int cnt = 0;
		auto &gene = population[i].gene;
		int k = ui(e);
		int randnum =  1;
		//int randnum = 1;

		PMUTATION = 0.001 + (0.3 - 0.001) * GENER / MAXGENS;
		for (int k = 0; k < netnodeNum; ++k) {
			if (ud(e) < PMUTATION) {
				gene.flip(k);
				cnt++;
				break;
			}
		// }
		// if (ud(e) < PMUTATION) {
		// 	population[i].gene.flip(ui(e));
		}
	}
}
// }

void GA() {
	vector<GeneType> population(POPSIZE + 1);

	initialize(population);

	evaluate(population);

	elitist(population);

	// cout << "gene: " << endl;
	// for (int i = 0; i < POPSIZE; ++i) {
	// 	 cout << population[i].gene.to_string() << endl;
	// }
	// cout << endl;

	int generation;
	int best_fitness = 0;
	int cnt = 0;
	for (generation = 0; generation < MAXGENS; ++generation) {
		GENER = generation;
		select(population);

		// cout << "select: " << endl;
		// for (int i = 0; i < POPSIZE; ++i) {
		// 	 cout << population[i].gene.to_string() << endl;
		// }
		// cout << endl;

		crossover(population);
		//
		// cout << "crossover: " << endl;
		// for (int i = 0; i < POPSIZE; ++i) {
		// 	 cout << population[i].gene.count() << endl;
		// }
		// cout << endl;

		mutate(population);
		//
		// cout << "mutate: " << endl;
		// for (int i = 0; i < POPSIZE; ++i) {
		// 	 cout << population[i].gene.count() << endl;
		// }
		// cout << endl;

		evaluate(population);
		elitist(population);

		// cout << "elitist: " << endl;
		// for (int i = 0; i < POPSIZE; ++i) {
		// 	 cout << INF- population[i].fitness << endl;
		// }
		// cout << endl;
		 cout << "generation: " << generation << " best fitness: " << INF - population[POPSIZE].fitness << endl;
		// //cout << population[POPSIZE].gene.to_string() << endl;
		if(best_fitness == INF - population[POPSIZE].fitness){
			cnt++;
		}
		else{
			cnt = 0;
			best_fitness = INF - population[POPSIZE].fitness;
		}
		if (cnt > 200){SA();}
		finish = clock();
		if ((double)(finish - start)/CLOCKS_PER_SEC > 86.0){
			break;
		}
	}
	cout<<"costGaBest"<<INF - costGaBest;
}

int  change_server(int x)
{
  int xsize = 0;
  int candi = -1;

  //to get the new candidate
  adjNode* ptr = graphList.index[x].next;
  if(!ptr)
  {
    cout<<"x is not a server point"<<endl;
    return -1;
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
	return candi;
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
	int costNow = 0, costBest = 0, costBestPre = 0;
	int k0 = 500000;

	unordered_set<int> newSolu;
	unordered_set<int> oriSolu;
	unordered_set<int> interSet;
	unordered_set<int>::iterator it;
	unordered_set<int>::iterator iter;
	srand((unsigned)time(NULL));

	// adjNode* ptrServer = graphList.index[server].next;
	// int tempSize = 0;
	//
	// while (ptrServer) {
	// 	newSolu.insert(ptrServer->id);
	// 	ptrServer = ptrServer->next;
	// }


	int size = gaSolu.size();
	for (int k = 0; k < size; k++) {
		graphList.delete_edge(*gaSolu.begin(), server);
		graphList.delete_edge(server, *gaSolu.begin());
		newSolu.erase(*gaSolu.begin());
	}

	newSolu = btSolu;
	for (int k = 0; k <  btSolu.size(); k++) {

		graphList.insert_edge(*newSolu.begin(), server, INF, 0);
		graphList.insert_edge(server, *newSolu.begin(), INF, 0);
		newSolu.erase(*newSolu.begin());
	}
	newSolu = btSolu;

	costBest = ZKW_min_cost();
	record_flow();
	costBestPre = costBest;

	oriSolu = newSolu;
	set_intersection(newSolu.begin(),newSolu.end(),oriSolu.begin(),oriSolu.end(),inserter(interSet, interSet.begin()));
 	bool clockFlag = false;
	int costFlag = 0;
	int nodeSize = newSolu.size();
	int tempSize = 0;
	int p = 0;

	for (int i = 0; i < k0 ; i++)
	{
		//cout<<"k0 : "<<i<<endl;

		if(nodeSize > 300){
			if(costFlag < 180)
				p = rand() % 65 + 20 ;
			else
			{
				p = rand() % 35;
				//cout<<"bbbbb"<<endl;
			}

		}
		else
		{
			p = rand() % 100;
		}
		// if(costFlag > 800 )
		// {
		// 	//cout<<"ccccccc"<<endl;
		// 	p = rand() % 35;
		// }

		if(p >=0 && p < 25)//change
		{
			if (newSolu.size() == 0) break;
			//int sizeSolu = rand() % (newSolu.size() /40 + 3) + 1;
			//int sizeSolu = rand() % 5 + 1;
			int sizeSolu = 1;
			for(int i = 0; i < sizeSolu; i ++)
				{
					int temp = rand()%newSolu.size();
					for (it=newSolu.begin();it!=newSolu.end();it++)
					{
						if(temp <= 0)
						{
							iter  = interSet.find(*it);
							if(iter == interSet.end() && interSet.size() != 0) continue;
							else
							{
								graphList.delete_edge(*it, server);
								graphList.delete_edge(server, *it);
								newSolu.erase(*it);
								break;
							}
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

		else if(p >=25 && p < 30)	//delete
		{
			if (newSolu.size() == 0) break;
			//int sizeSolu = rand() % (newSolu.size() /100 + 2) + 1;
			//int sizeSolu = rand() % 2 + 1;
			int sizeSolu = 1;
			for(int i = 0; i < sizeSolu; i ++)
			{
				int temp = rand()%newSolu.size();
				for (it=newSolu.begin();it!=newSolu.end();it++)
				{
					if(temp <= 0)
					{
						iter  = interSet.find(*it);
						if(iter == interSet.end()) continue;
						else
						{
							graphList.delete_edge(*it, server);
							graphList.delete_edge(server, *it);
							newSolu.erase(*it);
							break;
						}
					}
					temp--;
				}
			}
		}

		else if(p>=30 && p < 40)//add
		{
			//int temp = rand() % (newSolu.size()/30 + 1) + 1;
			int temp = 1;
			for (int k = 0; k < 20; k++) {
				posServer = rand() % netnodeNum;
				tempSize = newSolu.size();
				newSolu.insert(posServer);
				if(newSolu.size() == tempSize) continue;
				else
				{
					graphList.insert_edge(posServer, server, INF, 0);
					graphList.insert_edge(server, posServer, INF, 0);
					break;
        }
			}
		}

		else if(p >= 40 && p < 85)	//delete
		{
			if (newSolu.size() == 0) break;
			//int sizeSolu = rand() % (newSolu.size() /100 + 1) + 1;
			//int sizeSolu = rand() % 5 + 1;
			int sizeSolu = 1;
			for(int i = 0; i < sizeSolu; i ++)
			{
				int temp = rand()%newSolu.size();
				for (it=newSolu.begin();it!=newSolu.end();it++)
				{
					if(temp <= 0)
					{
						iter  = interSet.find(*it);
						if(iter == interSet.end() && interSet.size() != 0) continue;
						else
						{
							graphList.delete_edge(*it, server);
							graphList.delete_edge(server, *it);
							newSolu.erase(*it);
							break;
						}
					}
					temp--;
				}
			}
		}

		else if(p >= 85)//add
		{
			//int temp = rand() % (newSolu.size()/40 + 2) + 1;
			int temp = 1;
			for (int k = 0; k < 20; k++) {
				posServer = rand() % netnodeNum;
				tempSize = newSolu.size();
				newSolu.insert(posServer);
				if(newSolu.size() == tempSize) continue;
				else
				{
					graphList.insert_edge(posServer, server, INF, 0);
					graphList.insert_edge(server, posServer, INF, 0);
					break;
				}
			}
		}

		costNow = ZKW_min_cost();

		cout<<"costBest: "<< costBest<<"newnodesize: "<<newSolu.size()<<endl;


		if (costNow < costBest)
		{
			set_intersection(newSolu.begin(),newSolu.end(),oriSolu.begin(),oriSolu.end(),inserter(interSet, interSet.begin()));
			costBest = costNow;
		 	oriSolu = newSolu;
			record_flow();
		}
		else
		{
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
		}

		if(costBestPre == costBest)
		{
			costFlag++;
		}

		costBestPre = costBest;

		finish = clock();

		if ((double)(finish - start)/CLOCKS_PER_SEC > 86.0){
			break;
		}

	}	//k0 for

	//cout<<"sa end, costBest: "<<costBest<<"oriSolu"<<oriSolu.size()<<endl;
}


int pre[nodeNumMax];
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




void init_graph(char * topo[MAX_EDGE_NUM], int line_num)	//enter init_graph
{
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
		//graph[y][server][0] = graph[server][y][0] = INF;


	}
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
}

void deploy_server(char * topo[MAX_EDGE_NUM], int line_num,char * filename)
{

	//initial the network

	start = clock();
	init_graph(topo, line_num);

	//updating Residual Network
	update_resGraph();

	//SA();
	GA();

	write_path();

	char b[20];
	sprintf(b, "%d\n\n", pathNum);
	res = b;
	res += strPath;

	char * topo_file = (char *)res.c_str();
	//char * topo_file = (char *)strGraph.c_str(); //output the graph

	write_result(topo_file, filename);

}
