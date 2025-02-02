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
#ifndef MANIAC_PLAYER_H
#define MANIAC_PLAYER_H

#include "player.h"


class ManiacPlayer : public Player{

public: 

	ManiacPlayer(ConfigFile*, int id, PlayerType type, std::string name, 
				std::string avatar, int sC, bool aS, bool sotS, int mB);

	~ManiacPlayer();


protected :

	bool preflopShouldCall();
	bool flopShouldCall();
	bool turnShouldCall();
	bool riverShouldCall();

	bool preflopShouldRaise();
	bool flopShouldRaise();
	bool turnShouldRaise();
	bool riverShouldRaise();

	bool flopShouldBet();
	bool turnShouldBet();
	bool riverShouldBet();
};

#endif
