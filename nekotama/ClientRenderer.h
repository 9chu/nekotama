#pragma once
#include <chrono>
#include <string>
#include <list>

#include <d3d9.h>
#include <d3dx9.h>

#include <atlbase.h>  // CComPtr

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
	private:
		IDirect3DDevice9* m_pDev;
		std::wstring m_ResDir;

		// 资源
		CComPtr<ID3DXFont> m_pFPSFont;
		CComPtr<ID3DXFont> m_pHintFont;
		CComPtr<IDirect3DTexture9> m_pHintBackground;
		CComPtr<ID3DXSprite> m_pHintSprite;

		// 提示文本
		std::list<Hint> m_HintList;

		uint32_t m_iDelay;
		double m_cFPS;
		uint32_t m_cFrameCounter;

		std::chrono::high_resolution_clock::time_point m_cLastTimePoint;
		std::chrono::microseconds m_cFrameDuration;
	public:
		void ShowHint(const std::wstring& text);
		void SetDelay(uint32_t iDelay = (uint32_t)-1);
		
		void DoDeviceLost();
		void DoDeviceReset();

		void Render();
	public:
		ClientRenderer(const std::wstring& resDir, IDirect3DDevice9* pDev);
		~ClientRenderer();
	};
}
