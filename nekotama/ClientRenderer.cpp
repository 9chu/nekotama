#include "ClientRenderer.h"

#define HINTWIDTH       200.f
#define HINTHEIGHT      80.f
#define HINTOFFSETX     0.f
#define HINTOFFSETY     (-5.0f)
#define HINTTEXTWIDTH   150.f
#define HINTTEXTHEIGHT  40.f
#define HINTTEXTOFFSETX 20.f
#define HINTTEXTOFFSETY 30.f

using namespace std;
using namespace nekotama;

static chrono::microseconds HINT_MOVEIN = chrono::duration_cast<chrono::microseconds>(chrono::milliseconds(1000));
static chrono::microseconds HINT_MOVEOUT = chrono::duration_cast<chrono::microseconds>(chrono::milliseconds(1000));
static chrono::microseconds HINT_SHOWTIME = chrono::duration_cast<chrono::microseconds>(chrono::milliseconds(3000));

ClientRenderer::ClientRenderer(const std::wstring& resDir, IDirect3DDevice9* pDev)
	: m_pDev(pDev), m_ResDir(resDir), m_cFPS(0), m_cFrameCounter(0), m_pFPSFont(NULL), m_iDelay((uint32_t)-1)
{
	// ��������
	D3DXCreateFont(m_pDev, 16, 0, 1, D3DX_DEFAULT, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, L"Times New Roman", &m_pFPSFont);
	D3DXCreateFont(m_pDev, 13, 0, 1, D3DX_DEFAULT, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, L"simhei", &m_pHintFont);

	// ��������
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

	// ��������
	D3DXCreateSprite(m_pDev, &m_pHintSprite);

	// ��ʼ��������
	m_cLastTimePoint = chrono::high_resolution_clock::now();
}

ClientRenderer::~ClientRenderer()
{
}

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

	// �������
	m_HintList.emplace(i, tLastIndex, text);
}

void ClientRenderer::SetDelay(uint32_t iDelay)
{
	m_iDelay = iDelay;
}

void ClientRenderer::Render()
{
	// ��������ʱ��
	auto cur = chrono::high_resolution_clock::now();
	auto elapsed = chrono::duration_cast<chrono::microseconds>(cur - m_cLastTimePoint);
	m_cLastTimePoint = cur;

	// ����FPS
	m_cFrameCounter++;
	m_cFrameDuration += elapsed;
	if (m_cFrameDuration.count() >= 1000000)
	{
		m_cFPS = m_cFrameCounter / (m_cFrameDuration.count() / 1000000.);
		m_cFrameCounter = 0;
		m_cFrameDuration = chrono::milliseconds::zero();
	}

	// ��ȡ��ͼ��С
	D3DVIEWPORT9 tViewPort;
	m_pDev->GetViewport(&tViewPort);

	// �������е���ʾ�ı�
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
	
	// �������е���ʾ�ı�
	if (m_pHintSprite && m_pHintBackground && m_pHintFont)
	{
		for (auto i = m_HintList.begin(); i != m_HintList.end(); ++i)
		{
			D3DXVECTOR3 tPos;

			// ����λ��
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

			m_pHintSprite->Begin(0);
			m_pHintSprite->Draw(m_pHintBackground, NULL, &D3DXVECTOR3(0, 0, 0), &tPos, 0xFFFFFFFF);
			m_pHintSprite->End();

			// �����ı�
			RECT tDrawPos;
			tDrawPos.left = (LONG)(tPos.x + HINTTEXTOFFSETX);
			tDrawPos.top = (LONG)(tPos.y + HINTTEXTOFFSETY);
			tDrawPos.right = (LONG)(tDrawPos.left + HINTTEXTWIDTH);
			tDrawPos.bottom = (LONG)(tDrawPos.top + HINTTEXTHEIGHT);
			m_pHintFont->DrawTextW(NULL, i->Text.c_str(), -1, &tDrawPos, DT_LEFT | DT_TOP, D3DCOLOR_ARGB(250, 210, 210, 210));
		}
	}
	
	// ����FPS�ı�
	if (m_pFPSFont)
	{
		wchar_t tBuf[64];
		if (m_iDelay == (uint32_t)-1)
			swprintf(tBuf, sizeof(tBuf), L"FPS:%.2lf", m_cFPS);
		else
			swprintf(tBuf, sizeof(tBuf), L"�ӳ�:%dms  FPS:%.2lf", m_iDelay, m_cFPS);
		RECT tDrawPos;
		tDrawPos.right = tViewPort.Width - 15;
		tDrawPos.left = 15;
		tDrawPos.top = 5;
		tDrawPos.bottom = tDrawPos.top + 20;
		m_pFPSFont->DrawTextW(NULL, tBuf, -1, &tDrawPos, DT_RIGHT, D3DCOLOR_ARGB(200, 0, 255, 0));
	}

	m_pDev->EndScene();
}
