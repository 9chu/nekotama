#pragma once
#include <cstdint>
#include <stdexcept>
#include <limits>

#define NKTM_PACKAGE_CONTENT_MAXSIZE 1024

namespace nekotama
{
	/// @brief 数据包类型
	enum class PackageType
	{
		None = 0
	};

	/// @brief 数据包头部
	struct PackageHeader
	{
		uint32_t SyncBytes;    // 同步字,='NKTM'
		uint16_t PackageType;  // 包类型字段
		uint32_t PackageSize;  // 不含头部的后续数据长度
	};

	/// @brief 数据包验证器
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

	/// @brief 数据包读取器
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
		void Close()
		{
			if (m_CurPos != m_DataContainer.size())
				throw logic_error("PackageReader: not all bytes are readed.")
		}
	public:
		PackageReader(const T& container)
			: m_DataContainer(container), m_CurPos(0), 
			m_PackageType(PackageType::None), m_PackageSize(0)
		{
			readHeader();
		}
	};

	/// @brief 数据包写入器
	/// @note  总是写入末尾
	template<typename T>
	class PackageWriter
	{
	private:
		T& m_DataContainer;
		bool m_bFinishWrite;
		PackageType m_Type;
		size_t m_lenPos;
	private:
		void setBytes(const uint8_t* data, size_t len)
		{
			for (size_t i = 0; i < len; ++i)
				m_DataContainer.push_back(data[i]);
			if (m_DataContainer.size() - m_lenPos > NKTM_PACKAGE_CONTENT_MAXSIZE)
				throw std::logic_error("PackageWriter: package is too big.")
		}
		void prepareHeader()
		{
			uint16_t typeField = (uint16_t)m_Type;
			uint32_t lenField = 0;
			setBytes((const uint8_t*)"NKTM", 4);
			setBytes((const uint8_t*)&typeField, sizeof(uint16_t));
			m_lenPos = m_DataContainer.size();
			setBytes((const uint8_t*)&lenField, sizeof(uint32_t));
		}
		void setLength()
		{
			uint32_t len = (uint32_t)(m_DataContainer.size() - m_lenPos);
			for (size_t i = 0; i < sizeof(uint32_t); ++i)
				m_DataContainer[m_lenPos + i] = ((const uint8_t*)len)[i];
		}
	public:
		template<typename P>
		PackageWriter& operator<<(const P v)
		{
			if (m_bFinishWrite)
				throw std::logic_error("PackageWriter: stream is finished.");
			setBytes((const uint8_t*)&v, sizeof(P));
		}
		PackageWriter& operator<<(const std::string& v)
		{
			if (m_bFinishWrite)
				throw std::logic_error("PackageWriter: stream is finished.");
			if (v.length() > std::numeric_limits<uint8_t>::max())
				throw std::logic_error("PackageWriter: string is too long.");
			uint8_t len = (uint8_t)v.length();
			setBytes((const uint8_t*)&len, sizeof(uint8_t));
			if (v.length() > 0)
				setBytes((const uint8_t*)v.data(), v.length());
		}
		void Close()
		{
			if (m_bFinishWrite)
				throw std::logic_error("PackageWriter: stream is finished.");
			setLength();
			m_bFinishWrite = true;
		}
	public:
		PackageWriter(T& container, PackageType type)
			: m_DataContainer(container), m_bFinishWrite(false), m_Type(type), m_lenPos(0)
		{
			prepareHeader();
		}
	};
}
