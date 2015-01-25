#include "Client.h"

// ��С�ļ�ʱʱ�䲽
#define TIMETICKSTEP 16

#define CONNECT_TIMETICKSTEP 500
#define CONNECT_TIMEOUT 5000

using namespace std;
using namespace nekotama;
using namespace Bencode;

////////////////////////////////////////////////////////////////////////////////

Client::Client(ISocketFactory* pFactory, ILogger* pLogger, const std::string& serverip, const std::string& nickname, uint16_t port)
	: m_pFactory(pFactory), m_pLogger(pLogger), m_sIP(serverip), m_iPort(port), m_sNickname(nickname),
	m_stopFlag(false), m_bRunning(false), m_iGamePort(0), m_iDelay(0)
{
	// ��ʼ��Socket
	m_pSocket = pFactory->Create(SocketType::TCP);
	m_pSocket->SetBlockingMode(false);
}

Client::~Client()
{
	if (m_threadMain)
	{
		if (m_threadMain->joinable())
		{
			m_stopFlag = true;
			m_threadMain->join();
		}	
	}
}

void Client::Start()
{
	if (m_bRunning)
		throw logic_error("client is running.");
	else
	{
		m_bRunning = true;
		m_threadMain = make_shared<thread>(std::bind(&Client::mainThreadLoop, this));
	}
}

void Client::Stop()
{
	if (m_bRunning)
		m_stopFlag = true;
	else
		throw logic_error("client is not running.");
}

void Client::Wait()
{
	if (m_bRunning)
		m_threadMain->join();
}

bool Client::IsRunning()const NKNOEXCEPT
{
	return m_bRunning;
}

void Client::GentlyStop()
{
	if (m_bRunning)
	{
		// ���͵ǳ����ݰ�
		Value tPackage(ValueType::Dictionary);
		tPackage.VDict["type"] = make_shared<Bencode::Value>((IntType)PackageType::Logout);
		m_sendQueue.Push(tPackage);
	}
}

void Client::NotifyGameCreated()
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Bencode::Value>((IntType)PackageType::CreateHost);
	m_sendQueue.Push(tPackage);
}

void Client::NotifyGameDestroyed()
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Bencode::Value>((IntType)PackageType::DestroyHost);
	m_sendQueue.Push(tPackage);
}

void Client::QueryGameInfo()
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Bencode::Value>((IntType)PackageType::QueryGame);
	m_sendQueue.Push(tPackage);
}

void Client::ForwardingPackage(const std::string& target_addr, uint16_t target_port, uint16_t source_port, const std::string& data)
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)PackageType::SendPackage);
	tPackage.VDict["target"] = make_shared<Value>(target_addr);
	tPackage.VDict["tport"] = make_shared<Value>((IntType)target_port);
	tPackage.VDict["sport"] = make_shared<Value>((IntType)source_port);
	tPackage.VDict["data"] = make_shared<Value>((StringType)data);
	m_sendQueue.Push(tPackage);
}

bool Client::poll(const Bencode::Value& v)
{
	PackageType tPakType = (PackageType)PackageHelper::GetPackageField<int>(v, "type");
	switch (tPakType)
	{
	case PackageType::Welcome:
		m_sServerName = PackageHelper::GetPackageField<const std::string&>(v, "name");
		if (PackageHelper::GetPackageField<int>(v, "pmaj") != NK_PROTOCOL_MAJOR || 
			PackageHelper::GetPackageField<int>(v, "pmin") != NK_PROTOCOL_MINOR)
		{
			OnNotSupportedServerVersion();
			return false;
		}
		else
		{
			// ���͵�½���ݰ�
			Value tPackage(ValueType::Dictionary);
			tPackage.VDict["type"] = make_shared<Bencode::Value>((IntType)PackageType::Login);
			tPackage.VDict["nick"] = make_shared<Bencode::Value>((StringType)m_sNickname);
			m_sendQueue.Push(tPackage);
		}
		break;
	case PackageType::Kicked:
		OnKicked((KickReason)PackageHelper::GetPackageField<int>(v, "why"));
		return false;
	case PackageType::LoginConfirm:
		m_sNickname = PackageHelper::GetPackageField<const std::string&>(v, "nick");
		m_sVirtualAddr = PackageHelper::GetPackageField<const std::string&>(v, "addr");
		m_iGamePort = (uint16_t)PackageHelper::GetPackageField<int>(v, "port");
		OnLoginSucceed(m_sServerName, m_sNickname, m_sVirtualAddr, m_iGamePort);
		break;
	case PackageType::Ping:
		// ����pong���ݰ�
		{
			m_iDelay = PackageHelper::GetPackageField<int>(v, "delay");
			OnDelayRefreshed();

			Value tPackage(ValueType::Dictionary);
			tPackage.VDict["type"] = make_shared<Bencode::Value>((IntType)PackageType::Pong);
			m_sendQueue.Push(tPackage);
		}
		break;
	case PackageType::RecvPackage:
		{
			const string& tSource = PackageHelper::GetPackageField<const string&>(v, "source");
			int tSourcePort = PackageHelper::GetPackageField<int>(v, "sport");
			int tTargetPort = PackageHelper::GetPackageField<int>(v, "tport");
			const string& tData = PackageHelper::GetPackageField<const string&>(v, "data");
			OnDataArrival(tSource, tSourcePort, tTargetPort, tData);
		}
		break;
	case PackageType::GameInfo:
		{
			auto i = v.VDict.find("hosts");
			if (i == v.VDict.end() || i->second->Type != ValueType::List)
				throw logic_error("invalid package data.");
			vector<GameInfo> tGameInfoList;
			for (auto j : i->second->VList)
			{
				GameInfo tInfo;
				tInfo.Nickname = PackageHelper::GetPackageField<const string&>(*j, "nick");
				tInfo.VirtualAddr = PackageHelper::GetPackageField<const string&>(*j, "target");
				tInfo.VirtualPort = PackageHelper::GetPackageField<int>(*j, "port");
				tInfo.Delay = PackageHelper::GetPackageField<int>(*j, "delay");
				tGameInfoList.push_back(tInfo);
			}
			OnRefreshGameInfo(tGameInfoList);
		}
		break;
	}

	return true;
}

void Client::mainThreadLoop()NKNOEXCEPT
{
	m_stopFlag = false;

	// ��������
	try
	{
		m_pSocket->Connect(m_sIP.c_str(), m_iPort);
	}
	catch (const std::exception& e)
	{
		m_pLogger->Log(StringFormat("���ӵ�������ʧ�ܡ�(%s:%u: %s)", m_sIP.c_str(), m_iPort, e.what()), LogType::Error);
		m_bRunning = false;
		OnConnectFailed();
		return;
	}

	// �ȴ����ӽ���
	SocketHandleSet tWriteTestSet;
	chrono::system_clock::time_point tConnectionStart = chrono::system_clock::now();
	while (!m_stopFlag)
	{
		tWriteTestSet.clear();
		tWriteTestSet.insert(m_pSocket);
		if (m_pFactory->Select(nullptr, &tWriteTestSet, nullptr, CONNECT_TIMETICKSTEP))
		{
			break;
		}

		if (CONNECT_TIMEOUT < chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - tConnectionStart).count())
		{
			m_pLogger->Log(StringFormat("���ӵ�������ʧ�ܣ���ʱ��(%s:%u)", m_sIP.c_str(), m_iPort), LogType::Error);
			m_bRunning = false;
			OnConnectFailed();
			return;
		}
	}

	// ��ʼ�������߳�
	m_sendQueue.Clear();
	thread tSendThread(bind(&Client::sendThreadLoop, this));
	
	// ������ѭ��
	Decoder tDecoder;
	SocketHandleSet tReadTestSet;
	while (!m_stopFlag)
	{
		try
		{
			if (m_pSocket->TestIfClosed())
				break;
		}
		catch (const std::exception& e)
		{
			m_pLogger->Log(StringFormat("���������Ƿ����ʱ����(%s)", e.what()), LogType::Error);
			break;
		}

		tReadTestSet.clear();
		tReadTestSet.insert(m_pSocket);
		if (m_pFactory->Select(&tReadTestSet, nullptr, nullptr, TIMETICKSTEP))
		{
			for (auto i : tReadTestSet)
			{
				uint8_t tBuf[512];
				size_t tReaded = 0;
				do
				{
					try
					{
						if (i->Recv(tBuf, sizeof(tBuf), tReaded))
						{
							for (size_t i = 0; i < tReaded; ++i)
							{
								if (tDecoder << tBuf[i])
								{
									try
									{
										if (!poll(*tDecoder))
										{
											// ��������ѭ����
											m_stopFlag = true;
											tReaded = 0;
											break;
										}
									}
									catch (const std::exception& e)  // ���Դ���
									{
										m_pLogger->Log(StringFormat("�������ݰ�ʱ��������(%s)", e.what()), LogType::Error);
										tDecoder.Reset();
									}
								}
							}
						}
						else
							break;
					}
					catch (const std::exception& e)  // ���Դ���
					{
						m_pLogger->Log(StringFormat("��ȡ����ʱ��������(%s)", e.what()), LogType::Error);
						tDecoder.Reset();
					}
				} while (tReaded >= sizeof(tBuf));  // ���ҽ���tReaded == sizeof(tBuf)ʱ˵����������û����
			}
		}
	}

	// ��ֹ���ȴ������߳�ֹͣ����
	m_sendQueue.Push(Value(ValueType::Null));
	tSendThread.join();

	OnLostConnection();
	m_bRunning = false;
}

void Client::sendThreadLoop()NKNOEXCEPT
{
	Encoder tEncoder;
	std::string tDataToSend;
	size_t tLastDataNotSent = 0;
	SocketHandleSet tWriteTestSet;

	while (!m_stopFlag)
	{
		// ����Ҫ����ȡһ�����ݰ�
		while (tLastDataNotSent == 0)
		{
			Value tPackage;
			m_sendQueue.Pop(tPackage);  // ����ֱ����ȡһ�����ݰ�
			if (tPackage.Type == ValueType::Null)  // ���ڱ�ʶ��ֹ����
				return;

			// �������ݰ�
			try
			{
				tDataToSend = tEncoder << tPackage;
				tLastDataNotSent = tDataToSend.length();
			}
			catch (const std::exception& e)  // ���Դ���
			{
				m_pLogger->Log(StringFormat("���뷢�����ݰ�ʧ�ܣ����ݰ���������(%s)", e.what()), LogType::Error);

				tDataToSend.clear();
				tLastDataNotSent = 0;
			}
		}

		// ����Ƿ���Է���
		tWriteTestSet.clear();
		tWriteTestSet.insert(m_pSocket);
		if (m_pFactory->Select(nullptr, &tWriteTestSet, nullptr, TIMETICKSTEP))
		{
			for (auto i : tWriteTestSet)
			{
				// ���͵�ǰ����
				while (tLastDataNotSent > 0)
				{
					size_t wrote = 0;

					try
					{
						if (m_pSocket->Send((uint8_t*)&tDataToSend.data()[tDataToSend.length() - tLastDataNotSent], tLastDataNotSent, wrote))
							tLastDataNotSent -= wrote;
						else
							break;
					}
					catch (const std::exception& e)  // ���Դ���
					{
						m_pLogger->Log(StringFormat("�������ݰ�ʧ�ܣ����ݰ���������(%s)", e.what()), LogType::Error);

						tDataToSend.clear();
						tLastDataNotSent = 0;
					}
				}
			}
		}
	}
}
