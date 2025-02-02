/*****************************************************************************
 * Raise73 - Texas Holdem No Limit software, offline game against custom AIs *
 * Copyright (C) 2024 Marc Ennaji                                            *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/
/* Game data. */

#ifndef _GAMEDATA_H_
#define _GAMEDATA_H_

#include <list>
#include <string>


typedef std::list<unsigned> PlayerIdList;

enum GameMode {
	GAME_MODE_CREATED = 1,
	GAME_MODE_STARTED,
	GAME_MODE_CLOSED
};

enum GameType {
	GAME_TYPE_NORMAL = 1,
	GAME_TYPE_REGISTERED_ONLY,
	GAME_TYPE_INVITE_ONLY,
	GAME_TYPE_RANKING
};

enum RaiseIntervalMode {
	RAISE_ON_HANDNUMBER = 1,
	RAISE_ON_MINUTES
};

enum RaiseMode {
	DOUBLE_BLINDS = 1,
	MANUAL_BLINDS_ORDER
};

enum AfterManualBlindsMode {
	AFTERMB_DOUBLE_BLINDS = 1,
	AFTERMB_RAISE_ABOUT,
	AFTERMB_STAY_AT_LAST_BLIND
};

enum TableProfile {
	RANDOM_OPPONENTS = 1,
	TIGHT_AGRESSIVE_OPPONENTS,
	LARGE_AGRESSIVE_OPPONENTS,
	MANIAC_OPPONENTS
};

// For the sake of simplicity, this is a struct.
struct GameData {
	GameData() : gameType(GAME_TYPE_NORMAL), maxNumberOfPlayers(0), startMoney(0),
		firstSmallBlind(0), raiseIntervalMode(RAISE_ON_HANDNUMBER),
		raiseSmallBlindEveryHandsValue(8), raiseSmallBlindEveryMinutesValue(1),
		raiseMode(DOUBLE_BLINDS), afterManualBlindsMode(AFTERMB_DOUBLE_BLINDS),
		afterMBAlwaysRaiseValue(0), guiSpeed(9), delayBetweenHandsSec(6),
		playerActionTimeoutSec(20) {}
	GameType gameType;
	TableProfile tableProfile;
	int maxNumberOfPlayers;
	int startMoney;
	int firstSmallBlind;
	RaiseIntervalMode raiseIntervalMode;
	int raiseSmallBlindEveryHandsValue;
	int raiseSmallBlindEveryMinutesValue;
	RaiseMode raiseMode;
	std::list<int> manualBlindsList;
	AfterManualBlindsMode afterManualBlindsMode;
	int afterMBAlwaysRaiseValue;
	int guiSpeed;
	int delayBetweenHandsSec;
	int playerActionTimeoutSec;
};

struct GameInfo {
	GameInfo() : mode(GAME_MODE_CREATED), adminPlayerId(0), isPasswordProtected(false) {}
	std::string name;
	GameData data;
	GameMode mode;
	unsigned adminPlayerId;
	PlayerIdList players;
	bool isPasswordProtected;
};

struct StartData {
	StartData() : startDealerPlayerId(0), numberOfPlayers(0) {}
	unsigned startDealerPlayerId;
	int numberOfPlayers;
};

#endif

