#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <stdlib.h>
#include <atlImage.h>
#pragma comment(lib, "msimg32.lib")


HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"�������";
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR IpCmdLine, int nCmdShow)
{
	HWND hWnd;
	MSG Message;

	//������ ����ü ����
	WNDCLASSEX WndClass;
	g_hInst = hInstance;
	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_HAND);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_QUESTION);

	//����ü ���
	RegisterClassEx(&WndClass);

	//������ ����
	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW,
		0, 0, 500, 500, NULL, (HMENU)NULL, hInstance, NULL);

	//������ ���
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//�޼��� ����
	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}

enum Timer
{
	PLAY,
	CREATE
};
enum PlayerShape
{
	WALK1,
	WALK2,
	JUMP,
	COLLISION
};
enum OBShape
{
	SMALL1,
	SMALL2,
	MEDIUM1,
	MEDIUM2,
};

struct CIRCLE
{
	int x;
	int y;
	double r;
};

struct PLAYER
{
	CIRCLE c;
	int w, h;
	int left, top;
	int shape;
};
struct OBSTACLE
{
	CIRCLE c;
	int shape;
	int w, h;
	int left, top;
	OBSTACLE* next;
};


//��ֹ�
OBSTACLE* ob_SL;
CImage obstacle_img[4];


double CircleDist(CIRCLE c1, CIRCLE c2);
bool isCollision(CIRCLE c1, CIRCLE c2);
void CreateObstacle();
OBSTACLE* GetTail();
void RemoveObstacle(OBSTACLE* target);

RECT win_rt;
POINT center_pt;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMSG, WPARAM wParam, LPARAM lParam)
{
	srand(time(NULL));
	PAINTSTRUCT ps;
	HDC hdc, memdc;
	HBITMAP hBit, hOldBit;

	//�̹���
	static CImage player_img[4];
	static CImage background;
	static CImage gameover;
	static CImage num_img[10];
	static int background_xPos, background_yPos;
	static int background_xPos2, background_yPos2;

	//ũ��, ��ġ
	static POINT click_pt;

	//�ð�����
	static int jumpTimer;
	static int walkTimer = 0;
	static int createTimer = 0;

	//���ھ�
	static int score = 0;
	TCHAR str[50];

	//Ƽ��
	static PLAYER player;
	RECT player_rt = { player.c.x - (int)player.c.r, player.c.y - (int)player.c.r,
				player.c.x + (int)player.c.r, player.c.y + (int)player.c.r };

	//���
	static bool isReady = true;
	static bool isPlay = false;
	static bool isGameOver = false;
	static bool isJump = false;
	static bool isShortJump = false;
	static bool isLongJump = false;
	static bool isWalkShape = false;

	switch (uMSG) {
	case WM_CREATE:

		player_img[WALK1].Load(TEXT("�̹���/1-1.png"));
		player_img[WALK2].Load(TEXT("�̹���/1-2.png"));
		player_img[JUMP].Load(TEXT("�̹���/1-3.png"));
		player_img[COLLISION].Load(TEXT("�̹���/2.png"));
		obstacle_img[SMALL1].Load(TEXT("�̹���/ob1-1.png"));
		obstacle_img[SMALL2].Load(TEXT("�̹���/ob1-2.png"));
		obstacle_img[MEDIUM1].Load(TEXT("�̹���/ob2-1.png"));
		obstacle_img[MEDIUM2].Load(TEXT("�̹���/ob2-2.png"));
		background.Load(TEXT("�̹���/ground.png"));
		gameover.Load(TEXT("�̹���/gameover.png"));

		//������ ������, ��ġ ���
		GetClientRect(hwnd, &win_rt);
		center_pt = { (win_rt.left + win_rt.right) / 2 ,(win_rt.top + win_rt.bottom) / 2 };

		//��ġ �ʱ�ȭ
		background_xPos = win_rt.left, background_yPos = center_pt.y + 10;
		background_xPos2 = win_rt.left + background.GetWidth(), background_yPos2 = center_pt.y + 10;

		//player ũ�� �ʱ�ȭ
		player.c = { center_pt.x - 140,center_pt.y,15 };
		player.w = player_img[WALK1].GetWidth();
		player.h = player_img[WALK1].GetHeight();
		player.left = player.c.x - player.w / 2;
		player.top = player.c.y - player.h / 2;

		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_SPACE://����
			if (isPlay)
			{
				isJump = true;
				isLongJump = true;
			}
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_SPACE://����
			if (isReady)//����ȭ��
			{
				isReady = false;
				isPlay = true;
				SetTimer(hwnd, PLAY, 0, NULL);
				InvalidateRgn(hwnd, NULL, FALSE);
			}
			if (isPlay && jumpTimer < 30)
			{
				isShortJump = true;
			}
			break;
		}
		break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hwnd, &ps);
		hBit = CreateCompatibleBitmap(hdc, win_rt.right, win_rt.bottom);
		memdc = CreateCompatibleDC(hdc);
		hOldBit = (HBITMAP)SelectObject(memdc, hBit);
		SelectObject(memdc, (HBITMAP)hBit);

		//���
		FillRect(memdc, &win_rt, (HBRUSH)GetStockObject(WHITE_BRUSH));
		wsprintf(str, L"SCORE: %d", score);
		TextOut(memdc, 5, 5, str, wcslen(str));

		//����ȭ��
		if (isReady)
		{
			background.TransparentBlt(memdc, background_xPos, background_yPos, background.GetWidth(), background.GetHeight(), RGB(64, 202, 201));
			player_img[0].TransparentBlt(memdc, player.left, player.top, player.w, player.h, RGB(0, 255, 0));
		}

		//�÷��� ȭ��
		if (isPlay)
		{
			//Rectangle(memdc,player_rt.left,player_rt.top,player_rt.right,player_rt.bottom);
			background.TransparentBlt(memdc, background_xPos, background_yPos, background.GetWidth(), background.GetHeight(), RGB(64, 202, 201));
			background.TransparentBlt(memdc, background_xPos2, background_yPos2, background.GetWidth(), background.GetHeight(), RGB(64, 202, 201));
			switch (player.shape)
			{
			case WALK1:
				player_img[0].TransparentBlt(memdc, player.left, player.top, player.w, player.h, RGB(0, 255, 0));
				break;
			case WALK2:
				player_img[1].TransparentBlt(memdc, player.left, player.top, player.w, player.h, RGB(0, 255, 0));
				break;
			case JUMP:
				player_img[2].TransparentBlt(memdc, player.left, player.top, player.w, player.h, RGB(0, 255, 0));
				break;
			case COLLISION:
				player_img[3].TransparentBlt(memdc, player.left, player.top, player.w, player.h, RGB(0, 255, 0));
				break;
			}
			if (ob_SL != NULL)
				for (OBSTACLE* p = ob_SL; p->next != NULL; p = p->next)
				{
					//Rectangle(memdc, p->c.x-p->c.r,p->c.y-p->c.r, p->c.x + p->c.r, p->c.y + p->c.r);
					switch (p->shape)
					{
					case SMALL1:
						obstacle_img[SMALL1].TransparentBlt(memdc, p->left, p->top, p->w, p->h, RGB(64, 202, 201));
						break;
					case SMALL2:
						obstacle_img[SMALL2].TransparentBlt(memdc, p->left, p->top, p->w, p->h, RGB(64, 202, 201));
						break;
					case MEDIUM1:
						obstacle_img[MEDIUM1].TransparentBlt(memdc, p->left, p->top, p->w, p->h, RGB(64, 202, 201));
						break;
					case MEDIUM2:
						obstacle_img[MEDIUM2].TransparentBlt(memdc, p->left, p->top, p->w, p->h, RGB(64, 202, 201));
						break;
					}
				}
			

			//���ӿ���
			if (isGameOver)
			{
				int w = background.GetWidth() / 4, h = background.GetHeight() / 4;
				gameover.TransparentBlt(memdc, center_pt.x - (w / 2), center_pt.y - (h / 2) - 100, w, h + 10, RGB(64, 202, 201));
				KillTimer(hwnd, PLAY);
			}
		}

		BitBlt(hdc, 0, 0, win_rt.right, win_rt.bottom, memdc, 0, 0, SRCCOPY);
		SelectObject(memdc, hOldBit);
		DeleteObject(hBit);
		DeleteDC(memdc);
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_TIMER:
		switch (wParam)
		{
		case PLAY:
		{
			//�浹üũ
			if (ob_SL != NULL)
				for (OBSTACLE* current = ob_SL; current->next != NULL; current = current->next)
				{
					if (isCollision(player.c, current->c))
					{

						isGameOver = true;
						player.shape = COLLISION;
						InvalidateRgn(hwnd, NULL, FALSE);
					}
				}


			if (isJump)
			{
				player.shape = JUMP;
				++jumpTimer;

				bool isUp = (isShortJump) ? jumpTimer > 0 && jumpTimer < 30:jumpTimer > 0 && jumpTimer < 50;
				bool isDown = (isShortJump) ? jumpTimer > 30 && jumpTimer < 60:jumpTimer > 50 && jumpTimer < 100;
				bool isGround = (isShortJump) ? jumpTimer >= 60 : jumpTimer >= 100;

				if (isUp)//�ö󰬴�
				{
					player.c.y -= 3;
					player.top -= 3;
				}
				if (isDown)//��������
				{
					player.c.y += 3;
					player.top += 3;
				}
				if (isGround)//����
				{
					player.shape = WALK1;
					isJump = false;
					isLongJump = false;
					isShortJump = false;
					player.c.y = center_pt.y;
					jumpTimer = 0;
				}

			}
			else
			{
				//���� ��
				if (walkTimer == 0)
				{
					walkTimer = 10;
					isWalkShape = !isWalkShape;
				}
				else --walkTimer;
				if (isWalkShape) player.shape = WALK2;
				else player.shape = WALK1;
			}
			//��ֹ� �����ϱ�
			if (createTimer > 0)
				--createTimer;
			if (createTimer == 0)
			{
				createTimer = rand() % 150 + 80;
				SetTimer(hwnd, CREATE, 0, NULL);
			}

			//��� �����̱�
			background_xPos -= 2;
			background_xPos2 -= 2;
			if (background_xPos < win_rt.left - background.GetWidth())
				background_xPos = win_rt.right;
			if (background_xPos2 < win_rt.left - background.GetWidth())
				background_xPos2 = win_rt.right;

			if (ob_SL != NULL)
				for (OBSTACLE* current = ob_SL; current->next != NULL; current = current->next)
				{
					//��ֹ� �����̱�
					current->c.x -= 2;
					current->left -= 2;

					//��ֹ� �����ϱ�
					if (current->left == win_rt.left - current->w)
					{
						++score;
						RemoveObstacle(current);
						ob_SL = ob_SL->next;
					}
				}

			break;
		}
		case CREATE:
		{
			CreateObstacle();
			KillTimer(hwnd, CREATE);
			break;
		}
		}
		InvalidateRgn(hwnd, NULL, FALSE);
		break;
	case WM_DESTROY:

		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, uMSG, wParam, lParam);
}

// �� �� �߽� ������ �Ÿ�
double CircleDist(CIRCLE c1, CIRCLE c2)
{
	double distance = sqrt(pow(c1.x - c2.x, 2) + pow(c1.y - c2.y, 2));
	return distance;
}

//�浹üũ
bool isCollision(CIRCLE c1, CIRCLE c2)
{
	double distance = CircleDist(c1, c2);
	if (distance < c1.r + c2.r) return true; // �浹
	else return false; // �浹x
}


void CreateObstacle()
{
	OBSTACLE* newOB = (OBSTACLE*)malloc(sizeof(OBSTACLE));
	newOB->shape = rand() % 4;
	int sz = 3;
	switch (newOB->shape)
	{
	case SMALL1:
		newOB->w = obstacle_img[SMALL1].GetWidth() / sz;
		newOB->h = obstacle_img[SMALL1].GetHeight() / sz;
		newOB->left = win_rt.right - newOB->w;
		newOB->top = center_pt.y - newOB->h + 30;
		newOB->c.r = newOB->h / 4;
		newOB->c.x = newOB->left + newOB->c.r;
		newOB->c.y = newOB->top + newOB->c.r;
		break;
	case SMALL2:
		newOB->w = obstacle_img[SMALL2].GetWidth() / sz;
		newOB->h = obstacle_img[SMALL2].GetHeight() / sz;
		newOB->left = win_rt.right - newOB->w;
		newOB->top = center_pt.y - newOB->h + 30;
		newOB->c.r = newOB->h / 2;
		newOB->c.x = newOB->left + newOB->c.r;
		newOB->c.y = newOB->top + newOB->c.r;
		break;
	case MEDIUM1:
		newOB->w = obstacle_img[MEDIUM1].GetWidth() / sz;
		newOB->h = obstacle_img[MEDIUM1].GetHeight() / sz;
		newOB->left = win_rt.right - newOB->w;
		newOB->top = center_pt.y - newOB->h + 30;
		newOB->c.r = newOB->h / 4;
		newOB->c.x = newOB->left + newOB->c.r;
		newOB->c.y = newOB->top + newOB->c.r;
		break;
	case MEDIUM2:
		newOB->w = obstacle_img[MEDIUM2].GetWidth() / sz;
		newOB->h = obstacle_img[MEDIUM2].GetHeight() / sz;
		newOB->left = win_rt.right - newOB->w;
		newOB->top = center_pt.y - newOB->h + 30;
		newOB->c.r = newOB->h / 4;
		newOB->c.x = newOB->left + newOB->c.r;
		newOB->c.y = newOB->top + newOB->c.r;
		break;

	}
	newOB->next = NULL;
	if (ob_SL == NULL) ob_SL = newOB;
	else GetTail()->next = newOB;

}

OBSTACLE* GetTail()
{
	OBSTACLE* current = ob_SL;
	while (1)
	{
		if (current->next == NULL) return current;
		current = current->next;
	}
}

void RemoveObstacle(OBSTACLE* target)
{
	free(target);
}