#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
#include <stdbool.h>
#include <malloc.h>
#include <conio.h>


// 设置控制台窗口的高、宽
#define CONSOLE_WIDTH 119
#define CONSOLE_HEIGHT 39

// 地图边界、填充值
#define MAP_BOUNDARY 1
#define INSIDE_MAP 0
#define MAP_ROWS 45
#define MAP_COLUMNS 40

int g_Map[MAP_ROWS][MAP_COLUMNS] = { 0 };


// 定义蛇的最大长度、形状
#define MAX_SNAKE_LENGTH 20
#define SNAKE_SHAPE "⊙"


// 键盘控制代码
#define KEY_UP 101
#define KEY_DOWN 204
#define KEY_LEFT 103
#define KEY_RIGHT 206


// 游戏控制
#define GAME_START 200
#define GAME_STOP 201


// 食物
typedef struct _Food
{
	int nRow;
	int nCol;
	char cShape[5];
}Food, *PFood;

Food g_FOOD = { 1, 1, "★" };


// 🐍身节点
typedef struct _SnakeNode
{
	int nRow;
	int nCol;
	int nOrientation;
	WORD color;
} SnakeNode, *PSnakeNode;


// 蛇
typedef struct _Snake
{
	PSnakeNode node;
	int nSnakeNodeLength;
} Snake, *PSnake;

PSnake g_Snake;


// 设置Console信息
bool SetWindowInfo(wchar_t *pTitle)
{
	// 设置标题
	SetConsoleTitle(pTitle);

	// 设置窗口大小
	HANDLE stdHandler = GetStdHandle(STD_OUTPUT_HANDLE);	// 窗口控制句柄
	SMALL_RECT sr;
	sr.Left = 0;
	sr.Right = CONSOLE_WIDTH;
	sr.Top = 0;
	sr.Bottom = CONSOLE_HEIGHT;
	bool scwRes = SetConsoleWindowInfo(stdHandler, true, &sr);
	if (scwRes == 0)
	{
		printf("ERROR: Set Window Info Failed.\r\n");
		return false;
	}

	// 设置缓冲区
	COORD cr = { CONSOLE_WIDTH + 1, CONSOLE_HEIGHT + 1 };
	bool setBuffRes=SetConsoleScreenBufferSize(stdHandler, cr);
	if (setBuffRes == 0)
	{
		printf("ERROR: Set Screen Buffer Size Failed.\r\n");
		return false;
	}

	// 禁止光标显示
	CONSOLE_CURSOR_INFO cur;
	cur.bVisible = false;
	cur.dwSize = sizeof(CONSOLE_CURSOR_INFO);
	bool setCurRes = SetConsoleCursorInfo(stdHandler, &cur);
	if (setCurRes == 0)
	{
		printf("ERROR: Set Cursor Info Failed.\r\n");
		return false;
	}

	return true;
}


/**
 * 在控制台写入字符指定行列写入一串字符
 * wColorAttr: 颜色属性，如: FOREGROUND_BLUE
 */
void WriteChar(char * pChar, int nRow, int nColumn, WORD wColorAttr)
{
	HANDLE stdHandler = GetStdHandle(STD_OUTPUT_HANDLE);
	
	// 设置光标位置
	COORD cr;
	cr.X = nRow * 2;
	cr.Y = nColumn;
	SetConsoleCursorPosition(stdHandler, cr);

	// 设置颜色，输出
	SetConsoleTextAttribute(stdHandler, wColorAttr);
	printf(pChar);
}


// 初始化地图
void InitMap(int ** pMap, int nRows, int nColumns)
{
	for (size_t row = 0; row < nRows; row++)
	{
		for (size_t col = 0; col < nColumns; col++)
		{
			if (row == 0 || row == (nRows - 1) || col == 0 || col == (nColumns - 1))
			{
				// 设置边界地图
				*pMap++ = MAP_BOUNDARY;
				WriteChar("■", row, col, FOREGROUND_BLUE);
			}
			else
			{
				// 地图内
				*pMap++ = INSIDE_MAP;
			}
		}
	}
}


// 随机刷新食物位置
void RandomFoodPos(int nMapRows, int nMapColumns)
{
	/*if (g_FOOD.nRow != 0 && g_FOOD.nCol != 0)
	{
		WriteChar("  ", g_FOOD.nRow, g_FOOD.nCol, FOREGROUND_GREEN);
	}*/

	// 取整数倍（因为WriteChar写入一行是 *2，所以取2的整数倍）
	g_FOOD.nRow = (2 * (int)rand()) % nMapRows - 2;
	g_FOOD.nCol = (2 * (int)rand()) % nMapColumns - 2;

	// 排除边界
	if (g_FOOD.nRow <= 0 || g_FOOD.nCol <= 0)
	{
		RandomFoodPos(nMapRows, nMapColumns);
	}

	WriteChar(g_FOOD.cShape, g_FOOD.nRow, g_FOOD.nCol, FOREGROUND_GREEN);
}


// 初始化🐍
void InitSnake(int nMapRow, int nMapCol)
{
	// 分配内存
	g_Snake = malloc(MAX_SNAKE_LENGTH * sizeof(Snake));
	for (size_t i = 0; i < MAX_SNAKE_LENGTH; i++)
	{
		g_Snake[i].node = malloc(sizeof(SnakeNode));
	}

	//初始化蛇头信息
	int X = nMapRow / 2;
	int Y = nMapCol / 2;
	g_Snake[0].node->nRow = X;
	g_Snake[0].node->nCol = Y;
	g_Snake[0].node->color = FOREGROUND_RED;
	g_Snake[0].node->nOrientation = KEY_RIGHT;
	g_Snake->nSnakeNodeLength = 1;

	WriteChar(SNAKE_SHAPE, X, Y, FOREGROUND_RED);
}


// 增加节点
bool AddSnakeNode(int nRow, int nCol)
{
	if (g_Snake->nSnakeNodeLength >= MAX_SNAKE_LENGTH)
	{
		return false;
	}

	int nSnakeLength = g_Snake->nSnakeNodeLength;
	g_Snake[nSnakeLength].node->nRow = nRow;
	g_Snake[nSnakeLength].node->nCol = nCol;
	g_Snake[nSnakeLength].node->color = FOREGROUND_INTENSITY;
	g_Snake[nSnakeLength].node->nOrientation = g_Snake[nSnakeLength - 1].node->nOrientation;
	g_Snake->nSnakeNodeLength++;

	WriteChar(SNAKE_SHAPE, nRow, nCol, FOREGROUND_INTENSITY);
	return true;
}


// 碰撞检测：撞到地图边界
bool HitMap()
{
	int nSnakeHeadRow = g_Snake[0].node->nRow;
	int nSnakeHeadCol = g_Snake[0].node->nCol;

	if (g_Map[nSnakeHeadRow][nSnakeHeadCol] == 1)
	{
		return true;
	}
	//if (nSnakeHeadRow == 0 || nSnakeHeadCol == 0 || nSnakeHeadRow == MAP_ROWS || nSnakeHeadCol == MAP_COLUMNS)
	//{
	//	return true;
	//}

	return false;
}


// 根据移动方向设置节点行/列
void _Move(int nNode, int nOrientation)
{
	switch (nOrientation)
	{
	case KEY_UP:
		g_Snake[nNode].node->nCol--;
		break;
	case KEY_DOWN:
		g_Snake[nNode].node->nCol++;
		break;	
	case KEY_LEFT:
		g_Snake[nNode].node->nRow--;
		break;
	case KEY_RIGHT:
		g_Snake[nNode].node->nRow++;
		break;
	default:
		break;
	}
}


// 显示||去除 节点
void DisplayNode(int nNode, char * pShape)
{
	int nCurrentSnakeNodeRow = g_Snake[nNode].node->nRow;
	int nCurrentSnakeNodeCol = g_Snake[nNode].node->nCol;
	WORD color = g_Snake[nNode].node->color;
	WriteChar(pShape, nCurrentSnakeNodeRow, nCurrentSnakeNodeCol, color);
}


// 移动单个节点
void Move(int nNode, int nMoveOrientation)
{
	if (HitMap())
	{
		WriteChar("Game Over, You Hit The Map!", MAP_ROWS / 3, MAP_COLUMNS / 4, FOREGROUND_RED);
		getchar();
		system("cls");
		exit(0);
	}

	int nRow = g_Snake[nNode].node->nRow;
	int nCol = g_Snake[nNode].node->nCol;

	// 判断是否吃到食物
	if (nNode == 0 && g_FOOD.nRow == nRow && g_FOOD.nCol == nCol)
	{
		int nSnakeLength = g_Snake->nSnakeNodeLength - 1;
		int nLastSnakeRow = g_Snake[nSnakeLength].node->nRow;
		int nLastSnakeCol = g_Snake[nSnakeLength].node->nCol;

		int nOrientation = g_Snake[nSnakeLength].node->nOrientation;

		switch (nOrientation)
		{
		case KEY_UP:
			nLastSnakeCol++;
			break;
		case KEY_DOWN:
			nLastSnakeCol--;
			break;
		case KEY_LEFT:
			nLastSnakeRow++;
			break;
		case KEY_RIGHT:
			nLastSnakeRow--;
			break;
		default:
			break;
		}

		AddSnakeNode(nLastSnakeRow, nLastSnakeCol);
		RandomFoodPos(MAP_ROWS, MAP_COLUMNS);
	}

	DisplayNode(nNode, "  ");
	if (nMoveOrientation == -1)	// 没有触碰方向键的情况
	{
		if (g_Map[nRow][nCol] == 0)
		{
			_Move(nNode, g_Snake[nNode].node->nOrientation);
			DisplayNode(nNode, SNAKE_SHAPE);
			return;
		}
	}

	if (nNode == 0)	// 蛇头直接判断
	{
		g_Map[nRow][nCol] = nMoveOrientation;
		g_Snake[nNode].node->nOrientation = nMoveOrientation;
		_Move(nNode, nMoveOrientation);
	}
	else
	{
		if (g_Map[nRow][nCol] == 0)	// 当前地图位置为0，走原来的方向位
		{
			_Move(nNode, g_Snake[nNode].node->nOrientation);
		}
		else
		{
			// 根据地图的此刻的位置判断走向
			g_Snake[nNode].node->nOrientation = g_Map[nRow][nCol];
			_Move(nNode, g_Snake[nNode].node->nOrientation);
		}	
	}

	// 最后一个节点，还原地图位置0
	if (nNode == g_Snake->nSnakeNodeLength - 1)
	{
		g_Map[nRow][nCol] = 0;
	}

	DisplayNode(nNode, SNAKE_SHAPE);
}


// 移动🐍的全身
void Moving(int nMoveOrientation)
{
	int nCurrentHeadOrientation1 = g_Snake[0].node->nOrientation * 2;
	int nCurrentHeadOrientation2 = g_Snake[0].node->nOrientation / 2;

	// 防止上下、左右冲突
	if (nMoveOrientation == nCurrentHeadOrientation1 || nMoveOrientation == nCurrentHeadOrientation2)
	{
		return;
	}

	for (size_t i = 0; i < g_Snake->nSnakeNodeLength; i++)
	{
		Move(i, nMoveOrientation);
	}
	Sleep(100);
}


// 判断键盘输入
int GetPressKey(char key)
{
	switch (key)
	{
	case 13:
		return GAME_START;
	case 32:
		return GAME_STOP;
	case 72:
		return KEY_UP;
	case 80: 
		return KEY_DOWN;
	case 75: 
		return KEY_LEFT;
	case 77: 
		return KEY_RIGHT;
	default:
		return 0;
	}
}


// Banner信息
void Banner(bool display)
{
	if (display)
	{
		WriteChar("Greedy Snake Games By Kele.", MAP_ROWS / 3, MAP_COLUMNS / 3.4, FOREGROUND_INTENSITY);
		WriteChar("Press Enter Start Game!", MAP_ROWS / 2.8, MAP_COLUMNS / 3, FOREGROUND_INTENSITY);
		WriteChar("Press Space Exit Game!", MAP_ROWS / 2.8, MAP_COLUMNS / 2.8, FOREGROUND_INTENSITY);
	}
	else
	{
		WriteChar("                           ", MAP_ROWS / 3, MAP_COLUMNS / 3.4, FOREGROUND_INTENSITY);
		WriteChar("                       ", MAP_ROWS / 2.8, MAP_COLUMNS / 3, FOREGROUND_INTENSITY);
		WriteChar("                      ", MAP_ROWS / 2.8, MAP_COLUMNS / 2.8, FOREGROUND_INTENSITY);
	}
}


int main()
{
	// 窗口信息
	SetWindowInfo("贪吃蛇小游戏");

	// 初始化地图
	InitMap(g_Map, MAP_ROWS, MAP_COLUMNS);

	// 食物随机刷新
	srand(time(NULL));

	bool gameRunning = false;
	bool notice = true;
	while (true)
	{
		if (notice)
		{
			Banner(notice);
			notice = false;
		}

		if (_kbhit())
		{
			int op = GetPressKey(_getch());
			if (op == GAME_START)
			{
				gameRunning = true;

				Banner(false);
				RandomFoodPos(MAP_ROWS, MAP_COLUMNS);
				InitSnake(MAP_ROWS, MAP_COLUMNS);
			}
			else if (op == GAME_STOP)
			{
				// 去除蛇的全部节点
				for (size_t i = 0; i < g_Snake->nSnakeNodeLength; i++)
				{
					DisplayNode(i, "  ");
				}

				// 去除食物
				WriteChar("  ", g_FOOD.nRow, g_FOOD.nCol, FOREGROUND_GREEN);
				WriteChar("Game Over!", MAP_ROWS / 2.5, MAP_COLUMNS / 4, FOREGROUND_RED);

				getchar();
				system("cls");
				exit(0);
			}
			else if (op == KEY_UP || op == KEY_DOWN || op == KEY_LEFT || op == KEY_RIGHT)
			{
				Moving(op);
			}
		}
		else if (gameRunning)
		{
			Moving(-1);
			if (g_Snake->nSnakeNodeLength >= MAX_SNAKE_LENGTH)
			{

				// 去除食物
				WriteChar("  ", g_FOOD.nRow, g_FOOD.nCol, FOREGROUND_GREEN);

				char news[50];
				sprintf(news, "Total Score: %d.", g_Snake->nSnakeNodeLength);

				WriteChar(" Game Over, You Won!", MAP_ROWS / 3, MAP_COLUMNS / 4, FOREGROUND_GREEN);
				WriteChar(news, MAP_ROWS / 2.8, MAP_COLUMNS / 3, FOREGROUND_GREEN);

				getchar();
				system("cls");
				exit(0);
			}
		}
	}

	return 0;
}