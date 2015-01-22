#pragma once

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
		Timeout,
		LoginFailed
	};
}
