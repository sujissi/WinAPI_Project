#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <stdlib.h>
#include <atlImage.h>
#pragma comment(lib, "msimg32.lib")


HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"공룡게임";
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR IpCmdLine, int nCmdShow)
{
	HWND hWnd;
	MSG Message;

	//윈도우 구조체 설정
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

	//구조체 등록
	RegisterClassEx(&WndClass);

	//윈도우 생성
	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW,
		0, 0, 500, 500, NULL, (HMENU)NULL, hInstance, NULL);

	//윈도우 출력
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//메세지 루프
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


//장애물
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

	//이미지
	static CImage player_img[4];
	static CImage background;
	static CImage gameover;
	static CImage num_img[10];
	static int background_xPos, background_yPos;
	static int background_xPos2, background_yPos2;

	//크기, 위치
	static POINT click_pt;

	//시간변수
	static int jumpTimer;
	static int walkTimer = 0;
	static int createTimer = 0;

	//스코어
	static int score = 0;
	TCHAR str[50];

	//티노
	static PLAYER player;
	RECT player_rt = { player.c.x - (int)player.c.r, player.c.y - (int)player.c.r,
				player.c.x + (int)player.c.r, player.c.y + (int)player.c.r };

	//토글
	static bool isReady = true;
	static bool isPlay = false;
	static bool isGameOver = false;
	static bool isJump = false;
	static bool isShortJump = false;
	static bool isLongJump = false;
	static bool isWalkShape = false;

	switch (uMSG) {
	case WM_CREATE:

		player_img[WALK1].Load(TEXT("이미지/1-1.png"));
		player_img[WALK2].Load(TEXT("이미지/1-2.png"));
		player_img[JUMP].Load(TEXT("이미지/1-3.png"));
		player_img[COLLISION].Load(TEXT("이미지/2.png"));
		obstacle_img[SMALL1].Load(TEXT("이미지/ob1-1.png"));
		obstacle_img[SMALL2].Load(TEXT("이미지/ob1-2.png"));
		obstacle_img[MEDIUM1].Load(TEXT("이미지/ob2-1.png"));
		obstacle_img[MEDIUM2].Load(TEXT("이미지/ob2-2.png"));
		background.Load(TEXT("이미지/ground.png"));
		gameover.Load(TEXT("이미지/gameover.png"));

		//윈도우 사이즈, 위치 얻기
		GetClientRect(hwnd, &win_rt);
		center_pt = { (win_rt.left + win_rt.right) / 2 ,(win_rt.top + win_rt.bottom) / 2 };

		//위치 초기화
		background_xPos = win_rt.left, background_yPos = center_pt.y + 10;
		background_xPos2 = win_rt.left + background.GetWidth(), background_yPos2 = center_pt.y + 10;

		//player 크기 초기화
		player.c = { center_pt.x - 140,center_pt.y,15 };
		player.w = player_img[WALK1].GetWidth();
		player.h = player_img[WALK1].GetHeight();
		player.left = player.c.x - player.w / 2;
		player.top = player.c.y - player.h / 2;

		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_SPACE://점프
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
		case VK_SPACE://점프
			if (isReady)//시작화면
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

		//배경
		FillRect(memdc, &win_rt, (HBRUSH)GetStockObject(WHITE_BRUSH));
		wsprintf(str, L"SCORE: %d", score);
		TextOut(memdc, 5, 5, str, wcslen(str));

		//시작화면
		if (isReady)
		{
			background.TransparentBlt(memdc, background_xPos, background_yPos, background.GetWidth(), background.GetHeight(), RGB(64, 202, 201));
			player_img[0].TransparentBlt(memdc, player.left, player.top, player.w, player.h, RGB(0, 255, 0));
		}

		//플레이 화면
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
			

			//게임오버
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
			//충돌체크
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

				if (isUp)//올라갔다
				{
					player.c.y -= 3;
					player.top -= 3;
				}
				if (isDown)//내려갔다
				{
					player.c.y += 3;
					player.top += 3;
				}
				if (isGround)//착지
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
				//걸을 때
				if (walkTimer == 0)
				{
					walkTimer = 10;
					isWalkShape = !isWalkShape;
				}
				else --walkTimer;
				if (isWalkShape) player.shape = WALK2;
				else player.shape = WALK1;
			}
			//장애물 생성하기
			if (createTimer > 0)
				--createTimer;
			if (createTimer == 0)
			{
				createTimer = rand() % 150 + 80;
				SetTimer(hwnd, CREATE, 0, NULL);
			}

			//배경 움직이기
			background_xPos -= 2;
			background_xPos2 -= 2;
			if (background_xPos < win_rt.left - background.GetWidth())
				background_xPos = win_rt.right;
			if (background_xPos2 < win_rt.left - background.GetWidth())
				background_xPos2 = win_rt.right;

			if (ob_SL != NULL)
				for (OBSTACLE* current = ob_SL; current->next != NULL; current = current->next)
				{
					//장애물 움직이기
					current->c.x -= 2;
					current->left -= 2;

					//장애물 삭제하기
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

// 두 원 중심 사이의 거리
double CircleDist(CIRCLE c1, CIRCLE c2)
{
	double distance = sqrt(pow(c1.x - c2.x, 2) + pow(c1.y - c2.y, 2));
	return distance;
}

//충돌체크
bool isCollision(CIRCLE c1, CIRCLE c2)
{
	double distance = CircleDist(c1, c2);
	if (distance < c1.r + c2.r) return true; // 충돌
	else return false; // 충돌x
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