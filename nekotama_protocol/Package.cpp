#include "Package.h"

using namespace std;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

PackageValidChecker::PackageValidChecker()
	: m_State(0), m_PackageType((uint16_t)PackageType::None), m_PackageSize(0), m_InputBytes(0)
{}

PackageValidChecker::UpdateState PackageValidChecker::Update(uint8_t v)
{
	UpdateState tRet = UpdateState::InvalidByte;

	m_InputBytes++;
	switch (m_State)
	{
	case 0:
		if (v == 'N')
		{
			m_State++;
			tRet = UpdateState::ValidByte;
		}
		break;
	case 1:
		if (v == 'K')
		{
			m_State++;
			tRet = UpdateState::ValidByte;
		}
		break;
	case 2:
		if (v == 'T')
		{
			m_State++;
			tRet = UpdateState::ValidByte;
		}
		break;
	case 3:
		if (v == 'M')
		{
			m_State++;
			tRet = UpdateState::ValidByte;
		}
		break;
	case 4:
		((uint8_t*)&m_PackageType)[0] = v;
		m_State++;
		tRet = UpdateState::ValidByte;
		break;
	case 5:
		((uint8_t*)&m_PackageType)[1] = v;
		m_State++;
		tRet = UpdateState::ValidByte;
		break;
	case 6:
		((uint8_t*)&m_PackageSize)[0] = v;
		m_State++;
		tRet = UpdateState::ValidByte;
		break;
	case 7:
		((uint8_t*)&m_PackageSize)[1] = v;
		m_State++;
		tRet = UpdateState::ValidByte;
		break;
	case 8:
		((uint8_t*)&m_PackageSize)[2] = v;
		m_State++;
		tRet = UpdateState::ValidByte;
		break;
	case 9:
		((uint8_t*)&m_PackageSize)[3] = v;
		tRet = UpdateState::EndOfPackageHeader;
		if (m_PackageSize > NKTM_PACKAGE_CONTENT_MAXSIZE)
			return UpdateState::InvalidByte;
		else if (m_PackageSize == 0)
		{
			m_State = 0;
			m_InputBytes = 0;
			tRet = UpdateState::EndOfPackage;
		}
		else
			m_State++;
		break;
	case 10:
		if (m_PackageSize + 10 > m_InputBytes)
			return UpdateState::ValidByte;
		else
			return UpdateState::EndOfPackage;
	}

	if (tRet == UpdateState::InvalidByte)
	{
		m_State = 0;
		m_InputBytes = 0;
	}
	return tRet;
}

////////////////////////////////////////////////////////////////////////////////

