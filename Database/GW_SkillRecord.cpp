#include "GW_SkillRecord.h"
#include "WvsUnified.h"
#include "..\Common\Net\InPacket.h"
#include "..\Common\Net\OutPacket.h"
#include "..\WvsGame\WvsGameConstants.hpp"

void GW_SkillRecord::Encode(OutPacket * oPacket)
{
	oPacket->Encode4(nSkillID);
	oPacket->Encode4(nSLV);
	oPacket->Encode8(tExpired);
	if (WvsGameConstants::IsSkillNeedMasterLevel(nSkillID))
		oPacket->Encode4(nMasterLevel);
	if (nSkillID == 0x262A822 || nSkillID == 80000004)
		oPacket->Encode4(0); //unk
}

void GW_SkillRecord::Decode(InPacket * iPacket)
{
	nSkillID = iPacket->Decode4();
}

void GW_SkillRecord::Load(void * pRecordSet)
{
	Poco::Data::RecordSet &recordSet = *((Poco::Data::RecordSet*)pRecordSet);
	nCharacterID = recordSet["CharacterID"];
	nSkillID = recordSet["SkillID"];
	nSLV = recordSet["SLV"];
	nMasterLevel = recordSet["MasterLevel"];
	tExpired = recordSet["Expired"];
}

void GW_SkillRecord::Save()
{
	Poco::Data::Statement queryStatement(GET_DB_SESSION);
	queryStatement << "DELETE FROM SkillRecord Where SkillID = " << nSkillID << " and CharacterID = " << nCharacterID;
	queryStatement.execute();
	
	queryStatement.reset(GET_DB_SESSION);
	queryStatement << "INSERT INTO SkillRecord VALUES(null, "
		<< nCharacterID << ", "
		<< nSkillID << ", "
		<< nSLV << ", "
		<< nMasterLevel << ", "
		<< tExpired << ")";
	queryStatement.execute();
}

