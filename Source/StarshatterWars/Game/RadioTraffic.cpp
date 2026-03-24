/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         RadioTraffic.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC


	OVERVIEW
	========
	Radio message handler class implementation
*/

#include "RadioTraffic.h"

#include "RadioMessage.h"
#include "RadioView.h"
#include "RadioVox.h"
#include "Instruction.h"
#include "Ship.h"
#include "SimContact.h"
#include "SimElement.h"
#include "Sim.h"
#include "Game.h"
#include "Text.h"

// Unreal logging (replaces Print):
#include "Logging/LogMacros.h"

// Local log category for this translation unit:
DEFINE_LOG_CATEGORY_STATIC(LogRadioTraffic, Log, All);

// +----------------------------------------------------------------------+

RadioTraffic* RadioTraffic::radio_traffic = nullptr;

// +----------------------------------------------------------------------+

RadioTraffic::RadioTraffic()
{
	radio_traffic = this;
}

RadioTraffic::~RadioTraffic()
{
	traffic.destroy();
}

// +----------------------------------------------------------------------+

void
RadioTraffic::Initialize()
{
	if (!radio_traffic)
		radio_traffic = new RadioTraffic;
}

void
RadioTraffic::Close()
{
	delete radio_traffic;
	radio_traffic = nullptr;
}

// +--------------------------------------------------------------------+

void
RadioTraffic::SendQuickMessage(Ship* ship, RadioMessageAction action)
{
	if (!ship) return;

	SimElement* elem = ship->GetElement();

	if (elem) {
		if (action >= RadioMessageAction::REQUEST_PICTURE) {
			Ship* controller = ship->GetController();

			if (controller && !ship->IsStarship()) {
				RadioMessage* msg = new RadioMessage(controller, ship, action);
				Transmit(msg);
			}
		}
		else if (action >= RadioMessageAction::SPLASH_1 && action <= RadioMessageAction::DISTRESS) {
			RadioMessage* msg = new RadioMessage((SimElement*)0, ship, action);
			Transmit(msg);
		}
		else {
			RadioMessage* msg = new RadioMessage(elem, ship, action);

			if (action == RadioMessageAction::ATTACK || action == RadioMessageAction::ESCORT)
				msg->AddTarget(ship->GetTarget());

			Transmit(msg);
		}
	}
}

// +----------------------------------------------------------------------+

void
RadioTraffic::Transmit(RadioMessage* msg)
{
	if (msg && radio_traffic) {
		radio_traffic->SendRadioMessage(msg);
	}
}

// +----------------------------------------------------------------------+

void
RadioTraffic::SendRadioMessage(RadioMessage* msg)
{
	if (!msg) return;

	Sim* sim = Sim::GetSim();
	Ship* player = sim ? sim->GetPlayerShip() : nullptr;
	int   iff = 0;

	if (player)
		iff = player->GetIFF();

	if (msg->DestinationShip()) {
		traffic.append(msg);

		if (msg->GetChannel() == 0 || msg->GetChannel() == iff)
			DisplayMessage(msg);

		msg->DestinationShip()->HandleRadioMessage(msg);
	}

	else if (msg->DestinationElem()) {
		traffic.append(msg);

		if (msg->GetChannel() == 0 || msg->GetChannel() == iff)
			DisplayMessage(msg);

		msg->DestinationElem()->HandleRadioMessage(msg);
	}

	else {
		if (msg->GetChannel() == 0 || msg->GetChannel() == iff)
			DisplayMessage(msg);

		delete msg;
	}
}

// +----------------------------------------------------------------------+

Text
RadioTraffic::TranslateVox(const char* phrase)
{
	Text vox = "vox.";
	vox += phrase;
	vox.toLower();
	vox = Game::GetText(vox);

	if (vox.contains("vox."))  // failed to translate
		return Text(phrase);   // return the original text

	return vox;
}

void
RadioTraffic::DisplayMessage(RadioMessage* msg)
{
	if (!msg) return;

	char txt_buf[256];   txt_buf[0] = 0;
	char msg_buf[128];   msg_buf[0] = 0;
	char src_buf[64];    src_buf[0] = 0;
	char dst_buf[64];    dst_buf[0] = 0;
	char act_buf[64];    act_buf[0] = 0;
	int  vox_channel = 0;

	Ship* dst_ship = msg->DestinationShip();
	SimElement* dst_elem = msg->DestinationElem();

	// BUILD SRC AND DST BUFFERS -------------------

	if (msg->GetSender()) {
		const Ship* sender = msg->GetSender();

		// orders to self?
		if (dst_elem && dst_elem->NumShips() == 1 && dst_elem->GetShip(1) == sender) {
			if (msg->GetRadioAction() >= RadioMessageAction::CALL_ENGAGING) {
				sprintf_s(src_buf, "%s", sender->Name());

				if (sender->IsStarship())
					vox_channel = (sender->Identity() % 3) + 5;
			}
		}

		// orders to other ships:
		else {
			if (sender->IsStarship()) {
				vox_channel = (sender->Identity() % 3) + 5;
			}
			else {
				vox_channel = sender->GetElementIndex();
			}

			if (msg->GetRadioAction() >= RadioMessageAction::CALL_ENGAGING) {
				sprintf_s(src_buf, "%s", sender->Name());
			}
			else {
				sprintf_s(src_buf, "This is %s", sender->Name());

				if (dst_ship) {
					// internal announcement
					if (dst_ship->GetElement() == sender->GetElement()) {
						dst_elem = sender->GetElement();
						int index = sender->GetElementIndex();

						if (index > 1 && dst_elem) {
							sprintf_s(dst_buf, "%s Leader", (const char*)dst_elem->Name());
							sprintf_s(src_buf, "this is %s %d", (const char*)dst_elem->Name(), index);
						}
						else {
							sprintf_s(src_buf, "this is %s leader", (const char*)dst_elem->Name());
						}
					}

					else {
						strcpy_s(dst_buf, (const char*)dst_ship->Name());
						src_buf[0] = tolower(src_buf[0]);
					}
				}

				else if (dst_elem) {
					// flight
					if (dst_elem->NumShips() > 1) {
						sprintf_s(dst_buf, "%s Flight", (const char*)dst_elem->Name());

						// internal announcement
						if (sender->GetElement() == dst_elem) {
							int index = sender->GetElementIndex();

							if (index > 1) {
								sprintf_s(dst_buf, "%s Leader", (const char*)dst_elem->Name());
								sprintf_s(src_buf, "this is %s %d", (const char*)dst_elem->Name(), index);
							}
							else {
								sprintf_s(src_buf, "this is %s leader", (const char*)dst_elem->Name());
							}
						}
					}

					// solo
					else {
						strcpy_s(dst_buf, (const char*)dst_elem->Name());
						src_buf[0] = tolower(src_buf[0]);
					}
				}
			}
		}
	}

	// BUILD ACTION AND TARGET BUFFERS -------------------

	SimObject* target = 0;

	strcpy_s(act_buf, RadioMessage::ActionName(msg->GetRadioAction()));

	if (msg->TargetList().size() > 0)
		target = msg->TargetList()[0];

	if (msg->GetRadioAction() == RadioMessageAction::ACK || msg->GetRadioAction() == RadioMessageAction::NACK) {

		if (dst_ship == msg->GetSender()) {
			src_buf[0] = 0;
			dst_buf[0] = 0;

			if (msg->GetRadioAction() == RadioMessageAction::ACK)
				sprintf_s(msg_buf, "%s.", TranslateVox("Acknowledged").data());
			else
				sprintf_s(msg_buf, "%s.", TranslateVox("Unable").data());
		}
		else if (msg->GetSender()) {
			dst_buf[0] = 0;

			if (msg->GetInfo().length()) {
				sprintf_s(msg_buf, "%s. %s",
					TranslateVox(act_buf).data(),
					(const char*)msg->GetInfo());
			}
			else {
				sprintf_s(msg_buf, "%s.", TranslateVox(act_buf).data());
			}
		}
		else {
			if (msg->GetInfo().length()) {
				sprintf_s(msg_buf, "%s. %s",
					TranslateVox(act_buf).data(),
					(const char*)msg->GetInfo());
			}
			else {
				sprintf_s(msg_buf, "%s.", TranslateVox(act_buf).data());
			}
		}
	}

	else if (msg->GetRadioAction() == RadioMessageAction::MOVE_PATROL) {
		sprintf_s(msg_buf, TranslateVox("Move patrol.").data());
	}

	else if (target && dst_ship && msg->GetSender()) {
		SimContact* c = msg->GetSender()->FindContact(target);

		if (c && c->GetIFF(msg->GetSender()) > 10) {
			sprintf_s(msg_buf, "%s %s.", TranslateVox(act_buf).data(), TranslateVox("unknown contact").data());
		}

		else {
			sprintf_s(msg_buf, "%s %s.",
				TranslateVox(act_buf).data(),
				target->Name());
		}
	}

	else if (target) {
		sprintf_s(msg_buf, "%s %s.",
			TranslateVox(act_buf).data(),
			target->Name());
	}

	else if (msg->GetInfo().length()) {
		sprintf_s(msg_buf, "%s %s",
			TranslateVox(act_buf).data(),
			(const char*)msg->GetInfo());
	}

	else {
		strcpy_s(msg_buf, TranslateVox(act_buf).data());
	}

	const size_t MsgLen = strlen(msg_buf);
	if (MsgLen > 0) {
		const char last_char = msg_buf[MsgLen - 1];
		if (last_char != '!' && last_char != '.' && last_char != '?')
			strcat_s(msg_buf, ".");
	}
	else {
		strcpy_s(msg_buf, ".");
	}

	// final format:
	if (dst_buf[0] && src_buf[0]) {
		sprintf_s(txt_buf, "%s %s. %s", TranslateVox(dst_buf).data(), TranslateVox(src_buf).data(), msg_buf);
		txt_buf[0] = toupper(txt_buf[0]);
	}

	else if (src_buf[0]) {
		sprintf_s(txt_buf, "%s. %s", TranslateVox(src_buf).data(), msg_buf);
		txt_buf[0] = toupper(txt_buf[0]);
	}

	else if (dst_buf[0]) {
		sprintf_s(txt_buf, "%s %s", TranslateVox(dst_buf).data(), msg_buf);
		txt_buf[0] = toupper(txt_buf[0]);
	}

	else {
		strcpy_s(txt_buf, msg_buf);
	}

	// Debug (replaces legacy Print):
	UE_LOG(LogRadioTraffic, Verbose, TEXT("RADIO: %hs"), txt_buf);

	// vox:
	const char* path[8] = { "1", "1", "2", "3", "4", "5", "6", "7" };

	RadioVox* vox = new RadioVox(vox_channel, path[vox_channel], txt_buf);

	if (vox) {
		vox->AddPhrase(dst_buf);
		vox->AddPhrase(src_buf);
		vox->AddPhrase(act_buf);

		if (!vox->Start()) {
			RadioView::Message(txt_buf);
			delete vox;
		}
	}
}

// +----------------------------------------------------------------------+

void
RadioTraffic::DiscardMessages()
{
	if (radio_traffic)
		radio_traffic->traffic.destroy();
}
