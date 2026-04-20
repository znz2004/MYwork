#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <tuple>
#include <queue>
#include <sstream>
#include <fstream>

using namespace std::chrono;
using namespace std;

const int TILE_SIZE = 32;							// 每个地块的大小
const int MAP_SIZE = 20;							// 地图大小 (m × m)
const int SCREEN_WIDTH = TILE_SIZE * MAP_SIZE;
const int SCREEN_HEIGHT = TILE_SIZE * MAP_SIZE;
const int MAX_CHARACTER_CAN_GET_RESOUCE_NUM = 75;
const int MAX_STORAGE_NUM = 75;
const int TREE_GENERATION_INTERVAL = 500;			// 树生成间隔时间（单位：帧）
const int BUILD_WALL_NEED_WOOD_NUM = 5;				// 建造墙需要木材数目
const int BUILD_WALL_NEED_TIME = 60;				// 建造墙需要时间(单位：秒)
const int MOVE_ONE_TITLE_TIME = 1;					// 移动一个格子需要时间(单位：秒)
const int LOGGIND_TIME = 5;							// 伐木动作需要300帧（约5秒）


// 方向数组，分别表示上、下、左、右的移动
const int dx[] = { -1, 1, 0, 0 };
const int dy[] = { 0, 0, -1, 1 };


enum RESOURCE_TYPE
{
	RT_NONE = 0,
	RT_TREE,			//树
	RT_WOOD,			//木材
	RT_STORAGE,			//存储区
	RT_BULE_WALL,		//墙蓝图
	RT_WALL,			//墙
};


enum CHARACTER_STATUS
{
	CS_STOP = 0,		//无可能操作动作，原地不动
	CS_MOVE_STORAGE,	//移动去存储区取材料
	CS_MOVE_BULE_WALL,  //拿到材料移动到墙蓝图
	CS_MOVE_LOGGIND,	//移动去伐木
	CS_LOGGING,			//伐木
	CS_BUILD_WALL,		//建造墙
	CS_HANDING,			//搬运
};


// 地块结构
struct Tile
{
	Tile()
	{
		Reset();
	}

	void Reset()
	{
		x = 0;
		y = 0;
		resourceType = RT_NONE;
		cutDown = false;
		resourceFilled = 0;
		resourceNum = 0;
		host_index = 0;
	}

	int x, y;				// 坐标
	int resourceType;		// 资源类型 (见RESOURCE_TYPE)
	bool cutDown;			// 伐木标记
	int resourceFilled;		// 墙蓝图已填充木材的数目
	int resourceNum;		// 存储区的资源数目

	int host_index;			//占有它的玩家
};


// 角色结构
struct Character 
{
	Character(int pindex)
	{
		index = pindex;
		x = rand() % MAP_SIZE;
		y = rand() % MAP_SIZE;
		status = CS_STOP;
		status_end_time = 0;
		resource_num = 0;
		path.clear();
		begin_move_one_title_time = 0;
		path_index = 0;
		build_wall_x = 0;
		build_wall_y = 0;
	}

	int index;						
	int x, y;						// 角色的当前坐标
	int status;						// 角色状态
	int status_end_time;			// 状态结束时间
	int resource_num;				// 角色携带的资源数量
	vector<pair<int, int>> path;	// 移动路线
	int begin_move_one_title_time;	
	int path_index;

	int build_wall_x;
	int build_wall_y;
};

std::vector<std::vector<Tile>> tileMap;

// 判断位置是否合法
bool isValid(int x, int y)
{
	return x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE;
}

bool LoadMapFile();
// 初始化地图
void initializeTileMap()
{
	for (int i = 0; i < MAP_SIZE; ++i)
	{
		std::vector<Tile> row;
		for (int j = 0; j < MAP_SIZE; ++j)
		{
			Tile tile;
			tile.Reset();
			tile.x = i;
			tile.y = j;
			row.push_back(tile);
		}
		tileMap.push_back(row);
	}

	if (LoadMapFile())
	{
		return;
	}

	// 随机生成初始树
	// 初始树的数量
	int initialTrees = 5; 
	for (int i = 0; i < initialTrees; ++i)
	{
		int x = rand() % MAP_SIZE;
		int y = rand() % MAP_SIZE;
		tileMap[x][y].resourceType = RT_TREE; // 设置为树
	}
}

void generateTree()
{
	bool is_can_generate_tree = false;
	for (size_t i = 0; i < tileMap.size(); ++i)
	{
		for (size_t j = 0; j < tileMap[i].size(); ++j)
		{
			if (tileMap[i][j].resourceType == RT_NONE)
			{
				is_can_generate_tree = true;
			}
		}
	}

	if (!is_can_generate_tree)
	{
		return;
	}

	int x = 0, y = 0;
	do
	{
		x = rand() % MAP_SIZE;
		y = rand() % MAP_SIZE;
	} while (tileMap[x][y].resourceType != RT_NONE); // 找到一个空闲地块
	tileMap[x][y].resourceType = RT_TREE;			 // 在该地块生成树
}


// 位图字体数据，仅支持数字（0-9）
const char* numberFont[6] = {
	" ***   *   ***** ***** *   * ***** ***** ***** ***** ***** ",
	"*   * **    *      *   *   *     * *       *   * *   * *   *",
	"*   *  *    ***   ***  *****   **  *****   ***  ***** ***** ",
	"*   *  *    *      *      *  *      *   *     *     *     * ",
	" ***  ***   ***** *****     * ***** *****   *   ***** ***** "
};

// 渲染单个数字
void RenderDigit(SDL_Renderer* renderer, char digit, int x, int y, int scale)
{
	if (digit < '0' || digit > '9') return; // 只处理数字
	int index = digit - '0';

	for (int row = 0; row < 5; ++row)
	{
		for (int col = 0; col < 5; ++col)
		{
			if (numberFont[row][index * 6 + col] == '*')
			{
				SDL_Rect rect = { x + col * scale, y + row * scale, scale, scale };
				SDL_RenderFillRect(renderer, &rect);
			}
		}
	}
}

// 渲染整个数字字符串
void RenderNumber(SDL_Renderer* renderer, const std::string& number, int x, int y, int scale)
{
	int xOffset = 0;
	for (char digit : number)
	{
		if (digit >= '0' && digit <= '9')
		{
			RenderDigit(renderer, digit, x + xOffset, y, scale);
		}

		xOffset += 6 * scale; // 每个数字宽度为5，间隔1
	}
}


// 绘制地图
void renderTileMap(SDL_Renderer* renderer, SDL_Texture *bg_texture, SDL_Texture *bule_wall_texture,
	SDL_Texture *tree_texture, SDL_Texture *wall_texture, SDL_Texture *wood_texture, SDL_Texture *red_tree_texture, SDL_Texture *storage_texture)
{
	SDL_Rect bg_rect = { 0, 0,  TILE_SIZE * MAP_SIZE,  TILE_SIZE * MAP_SIZE };
	SDL_RenderCopy(renderer, bg_texture, NULL, &bg_rect);

	for(size_t i = 0; i < tileMap.size(); ++i)
	{
		for (size_t j = 0; j < tileMap[i].size(); ++j)
		{
			SDL_Rect rect = { tileMap[i][j].y * TILE_SIZE, tileMap[i][j].x * TILE_SIZE, TILE_SIZE, TILE_SIZE };
			if (tileMap[i][j].resourceType == RT_TREE)
			{
				if (tileMap[i][j].cutDown)
				{
					SDL_RenderCopy(renderer, red_tree_texture, NULL, &rect);
				}
				else
				{
					SDL_RenderCopy(renderer, tree_texture, NULL, &rect);
				}
			}
			else if (tileMap[i][j].resourceType == RT_WOOD)
			{
				SDL_RenderCopy(renderer, wood_texture, NULL, &rect);
			}
			else if (tileMap[i][j].resourceType == RT_STORAGE)
			{
				SDL_RenderCopy(renderer, storage_texture, NULL, &rect);
				RenderNumber(renderer, to_string(tileMap[i][j].resourceNum), tileMap[i][j].y * TILE_SIZE, tileMap[i][j].x * TILE_SIZE, 2);
			}
			else if (tileMap[i][j].resourceType == RT_BULE_WALL)
			{
				SDL_RenderCopy(renderer, bule_wall_texture, NULL, &rect);
			}
			else if (tileMap[i][j].resourceType == RT_WALL)
			{
				SDL_RenderCopy(renderer, wall_texture, NULL, &rect);			
			}
			else
			{
				//SDL_RenderCopy(renderer, bg_texture, NULL, &rect);
			}
		}
	}
}

// 绘制角色
void renderCharacter(SDL_Renderer* renderer, const Character& character, SDL_Texture *role_texture)
{
	SDL_Rect rect = { character.y * TILE_SIZE, character.x * TILE_SIZE, TILE_SIZE, TILE_SIZE };
	SDL_RenderCopy(renderer, role_texture, NULL, &rect);
}


// 创建储存区
void createStorageArea(int startX, int startY, int endX, int endY) 
{
	// 查找并合并重叠的储存区
	for (int i = startX; i <= endX; ++i) 
	{
		for (int j = startY; j <= endY; ++j) 
		{
			if (!isValid(i, j))
			{
				continue;
			}

			// 如果该位置已为储存区，则扩展已有储存区
			if (tileMap[i][j].resourceType == RT_STORAGE) 
			{
				// 合并操作：更新新区域的起始和结束坐标
				startX = std::min(startX, i);
				startY = std::min(startY, j);
				endX = std::max(endX, i);
				endY = std::max(endY, j);
			}
		}
	}

	// 更新储存区
	for (int i = startX; i <= endX; ++i) 
	{
		for (int j = startY; j <= endY; ++j)
		{
			if (!isValid(i, j))
			{
				continue;
			}

			if (tileMap[i][j].resourceType == RT_NONE)
			{
				tileMap[i][j].resourceType = RT_STORAGE; // 标记为储存区
			}	
		}
	}
}

// 伐木命令执行
void cutDownTrees(int startX, int startY, int endX, int endY)
{
	for (int i = startX; i <= endX; ++i)
	{
		for (int j = startY; j <= endY; ++j)
		{
			if (!isValid(i, j))
			{
				continue;
			}

			if (tileMap[i][j].resourceType == RT_TREE && !tileMap[i][j].cutDown)
			{
				tileMap[i][j].cutDown = true; // 伐木标记
			}
		}
	}
}

bool RealCutDownTrees(Character &character, int time_now)
{
	if (character.status != CS_LOGGING)
	{
		return false; 
	}

	if (character.status_end_time <= time_now)
	{
		if (!isValid(character.x, character.y))
		{
			return false;
		}

		tileMap[character.x][character.y].resourceType = RT_WOOD;
		tileMap[character.x][character.y].resourceNum += 55;
		tileMap[character.x][character.y].cutDown = false;
		tileMap[character.x][character.y].host_index = 0;
		character.status_end_time = 0;
		character.status = CS_STOP;

		//printf("RealCutDownTrees111 %d %d %d\n", character.x, character.y, tileMap[character.x][character.y].resourceNum);
		return true;
	}

	return false;
}

bool buildBuleWall(int x, int y)
{
	if (!isValid(x, y))
	{
		return false;
	}

	if (tileMap[x][y].resourceType == RT_NONE)
	{
		tileMap[x][y].resourceType = RT_BULE_WALL; // 设置地块为墙
		return true;
	}

	return false; // 无法建造墙
}


bool BuildWall(Character &character, int time_now)
{
	if (character.status == CS_BUILD_WALL)
	{
		if (time_now - character.status_end_time >= BUILD_WALL_NEED_TIME)
		{
			if (isValid(character.x, character.y) && tileMap[character.x][character.y].resourceType == RT_BULE_WALL)
			{
				tileMap[character.x][character.y].resourceType = RT_WALL;
				tileMap[character.x][character.y].resourceFilled = 0;
				character.status_end_time = 0;
				character.status = CS_STOP;
				character.build_wall_x = 0;
				character.build_wall_y = 0;
				return true;
			}
		}

		character.build_wall_x = 0;
		character.build_wall_y = 0;
		character.status_end_time = 0;
		character.status = CS_STOP;
		return false;
	}

	if (!isValid(character.x, character.y))
	{
		return false;
	}

	if (tileMap[character.x][character.y].resourceType != RT_BULE_WALL)
	{
		return false;
	}

	if (tileMap[character.x][character.y].resourceFilled < BUILD_WALL_NEED_WOOD_NUM)
	{
		return false;
	}

	character.status = CS_BUILD_WALL;
	character.status_end_time = time_now;
	return true;
}


// 广度优先搜索（BFS），找到从start到end的路径
vector<pair<int, int>> bfs(int startX, int startY, int endx, int endy)
{
	queue<pair<int, int>> q;
	vector<vector<bool>> visited(MAP_SIZE, vector<bool>(MAP_SIZE, false));
	vector<vector<pair<int, int>>> parent(MAP_SIZE, vector<pair<int, int>>(MAP_SIZE, { -1, -1 }));

	q.push({ startX, startY });
	visited[startX][startY] = true;

	while (!q.empty())
	{
		pair<int, int> start = q.front();
		q.pop();

		if (start.first == endx && start.second == endy)
		{
			vector<pair<int, int>> path;
			pair<int, int> cur = start;
			while (cur != make_pair(startX, startY))
			{
				path.push_back(cur);
				cur = parent[cur.first][cur.second];
			}

			path.push_back({ startX, startY });
			reverse(path.begin(), path.end());
			return path;
		}

		// 探索四个方向
		for (int i = 0; i < 4; i++) 
		{
			int nx = start.first + dx[i];
			int ny = start.second + dy[i];

			if (isValid(nx, ny) && tileMap[nx][ny].resourceType != RT_WALL && !visited[nx][ny])
			{
				visited[nx][ny] = true;
				parent[nx][ny] = { start.first, start.second };
				q.push({ nx, ny });
			}
		}
	}

	return {};
}


bool FindBuleWall(Character &character, int time_now)
{
	if (character.status != CS_STOP)
	{
		return false;
	}

	for (int i = 0; i < MAP_SIZE; ++i)
	{
		for (int j = 0; j < MAP_SIZE; ++j)
		{
			if (tileMap[i][j].resourceType == RT_BULE_WALL)
			{
				if (tileMap[i][j].resourceFilled < BUILD_WALL_NEED_WOOD_NUM)
				{
					for (int k = 0; k < MAP_SIZE; ++k)
					{
						for (int l = 0; l < MAP_SIZE; ++l)
						{
							if (tileMap[k][l].resourceType == RT_STORAGE)
							{
								vector<pair<int, int>> path = bfs(character.x, character.y, k, l);
								character.path = path;
								character.status = CS_MOVE_STORAGE;
								character.status_end_time = (int)path.size() * MOVE_ONE_TITLE_TIME + time_now;
								character.path_index = 0;
								character.begin_move_one_title_time = time_now;
								character.build_wall_x = i;
								character.build_wall_y = j;

								//printf("FindBuleWall %d %d %d %d\n", character.x, character.y, k, l);
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}


bool Move(Character &character, int time_now)
{
	if (character.status != CS_MOVE_STORAGE && character.status != CS_MOVE_BULE_WALL && character.status != CS_MOVE_LOGGIND && character.status != CS_HANDING)
	{
		return false;
	}

	if (character.path_index < 0)
	{
		return false;
	}

	if (character.path_index >= (int)character.path.size() - 1)
	{
		//printf("Move End %d %d %d\n", character.x, character.y, character.status);
		if (character.status == CS_MOVE_STORAGE)
		{
			int can_get_resouce_num = MAX_CHARACTER_CAN_GET_RESOUCE_NUM - character.resource_num;
			if (can_get_resouce_num > 0)
			{
				if (isValid(character.x, character.y))
				{
					if (tileMap[character.x][character.y].resourceNum > can_get_resouce_num)
					{
						character.resource_num += can_get_resouce_num;
						tileMap[character.x][character.y].resourceNum -= can_get_resouce_num;
					}
					else
					{
						character.resource_num += tileMap[character.x][character.y].resourceNum;
						tileMap[character.x][character.y].resourceNum = 0;
					}
				}	
			}

			vector<pair<int, int>> path = bfs(character.x, character.y, character.build_wall_x, character.build_wall_y);
			character.path.clear();
			character.path = path;
			character.path_index = 0;
			character.begin_move_one_title_time = time_now;
			character.status = CS_MOVE_BULE_WALL;
			character.status_end_time = time_now + (int)path.size() * MOVE_ONE_TITLE_TIME;
			return true;
		}
		else if (character.status == CS_MOVE_BULE_WALL)
		{
			if (isValid(character.x, character.y))
			{
				tileMap[character.x][character.y].resourceFilled += character.resource_num;
				character.path.clear();
				character.path_index = 0;
				character.begin_move_one_title_time = 0;
				character.status = CS_STOP;
				character.status_end_time = 0;
				BuildWall(character, time_now);
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (character.status == CS_MOVE_LOGGIND)
		{
			character.path.clear();
			character.path_index = 0;
			character.begin_move_one_title_time = 0;
			character.status = CS_LOGGING;
			character.status_end_time = time_now + LOGGIND_TIME;
			return true;
		}
		else if (character.status == CS_HANDING)
		{
			character.path.clear();
			character.path_index = 0;
			character.begin_move_one_title_time = 0;
			character.status = CS_STOP;
			character.status_end_time = 0;

			if (isValid(character.x, character.y))
			{
				int can_put_in_storage_num = MAX_STORAGE_NUM - tileMap[character.x][character.y].resourceNum;
				if (can_put_in_storage_num > 0)
				{

					if (can_put_in_storage_num > character.resource_num)
					{
						tileMap[character.x][character.y].resourceNum += character.resource_num;
						character.resource_num = 0;
					}
					else
					{
						tileMap[character.x][character.y].resourceNum += can_put_in_storage_num;
						character.resource_num -= can_put_in_storage_num;
					}
				}
			}
			
			return true;
		}

		return false;
	}

	if (character.begin_move_one_title_time + MOVE_ONE_TITLE_TIME <= time_now)
	{
		if (character.path_index + 1 >= 0 && character.path_index + 1 < (int)character.path.size())
		{
			character.x = character.path[character.path_index + 1].first;
			character.y = character.path[character.path_index + 1].second;
			character.begin_move_one_title_time = time_now;
			character.path_index += 1;
			return true;
			//printf("Move11 %d %d\n", character.x, character.y);
		}
	}

	return false;
}


bool Handing(Character &character, int time_now)
{
	if (character.status != CS_STOP)
	{
		return false;
	}

	if (!isValid(character.x, character.y))
	{
		return false;
	}

	if (tileMap[character.x][character.y].resourceType != RT_WOOD)
	{
		return false;
	}

	if (tileMap[character.x][character.y].host_index != 0)
	{
		return false;
	}

	bool is_can_handing = false;
	int storage_x = 0, storage_y = 0;
	for (int i = 0; i < MAP_SIZE; ++i)
	{
		for (int j = 0; j < MAP_SIZE; ++j)
		{
			if (tileMap[i][j].resourceType == RT_STORAGE && tileMap[i][j].resourceNum < MAX_STORAGE_NUM)
			{
				is_can_handing = true;
				storage_x = i;
				storage_y = j;
				break;
			}
		}

		if (is_can_handing)
		{
			break;
		}
	}

	if (!is_can_handing)
	{
		return false;
	}

	vector<pair<int, int>> path = bfs(character.x, character.y, storage_x, storage_y);
	character.status = CS_HANDING;
	character.status_end_time = (int)path.size() * MOVE_ONE_TITLE_TIME + time_now;
	character.resource_num += tileMap[character.x][character.y].resourceNum;
	character.path = path;
	character.path_index = 0;
	character.begin_move_one_title_time = time_now;
	tileMap[character.x][character.y].resourceNum = 0;
	tileMap[character.x][character.y].resourceType = RT_NONE;
	tileMap[character.x][character.y].host_index = character.index;
	//printf("Handing %d %d %d %d\n", character.x, character.y, tileMap[character.x][character.y].resourceNum, character.resource_num);
	return true;
}


void CheckCanLoging(Character &character, int time_now)
{
	if (character.status != CS_STOP)
	{
		return;
	}

	bool is_can_logging = false;
	int _x = 0, _y = 0;
	for (int i = 0; i < MAP_SIZE; ++i)
	{
		for (int j = 0; j < MAP_SIZE; ++j)
		{
			if (tileMap[i][j].resourceType == RT_TREE && tileMap[i][j].cutDown && tileMap[i][j].host_index == 0)
			{
				_x = i;
				_y = j;
				is_can_logging = true;
				break;
			}
		}

		if (is_can_logging)
		{
			break;
		}
	}

	if (!is_can_logging)
	{
		return;
	}

	vector<pair<int, int>> path = bfs(character.x, character.y, _x, _y);
	character.status = CS_MOVE_LOGGIND;
	character.status_end_time = (int)path.size() * MOVE_ONE_TITLE_TIME + time_now;
	character.path = path;
	character.path_index = 0;
	character.begin_move_one_title_time = time_now;
	tileMap[_x][_y].host_index = character.index;

	//printf("CheckCanLoging111 %d %d %d %d\n", character.x, character.y, _x, _y);
	return;
}


void UpdateCharacter(Character &character)
{
	int time_now = (int)duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
	CheckCanLoging(character, time_now);
	if (Move(character, time_now))
	{
		return;
	}

	if (RealCutDownTrees(character, time_now))
	{
		return;
	}

	if (Handing(character, time_now))
	{
		return;
	}
	
	if (FindBuleWall(character, time_now))
	{
		return;
	}
	
	if (BuildWall(character, time_now))
	{
		return;
	}
}


void SaveMapToFile()
{
	ofstream outMapFile("./tilemap.txt", ios::out);
	if (!outMapFile)
	{
		return;
	}

	for (size_t i = 0; i < tileMap.size(); ++i)
	{
		for (size_t j = 0; j < tileMap.size(); ++j)
		{
			int is_cut_down = tileMap[i][j].cutDown ? 1 : 0;
			outMapFile << tileMap[i][j].x << "," << tileMap[i][j].y << "," << tileMap[i][j].resourceType << "," << is_cut_down
				<< "," << tileMap[i][j].resourceFilled << "," << tileMap[i][j].resourceNum << "," << tileMap[i][j].host_index << endl;
		}
	}
	
	outMapFile.close();
	return;
}


bool LoadMapFile()
{
	std::ifstream inputFile("./tilemap.txt");
	if (!inputFile.is_open()) 
	{
		return false;
	}

	string line;
	while (std::getline(inputFile, line)) 
	{
		stringstream ss(line);
		string x, y, resourceType, is_cut_down, resourceFilled, resourceNum, host_index;
		std::getline(ss, x, ',');
		std::getline(ss, y, ',');
		std::getline(ss, resourceType, ',');
		std::getline(ss, is_cut_down, ',');
		std::getline(ss, resourceFilled, ',');
		std::getline(ss, resourceNum, ',');
		std::getline(ss, host_index, ',');

		Tile tile;
		tile.x = std::stoi(x);
		tile.y = std::stoi(y);
		tile.resourceType = std::stoi(resourceType);
		tile.cutDown = std::stoi(is_cut_down) == 1 ? true : false;
		tile.resourceFilled = std::stoi(resourceFilled);
		tile.resourceNum = std::stoi(resourceNum);
		tile.host_index = std::stoi(host_index);
		if (isValid(tile.x, tile.y))
		{
			tileMap[tile.x][tile.y] = tile;
		}
		else
		{
			return false;
		}
	}

	inputFile.close();
	return true;
}


bool LoadCharacterFile(Character &character)
{
	char c_filename[128];
	snprintf(c_filename, 128, "character_%d.txt", character.index);
	string filename(c_filename);

	std::ifstream inputFile(filename.c_str());
	if (!inputFile.is_open())
	{
		return false;
	}

	string line;
	if (!std::getline(inputFile, line))
	{
		return false;
	}

	string x, y, status, status_end_time, resource_num, begin_move_one_title_time, path_index, build_wall_x, build_wall_y, path_cout;
	stringstream ss(line);
	std::getline(ss, x, ',');
	std::getline(ss, y, ',');
	std::getline(ss, status, ',');
	std::getline(ss, status_end_time, ',');
	std::getline(ss, resource_num, ',');
	std::getline(ss, begin_move_one_title_time, ',');
	std::getline(ss, path_index, ',');
	std::getline(ss, build_wall_x, ',');
	std::getline(ss, build_wall_y, ',');
	std::getline(ss, path_cout, ',');

	character.x = std::stoi(x);
	character.y = std::stoi(y);
	character.status = std::stoi(status);
	character.status_end_time = std::stoi(status_end_time);
	character.resource_num = std::stoi(resource_num);
	character.begin_move_one_title_time = std::stoi(begin_move_one_title_time);
	character.path_index = std::stoi(path_index);
	character.build_wall_x = std::stoi(build_wall_x);
	character.build_wall_y = std::stoi(build_wall_y);
	int int_path_cout = std::stoi(path_cout);
	for (int i = 0; i < int_path_cout; ++i)
	{
		string path_x, path_y;
		std::getline(ss, path_x, ',');
		std::getline(ss, path_y, ',');
		pair<int, int> path;
		path.first = std::stoi(path_x);
		path.second = std::stoi(path_y);
		character.path.push_back(path);
	}

	inputFile.close();
	return true;
}


void SaveCharacterToFile(Character &character, int index)
{
	char c_filename[128];
	snprintf(c_filename, 128, "character_%d.txt", index);
	string filename(c_filename);

	ofstream outCharacterFile(filename.c_str(), ios::out);
	if (!outCharacterFile)
	{
		return;
	}

	outCharacterFile << character.x << "," << character.y << "," << character.status << "," << character.status_end_time << "," << character.resource_num
		<< "," << character.begin_move_one_title_time << "," << character.path_index << "," << character.build_wall_x << "," << character.build_wall_y << "," << character.path.size();
	for (size_t i = 0; i < character.path.size(); ++i)
	{
		outCharacterFile << "," << character.path[i].first << "," << character.path[i].second;
	}
	outCharacterFile << endl;
	outCharacterFile.close();
	return;
}


SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) 
{
	SDL_Surface* surface = IMG_Load(path.c_str());
	if (!surface) 
	{
		std::cerr << "Failed to load image: " << path << " IMG_Error: " << IMG_GetError() << std::endl;
		return nullptr;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	return texture;
}


int main(int argc, char* argv[]) 
{
	srand(static_cast<unsigned>(time(0)));
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) 
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
		return -1;
	}

	// 初始化 SDL_image 库
	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		std::cout << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
		SDL_Quit();
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("RinWorld", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (!window)
	{
		std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
		IMG_Quit();
		SDL_Quit();
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) 
	{
		std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
		IMG_Quit();
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	SDL_Texture* bg_texture = loadTexture("./bg.png", renderer);
	SDL_Texture* bule_wall_texture = loadTexture("./bule_wall.png", renderer);
	SDL_Texture* role_texture = loadTexture("./role.png", renderer);
	SDL_Texture* tree_texture = loadTexture("./tree.png", renderer);
	SDL_Texture* wall_texture = loadTexture("./wall.png", renderer);
	SDL_Texture* wood_texture = loadTexture("./wood.png", renderer);
	SDL_Texture* red_tree_texture = loadTexture("./red_tree.png", renderer);
	SDL_Texture* storage_textture = loadTexture("./storage.png", renderer);
	if(bg_texture == nullptr || bule_wall_texture == nullptr || role_texture == nullptr || tree_texture == nullptr || wall_texture == nullptr || wood_texture == nullptr
		|| red_tree_texture == nullptr || storage_textture == nullptr)
	{
		std::cerr << "Failed to load texture: " << SDL_GetError() << std::endl;
		IMG_Quit();
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return -1;
	}

	tileMap.clear();
	initializeTileMap();

	Character character1(1);
	LoadCharacterFile(character1);

	Character character2(2);
	LoadCharacterFile(character2);

	SDL_Event event;
	bool isRunning = true;
	bool isSelectingStorage = false;
	int storageStartX = 0, storageStartY = 0, storageEndX = 0, storageEndY = 0;
	bool isSelectingCutDown = false;
	int cutStartX = 0, cutStartY = 0, cutEndX = 0, cutEndY = 0;
	int treeGenerationTimer = 0;

	while (isRunning) 
	{
		while (SDL_PollEvent(&event)) 
		{
			if (event.type == SDL_QUIT) 
			{
				isRunning = false;
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN) 
			{
				// 左键按下：开始选择存储区
				if (event.button.button == SDL_BUTTON_LEFT) 
				{
					storageStartX = event.button.y / TILE_SIZE;
					storageStartY = event.button.x / TILE_SIZE;
					isSelectingStorage = true;
				}

				// 右键按下：开始伐木命令
				if (event.button.button == SDL_BUTTON_RIGHT) 
				{
					cutStartX = event.button.y / TILE_SIZE;
					cutStartY = event.button.x / TILE_SIZE;

					if (event.button.clicks == 2) // 右键双击建造墙
					{
						buildBuleWall(cutStartX, cutStartY);
					}
					else
					{
						isSelectingCutDown = true;
					}					
				}
			}
			else if (event.type == SDL_MOUSEBUTTONUP) 
			{
				// 左键释放：创建存储区
				if (event.button.button == SDL_BUTTON_LEFT && isSelectingStorage) 
				{
					storageEndX = event.button.y / TILE_SIZE;
					storageEndY = event.button.x / TILE_SIZE;
					// 创建存储区
					createStorageArea(storageStartX, storageStartY, storageEndX, storageEndY);
					isSelectingStorage = false;
				}

				// 右键释放：执行伐木命令
				if (event.button.button == SDL_BUTTON_RIGHT && isSelectingCutDown) {
					cutEndX = event.button.y / TILE_SIZE;
					cutEndY = event.button.x / TILE_SIZE;
					// 执行伐木命令
					cutDownTrees(cutStartX, cutStartY, cutEndX, cutEndY);
					isSelectingCutDown = false;
				}
			}
		}

		treeGenerationTimer++;
		if (treeGenerationTimer >= TREE_GENERATION_INTERVAL)
		{
			generateTree();
			treeGenerationTimer = 0;
		}
		UpdateCharacter(character1);
		UpdateCharacter(character2);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		renderTileMap(renderer, bg_texture, bule_wall_texture, tree_texture, wall_texture, wood_texture, red_tree_texture, storage_textture);
		renderCharacter(renderer, character1, role_texture);
		renderCharacter(renderer, character2, role_texture);

		SDL_RenderPresent(renderer);
		SDL_Delay(16); // 控制帧率
	}

	SaveMapToFile();
	SaveCharacterToFile(character1, 1);
	SaveCharacterToFile(character2, 2);

	SDL_DestroyTexture(bg_texture);
	SDL_DestroyTexture(bule_wall_texture);
	SDL_DestroyTexture(role_texture);
	SDL_DestroyTexture(tree_texture);
	SDL_DestroyTexture(wall_texture);
	SDL_DestroyTexture(wood_texture);

	TTF_Quit();
	IMG_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
