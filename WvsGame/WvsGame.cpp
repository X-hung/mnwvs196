#include "WvsGame.h"
#include "..\WvsLib\Constants\WvsGameConstants.hpp"
#include "..\WvsLib\Task\AsnycScheduler.h"
#include "..\WvsLib\Constants\ConfigLoader.hpp"
#include "ClientSocket.h"
#include "..\WvsLib\Logger\WvsLogger.h"

WvsGame::WvsGame()
{

}

WvsGame::~WvsGame()
{

}

std::shared_ptr<Center>& WvsGame::GetCenter()
{
	return m_pCenterInstance;
}

void WvsGame::ConnectToCenter(int nCenterIdx)
{
	m_pCenterInstance->SetSocketDisconnectedCallBack(std::bind(&Center::OnNotifyCenterDisconnected, m_pCenterInstance.get()));
	m_pCenterInstance->SetCenterIndex(nCenterIdx);
	m_pCenterInstance->Connect(
		ConfigLoader::GetInstance()->StrValue("Center" + std::to_string(nCenterIdx) + "_IP"),
		ConfigLoader::GetInstance()->IntValue("Center" + std::to_string(nCenterIdx) + "_Port")
	);
}

void WvsGame::CenterAliveMonitor()
{
	if (m_pCenterInstance->CheckSocketStatus(SocketBase::SocketStatus::eConnecting))
		return;
	WvsLogger::LogRaw(WvsLogger::LEVEL_WARNING, "=================定期檢查Center Server連線程序=================\n");
	if (m_pCenterInstance && m_pCenterInstance->CheckSocketStatus(SocketBase::SocketStatus::eClosed))
	{
		WvsLogger::LogFormat(WvsLogger::LEVEL_ERROR, "Center Server %d 尚未連線，嘗試重新連線。\n", 0);
		m_pCenterWorkThread->detach();
		*m_pCenterWorkThread = std::thread(&WvsGame::ConnectToCenter, this, 0);
	}
}

void WvsGame::InitializeCenter()
{
	m_nChannelID = ConfigLoader::GetInstance()->IntValue("ChannelID");
	m_pCenterServerService = new asio::io_service();
	m_pCenterInstance = std::make_shared<Center>(*m_pCenterServerService);
	m_pCenterWorkThread = new std::thread(&WvsGame::ConnectToCenter, this, 0);
	auto holderFunc = std::bind(&WvsGame::CenterAliveMonitor, this);
	auto aliveHolder = AsnycScheduler::CreateTask(holderFunc, 10 * 1000, true);
	aliveHolder->Start();


	std::thread tCenterWorkThread([&] {
		asio::io_service::work work(*m_pCenterServerService);
		std::error_code ec;
		m_pCenterServerService->run(ec);
	});
	tCenterWorkThread.detach();
}

void WvsGame::OnUserConnected(std::shared_ptr<User> &pUser)
{
	std::lock_guard<std::mutex> lockGuard(m_mUserLock);
	m_mUserMap[pUser->GetUserID()] = pUser;
}

void WvsGame::OnNotifySocketDisconnected(SocketBase *pSocket)
{
	std::lock_guard<std::mutex> lockGuard(m_mUserLock);
	auto pClient = (ClientSocket*)pSocket;
	if (pClient->GetUser())
	{
		m_mUserMap.erase(pClient->GetUser()->GetUserID());
		pClient->SetUser(nullptr);
	}
}

int WvsGame::GetChannelID() const
{
	return m_nChannelID;
}

User * WvsGame::FindUser(int nUserID)
{
	std::lock_guard<std::mutex> lockGuard(m_mUserLock);
	auto findIter = m_mUserMap.find(nUserID);
	if (findIter == m_mUserMap.end())
		return nullptr;
	return findIter->second.get();
}
