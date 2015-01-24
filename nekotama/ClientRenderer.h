#pragma once
#include <chrono>
#include <string>
#include <list>

#include <d3d9.h>
#include <d3dx9.h>

#include <atlbase.h>  // CComPtr

#include <Client.h>

namespace nekotama
{
	class ClientRenderer
	{
		struct Hint
		{
			uint32_t DisplayIndex;
			uint32_t State;
			std::wstring Text;
			std::chrono::microseconds Timer;

			Hint(uint32_t displayIndex, const std::wstring& text)
				: DisplayIndex(displayIndex), State(0), Text(text) {}
		};
		struct GameInfoDisplay
		{
			std::wstring Nickname;
			std::wstring VirtualAddr;
			uint16_t VirtualPort;
			uint32_t Delay;

			GameInfoDisplay() {}
			GameInfoDisplay(const GameInfoDisplay& org)
				: Nickname(org.Nickname), VirtualAddr(org.VirtualAddr), VirtualPort(org.VirtualPort), Delay(org.Delay) {}
			GameInfoDisplay(GameInfoDisplay&& org)
				: Nickname(org.Nickname), VirtualAddr(org.VirtualAddr), VirtualPort(org.VirtualPort), Delay(org.Delay) {}
		};
	private:
		IDirect3DDevice9* m_pDev;
		std::wstring m_ResDir;

		// 资源
		bool m_bDevLost;
		CComPtr<ID3DXFont> m_pFPSFont;
		CComPtr<ID3DXFont> m_pHintFont;
		CComPtr<IDirect3DTexture9> m_pHintBackground;
		CComPtr<IDirect3DTexture9> m_pGameInfoListBackground;
		CComPtr<ID3DXSprite> m_pMainSprite;
		uint32_t m_iGameInfoListBackgroundWidth;
		uint32_t m_iGameInfoListBackgroundHeight;

		// 提示文本
		std::mutex m_GameInfoListMutex;
		std::list<Hint> m_HintList;
		std::vector<GameInfoDisplay> m_GameInfoList;

		bool m_bGameInfoListVisible;
		uint32_t m_iDelay;
		double m_cFPS;
		uint32_t m_cFrameCounter;

		std::chrono::high_resolution_clock::time_point m_cLastTimePoint;
		std::chrono::microseconds m_cFrameDuration;
	public:
		void ShowHint(const std::wstring& text);
		void SetGameInfo(const std::vector<Client::GameInfo>& info);
		void SetDelay(uint32_t iDelay = (uint32_t)-1);
		
		bool IsGameInfoListVisible();
		void HideGameInfoList();
		void ShowGameInfoList();
		
		void DoDeviceLost();
		void DoDeviceReset();

		void Render();
	public:
		ClientRenderer(const std::wstring& resDir, IDirect3DDevice9* pDev);
		~ClientRenderer();
	};
}
