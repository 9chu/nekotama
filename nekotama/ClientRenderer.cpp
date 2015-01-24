#include "ClientRenderer.h"

#include "Encoding.h"

#define HINTWIDTH       200.f
#define HINTHEIGHT      80.f
#define HINTOFFSETX     0.f
#define HINTOFFSETY     (-5.0f)
#define HINTTEXTWIDTH   150.f
#define HINTTEXTHEIGHT  40.f
#define HINTTEXTOFFSETX 20.f
#define HINTTEXTOFFSETY 30.f

#define GAMEINFOTOP            88.f
#define GAMEINFOHEIGHT         15.f
#define GAMEINFOLEFT           32.f
#define GAMEINFOWIDTH_NICKNAME 150.f
#define GAMEINFOWIDTH_IP       105.f
#define GAMEINFOWIDTH_PORT     50.f
#define GAMEINFOWIDTH_DELAY    40.f

using namespace std;
using namespace nekotama;

static chrono::microseconds HINT_MOVEIN = chrono::duration_cast<chrono::microseconds>(chrono::milliseconds(1000));
static chrono::microseconds HINT_MOVEOUT = chrono::duration_cast<chrono::microseconds>(chrono::milliseconds(1000));
static chrono::microseconds HINT_SHOWTIME = chrono::duration_cast<chrono::microseconds>(chrono::milliseconds(3000));

ClientRenderer::ClientRenderer(const std::wstring& resDir, IDirect3DDevice9* pDev)
	: m_pDev(pDev), m_ResDir(resDir), m_cFPS(0), m_cFrameCounter(0), m_pFPSFont(NULL), m_iDelay((uint32_t)-1),
	m_bGameInfoListVisible(false)
{
	// 创建字体
	D3DXCreateFont(m_pDev, 16, 0, 1, D3DX_DEFAULT, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, L"Times New Roman", &m_pFPSFont);
	D3DXCreateFont(m_pDev, 13, 0, 1, D3DX_DEFAULT, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, L"simhei", &m_pHintFont);

	// 加载纹理
	D3DXCreateTextureFromFileEx(
		m_pDev, 
		(m_ResDir + L"\\Hint.png").c_str(),
		D3DX_DEFAULT_NONPOW2, 
		D3DX_DEFAULT_NONPOW2, 
		D3DX_DEFAULT, 
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER,
		D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER,
		0,
		NULL,
		NULL,
		&m_pHintBackground);
	D3DXIMAGE_INFO tGameInfoListImgInfo;
	D3DXCreateTextureFromFileEx(
		m_pDev,
		(m_ResDir + L"\\GameList.png").c_str(),
		D3DX_DEFAULT_NONPOW2,
		D3DX_DEFAULT_NONPOW2,
		D3DX_DEFAULT,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER,
		D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER,
		0,
		&tGameInfoListImgInfo,
		NULL,
		&m_pGameInfoListBackground);
	m_iGameInfoListBackgroundWidth = tGameInfoListImgInfo.Width;
	m_iGameInfoListBackgroundHeight = tGameInfoListImgInfo.Height;

	// 创建精灵
	D3DXCreateSprite(m_pDev, &m_pMainSprite);

	// 初始化计数器
	m_cLastTimePoint = chrono::high_resolution_clock::now();
}

ClientRenderer::~ClientRenderer()
{}

void ClientRenderer::ShowHint(const std::wstring& text)
{
	int tLastIndex = 0;
	auto i = m_HintList.begin();
	for (; i != m_HintList.end(); ++i)
	{
		if (i->DisplayIndex != tLastIndex)
			break;
		else
			tLastIndex = i->DisplayIndex + 1;
	}

	// 插入对象
	m_HintList.emplace(i, tLastIndex, text);
}

void ClientRenderer::SetGameInfo(const std::vector<Client::GameInfo>& info)
{
	unique_lock<mutex> tLock(m_GameInfoListMutex);

	m_GameInfoList.clear();
	for (auto i : info)
	{
		GameInfoDisplay t;
		t.Delay = i.Delay;
		t.VirtualPort = i.VirtualPort;
		t.Nickname = MultiByteToWideChar(i.Nickname);
		t.VirtualAddr = MultiByteToWideChar(i.VirtualAddr);
		m_GameInfoList.emplace_back(move(t));
	}
}

void ClientRenderer::SetDelay(uint32_t iDelay)
{
	unique_lock<mutex> tLock(m_GameInfoListMutex);

	m_iDelay = iDelay;
}

bool ClientRenderer::IsGameInfoListVisible()
{
	return m_bGameInfoListVisible;
}

void ClientRenderer::HideGameInfoList()
{
	m_bGameInfoListVisible = false;
}

void ClientRenderer::ShowGameInfoList()
{
	m_bGameInfoListVisible = true;
}

void ClientRenderer::DoDeviceLost()
{
	if (m_pFPSFont)
		m_pFPSFont->OnLostDevice();
	if (m_pHintFont)
		m_pHintFont->OnLostDevice();
	if (m_pMainSprite)
		m_pMainSprite->OnLostDevice();
}

void ClientRenderer::DoDeviceReset()
{
	if (m_pFPSFont)
		m_pFPSFont->OnResetDevice();
	if (m_pHintFont)
		m_pHintFont->OnResetDevice();
	if (m_pMainSprite)
		m_pMainSprite->OnResetDevice();
}

void ClientRenderer::Render()
{
	// 计算流逝时间
	auto cur = chrono::high_resolution_clock::now();
	auto elapsed = chrono::duration_cast<chrono::microseconds>(cur - m_cLastTimePoint);
	m_cLastTimePoint = cur;

	// 计算FPS
	m_cFrameCounter++;
	m_cFrameDuration += elapsed;
	if (m_cFrameDuration.count() >= 1000000)
	{
		m_cFPS = m_cFrameCounter / (m_cFrameDuration.count() / 1000000.);
		m_cFrameCounter = 0;
		m_cFrameDuration = chrono::milliseconds::zero();
	}

	// 获取视图大小
	D3DVIEWPORT9 tViewPort;
	m_pDev->GetViewport(&tViewPort);

	// 更新所有的提示文本
	auto i = m_HintList.begin();
	while (i != m_HintList.end())
	{
		i->Timer += elapsed;
		switch (i->State)
		{
		case 0:
			if (i->Timer >= HINT_MOVEIN)
			{
				i->Timer = chrono::microseconds::zero();
				i->State = 1;
			}
			break;
		case 1:
			if (i->Timer >= HINT_SHOWTIME)
			{
				i->Timer = chrono::microseconds::zero();
				i->State = 2;
			}
			break;
		case 2:
			if (i->Timer >= HINT_MOVEOUT)
				i->State = 3;
			break;
		}
		if (i->State == 3)
			i = m_HintList.erase(i);
		else
			++i;
	}

	m_pDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
	m_pDev->BeginScene();
	
	// 绘制所有的提示文本
	if (m_pMainSprite && m_pHintBackground && m_pHintFont)
	{
		for (auto i = m_HintList.begin(); i != m_HintList.end(); ++i)
		{
			D3DXVECTOR3 tPos;

			// 计算位置
			switch (i->State)
			{
			case 0:
				{
					float k = i->Timer.count() / (float)HINT_MOVEIN.count();
					k = - k * k + 2 * k;
					float xs = (float)tViewPort.Width;
					float xe = (float)tViewPort.Width - HINTWIDTH + HINTOFFSETX;
					tPos.x = xs + (xe - xs) * k;
					tPos.y = i->DisplayIndex * (HINTHEIGHT + HINTOFFSETY);
					tPos.z = 0.f;
				}
				break;
			case 1:
				{
					tPos.x = (float)tViewPort.Width - HINTWIDTH + HINTOFFSETX;
					tPos.y = i->DisplayIndex * (HINTHEIGHT + HINTOFFSETY);
					tPos.z = 0.f;
				}
				break;
			case 2:
				{
					float k = i->Timer.count() / (float)HINT_MOVEIN.count();
					k = - k * k + 2 * k;
					float xs = (float)tViewPort.Width - HINTWIDTH + HINTOFFSETX;
					float xe = (float)tViewPort.Width;
					tPos.x = xs + (xe - xs) * k;
					tPos.y = i->DisplayIndex * (HINTHEIGHT + HINTOFFSETY);
					tPos.z = 0.f;
				}
				break;
			}

			m_pMainSprite->Begin(0);
			m_pMainSprite->Draw(m_pHintBackground, NULL, &D3DXVECTOR3(0, 0, 0), &tPos, 0xFFFFFFFF);
			m_pMainSprite->End();

			// 绘制文本
			RECT tDrawPos;
			tDrawPos.left = (LONG)(tPos.x + HINTTEXTOFFSETX);
			tDrawPos.top = (LONG)(tPos.y + HINTTEXTOFFSETY);
			tDrawPos.right = (LONG)(tDrawPos.left + HINTTEXTWIDTH);
			tDrawPos.bottom = (LONG)(tDrawPos.top + HINTTEXTHEIGHT);
			m_pHintFont->DrawTextW(NULL, i->Text.c_str(), -1, &tDrawPos, DT_LEFT | DT_TOP, D3DCOLOR_ARGB(250, 210, 210, 210));
		}
	}

	// 绘制游戏列表
	if (m_bGameInfoListVisible && m_pHintFont && m_pMainSprite && m_pGameInfoListBackground)
	{
		unique_lock<mutex> tLock(m_GameInfoListMutex);

		// 计算绘制位置
		D3DXVECTOR3 tPos;
		tPos.x = tViewPort.Width / 2.f - m_iGameInfoListBackgroundWidth / 2.f;
		tPos.y = tViewPort.Height / 2.f - m_iGameInfoListBackgroundHeight / 2.f;
		tPos.z = 0.f;

		m_pMainSprite->Begin(0);
		m_pMainSprite->Draw(m_pGameInfoListBackground, NULL, &D3DXVECTOR3(0, 0, 0), &tPos, 0xFFFFFFFF);
		m_pMainSprite->End();

		for (size_t i = 0; i < m_GameInfoList.size(); ++i)
		{
			wchar_t tBuf[64];
			RECT tDrawPos;
			tDrawPos.top = (LONG)(tPos.y + GAMEINFOTOP + i * GAMEINFOHEIGHT);
			tDrawPos.bottom = (LONG)(tPos.y + GAMEINFOTOP + (i + 1) * GAMEINFOHEIGHT);
			
			tDrawPos.left = (LONG)(tPos.x + GAMEINFOLEFT);
			tDrawPos.right = (LONG)(tDrawPos.left + GAMEINFOWIDTH_NICKNAME);
			m_pHintFont->DrawTextW(NULL, m_GameInfoList[i].Nickname.c_str(), -1, &tDrawPos, DT_LEFT, 0xFDFFFFFF);

			tDrawPos.left = (LONG)(tDrawPos.right);
			tDrawPos.right = (LONG)(tDrawPos.left + GAMEINFOWIDTH_IP);
			m_pHintFont->DrawTextW(NULL, m_GameInfoList[i].VirtualAddr.c_str(), -1, &tDrawPos, DT_LEFT, 0xFDFFFFFF);

			tDrawPos.left = (LONG)(tDrawPos.right);
			tDrawPos.right = (LONG)(tDrawPos.left + GAMEINFOWIDTH_PORT);
			m_pHintFont->DrawTextW(NULL, _itow(m_GameInfoList[i].VirtualPort, tBuf, 10), -1, &tDrawPos, DT_LEFT, 0xFDFFFFFF);

			tDrawPos.left = (LONG)(tDrawPos.right);
			tDrawPos.right = (LONG)(tDrawPos.left + GAMEINFOWIDTH_DELAY);
			m_pHintFont->DrawTextW(NULL, _itow(m_GameInfoList[i].Delay, tBuf, 10), -1, &tDrawPos, DT_LEFT, 0xFDFFFFFF);
		}
	}
	
	// 绘制FPS文本
	if (m_pFPSFont)
	{
		wchar_t tBuf[64];
		if (m_iDelay == (uint32_t)-1)
			swprintf(tBuf, sizeof(tBuf), L"FPS:%.2lf", m_cFPS);
		else
			swprintf(tBuf, sizeof(tBuf), L"延迟:%dms  FPS:%.2lf", m_iDelay, m_cFPS);
		RECT tDrawPos;
		tDrawPos.right = tViewPort.Width - 15;
		tDrawPos.left = 15;
		tDrawPos.top = 5;
		tDrawPos.bottom = tDrawPos.top + 20;
		m_pFPSFont->DrawTextW(NULL, tBuf, -1, &tDrawPos, DT_RIGHT, D3DCOLOR_ARGB(150, 50, 50, 50));
		tDrawPos.right -= 1;
		tDrawPos.left -= 1;
		tDrawPos.top -= 1;
		tDrawPos.bottom -= 1;
		m_pFPSFont->DrawTextW(NULL, tBuf, -1, &tDrawPos, DT_RIGHT, D3DCOLOR_ARGB(250, 0, 200, 0));
	}

	m_pDev->EndScene();
}
