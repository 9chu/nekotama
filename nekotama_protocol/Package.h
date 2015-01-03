#pragma once
#include <cstdint>
#include <stdexcept>

#define NKTM_PACKAGE_CONTENT_MAXSIZE 1024

namespace nekotama
{
	/// @brief ���ݰ�����
	enum class PackageType
	{
		None = 0
	};

	/// @brief ���ݰ�ͷ��
	struct PackageHeader
	{
		uint32_t SyncBytes;    // ͬ����,='NKTM'
		uint16_t PackageType;  // �������ֶ�
		uint32_t PackageSize;  // ����ͷ���ĺ������ݳ���
	};

	/// @brief ���ݰ���֤��
	class PackageValidChecker
	{
	public:
		enum class UpdateState
		{
			ValidByte,
			EndOfPackageHeader,
			EndOfPackage,
			InvalidByte
		};
	private:
		int m_State;
		uint16_t m_PackageType;
		uint32_t m_PackageSize;
		uint32_t m_InputBytes;
	public:
		UpdateState Update(uint8_t v);
	public:
		PackageValidChecker();
	};

	/// @brief ���ݰ���ȡ��
	template<typename T>
	class PackageReader
	{
	private:
		const T& m_DataContainer;
		size_t m_CurPos;

		PackageType m_PackageType;
		size_t m_PackageSize;
	private:
		void getBytes(uint8_t* buffer, size_t len)
		{
			if (m_CurPos + len > m_DataContainer.size())
				throw std::logic_error("PackageReader: data out of range.");
			for (size_t i = 0; i < len; ++i)
			{
				buffer[i] = m_DataContainer[m_CurPos];
				m_CurPos++;
			}
		}
		void readHeader()
		{
			uint8_t flagBytes[5] = { 0 };
			getBytes(flagBytes, sizeof(flagBytes) - 1);
			if (strcmp((const char*)flagBytes, "NKTM") != 0)
				throw std::logic_error("PackageReader: invalid synchronization word.");
			uint16_t typeField;
			getBytes((uint8_t*)&typeField, sizeof(uint16_t));
			m_PackageType = (PackageType)typeField;
			getBytes((uint8_t*)&m_PackageSize, sizeof(uint32_t));
			if (m_PackageSize > NKTM_PACKAGE_CONTENT_MAXSIZE)
				throw std::logic_error("PackageReader: package content is too big.");
		}
	public:
		PackageType GetPackageType()const { return m_PackageType; }
		size_t GetPackageSize()const { return m_PackageSize; }

		template<typename P>
		PackageReader& operator>>(P& v)
		{
			getBytes((uint8_t*)&v, sizeof(P));
			return *this;
		}
		PackageReader& operator>>(std::string& v)
		{
			uint8_t len;
			getBytes((uint8_t*)&v, sizeof(uint8_t));
			v.clear();
			if (len > 0)
			{
				v.resize(len);
				getBytes((uint8_t*)v.data(), len);
			}
			return *this;
		}
	public:
		PackageReader(const T& container, size_t pos = 0)
			: m_DataContainer(container), m_CurPos(pos), 
			m_PackageType(PackageType::None), m_PackageSize(0)
		{
			readHeader();
		}
	};

	/// @brief ���ݰ�д����
	/// @note  ����д��ĩβ
	/*
	template<typename T>
	class PackageWriter
	{
	private:
		T& m_DataContainer;
		bool m_bFinishWrite;
		PackageType m_Type;
	public:
		PackageWriter& operator<<(uint8_t v)
		{
			if (m_bFinishWrite)
				throw std::logic_error("stream is finished.");
		}
		PackageWriter& operator<<(uint16_t v)
		{
			if (m_bFinishWrite)
				throw std::logic_error("stream is finished.");
		}
		PackageWriter& operator<<(uint32_t v)
		{
			if (m_bFinishWrite)
				throw std::logic_error("stream is finished.");
		}
		PackageWriter& operator<<(const std::string& v)
		{
			if (m_bFinishWrite)
				throw std::logic_error("stream is finished.");
		}
		PackageWriter& operator<<(const std::endl& v)
		{
			if (m_bFinishWrite)
				throw std::logic_error("stream is finished.");
			// ʲô������
		}
	public:
		PackageWriter(T& container, PackageType type)
			: m_DataContainer(container), m_bFinishWrite(false), m_Type(type) {}
	};
	*/
}
