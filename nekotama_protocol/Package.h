#pragma once
#include <stdexcept>
#include <string>

#include <ISocket.h>
#include <Bencode.h>

#define NK_PROTOCOL_MAJOR 0  ///< @brief Э�����汾��
#define NK_PROTOCOL_MINOR 1  ///< @brief Э��ΰ汾��

namespace nekotama
{
	/*
		���ݰ����ͣ�
			1. ��ӭ
				name        ����������
				pmaj        Э�����汾��
				pmin        Э��ΰ汾��
			2. �߳�
				why         ��ӦKickReason��ָ�����������߳���ԭ��
			3. ��½
				nick        �ǳ�
			4. ��½ȷ��
				nick        ��½��ʵ���ǳ�
				addr        �ӷ���˻�õ�����ip��ַ
				port        ��Ϸ�˿ڣ����ڽ�������
			5. PING
			6. PONG
			7. �ǳ�
	*/

	/// @brief ���ݰ�����
	enum class PackageType
	{
		Welcome = 1,
		Kicked = 2,
		Login = 3,
		LoginConfirm = 4,
		Ping = 5,
		Pong = 6,
		Logout = 7
	};

	/// @brief �ͻ��˻Ự�߳�ԭ��
	enum class KickReason
	{
		ServerIsFull,
		ServerClosed,
		LoginFailed
	};

	namespace PackageHelper
	{
		template<typename T>
		static T GetPackageField(const Bencode::Value& v, const std::string& field);

		template<>
		static int GetPackageField(const Bencode::Value& v, const std::string& field)
		{
			if (v.Type != Bencode::ValueType::Dictionary)
				throw std::logic_error("invalid package format.");
			auto tField = v.VDict.find(field);
			if (tField == v.VDict.end())
				throw std::logic_error(StringFormat("invalid package format, field '%s' required.", field.c_str()));
			if (tField->second->Type != Bencode::ValueType::Int)
				throw std::logic_error(StringFormat("invalid package format, field '%s' should be an integer.", field.c_str()));
			return tField->second->VInt;
		}

		template<>
		static const std::string& GetPackageField(const Bencode::Value& v, const std::string& field)
		{
			if (v.Type != Bencode::ValueType::Dictionary)
				throw std::logic_error("invalid package format.");
			auto tField = v.VDict.find(field);
			if (tField == v.VDict.end())
				throw std::logic_error(StringFormat("invalid package format, field '%s' required.", field.c_str()));
			if (tField->second->Type != Bencode::ValueType::String)
				throw std::logic_error(StringFormat("invalid package format, field '%s' should be a string.", field.c_str()));
			return tField->second->VString;
		}
	}
}
