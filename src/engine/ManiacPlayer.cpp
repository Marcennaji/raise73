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
#include <engine/ManiacPlayer.h>

#include <engine/handinterface.h>
#include <engine/tools.h>
#include <engine/cardsvalue.h>
#include <configfile.h>
#include <loghelper.h>
//#include <engine/psim/simulate.hpp>
#include "Exception.h"
#include "engine_msg.h"
#include "ranges.h"

#include <qt/guiwrapper.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

using namespace std;

ManiacPlayer::ManiacPlayer(ConfigFile *c, int id, PlayerType type, std::string name, 
						std::string avatar, int sC, bool aS, bool sotS, int mB):
	Player(c, id, type, name, avatar, sC, aS, sotS, mB){
	
	// initialize utg starting range, in a full table 
	int utgFullTableRange = 0;
	Tools::GetRand(30, 35, 1, &utgFullTableRange);
	initializeRanges(50, utgFullTableRange);
	
}

	
ManiacPlayer::~ManiacPlayer(){}


bool ManiacPlayer::preflopShouldCall(){

	const int nbRaises = currentHand->getPreflopRaisesNumber();
	const int nbCalls = currentHand->getPreflopCallsNumber();
	const int nbPlayers = currentHand->getActivePlayerList()->size(); 

	float callingRange = getPreflopCallingRange();

	if (callingRange == -1)
		return false;// never call : raise or fold

	string stringCallingRange;

	const char * * RANGES_STRING;

	if (nbPlayers == 2)
		RANGES_STRING = TOP_RANGE_2_PLAYERS;
	else if (nbPlayers == 3)
		RANGES_STRING = TOP_RANGE_3_PLAYERS;
	else if (nbPlayers == 4)
		RANGES_STRING = TOP_RANGE_4_PLAYERS;
	else RANGES_STRING = TOP_RANGE_MORE_4_PLAYERS;

	stringCallingRange = RANGES_STRING[(int)callingRange];

	if (nbRaises < 3){
		stringCallingRange += HIGH_PAIRS;
#ifdef LOG_POKER_EXEC
		cout << "\t\tManiac adding high pairs to the initial calling range." << endl;
#endif
	}
	
	const int lastRaiserID = currentHand->getPreflopLastRaiserID();
	std::shared_ptr<Player> lastRaiser = getPlayerByUniqueId(lastRaiserID);
	
	if (nbRaises < 2 &&
		getCash() >= currentHand->getBoard()->getPot() * 10 &&
		lastRaiser != NULL && 
		lastRaiser->getCash() >= currentHand->getBoard()->getPot() * 10 &&
		! isPreflopBigBet()){

		stringCallingRange += HIGH_SUITED_CONNECTORS;
		stringCallingRange += HIGH_SUITED_ACES;
		stringCallingRange += PAIRS;

#ifdef LOG_POKER_EXEC
		cout << "\t\tManiac adding high suited connectors, high suited aces and pairs to the initial calling range." << endl;
#endif
		if (currentHand->getRunningPlayerList()->size() > 2 && nbRaises + nbCalls > 1 && myPosition >= MIDDLE){
			stringCallingRange += CONNECTORS;
			stringCallingRange += SUITED_ONE_GAPED;
			stringCallingRange += SUITED_TWO_GAPED;
#ifdef LOG_POKER_EXEC
		cout << "\t\tManiac adding connectors, suited one-gaped and suited two-gaped to the initial calling range." << endl;
#endif
		}
	}

	// defend against 3bet bluffs :
	if (nbRaises == 2 &&
		myCurrentHandActions.getPreflopActions().size() > 0 &&
		myCurrentHandActions.getPreflopActions().back() == PLAYER_ACTION_RAISE &&
		getCash() >= currentHand->getBoard()->getPot() * 10 &&
		lastRaiser != NULL && 
		lastRaiser->getCash() >= currentHand->getBoard()->getPot() * 10 &&
		! isPreflopBigBet()){

		int rand = 0;
		Tools::GetRand(1, 3, 1, &rand);
		if (rand == 1){

			stringCallingRange += HIGH_SUITED_CONNECTORS;
			stringCallingRange += HIGH_SUITED_ACES;
			stringCallingRange += PAIRS;

	#ifdef LOG_POKER_EXEC
			cout << "\t\tManiac defending against 3-bet : adding high suited connectors, high suited aces and pairs to the initial calling range." << endl;
	#endif
		}
	}

#ifdef LOG_POKER_EXEC
	cout << "\t\tManiac final calling range : " << stringCallingRange << endl;
#endif

	return isCardsInRange(myCard1, myCard2, stringCallingRange);

}

bool ManiacPlayer::preflopShouldRaise(){

	const int nbRaises = currentHand->getPreflopRaisesNumber();
	const int nbCalls = currentHand->getPreflopCallsNumber();
	const int nbPlayers = currentHand->getActivePlayerList()->size(); 

	float raisingRange = getPreflopRaisingRange();

	if (raisingRange == -1)
		return false;// never raise : call or fold

	if (nbRaises > 3)
		return false;// never 6-bet : call or fold

	string stringRaisingRange;

	const char * * RANGES_STRING;

	if (nbPlayers == 2)
		RANGES_STRING = TOP_RANGE_2_PLAYERS;
	else if (nbPlayers == 3)
		RANGES_STRING = TOP_RANGE_3_PLAYERS;
	else if (nbPlayers == 4)
		RANGES_STRING = TOP_RANGE_4_PLAYERS;
	else RANGES_STRING = TOP_RANGE_MORE_4_PLAYERS;

	stringRaisingRange = RANGES_STRING[(int)raisingRange];

#ifdef LOG_POKER_EXEC
	cout << stringRaisingRange << endl;
#endif

	// determine when to 3-bet without a real hand
	bool speculativeHandedAdded = false;

	if (nbRaises == 1){

		const int lastRaiserID = currentHand->getPreflopLastRaiserID();
		std::shared_ptr<Player> lastRaiser = getPlayerByUniqueId(lastRaiserID);
		PreflopStatistics raiserStats = lastRaiser->getStatistics(nbPlayers).getPreflopStatistics();

		if (! isCardsInRange(myCard1, myCard2, stringRaisingRange) &&
			getM() > 20 &&
			myCash > currentHand->getCurrentBettingRound()->getHighestSet() * 20 &&
			myPosition > UTG_PLUS_TWO &&
			raiserStats.m_hands > MIN_HANDS_STATISTICS_ACCURATE &&
			myPosition > lastRaiser->getPosition() &&
			lastRaiser->getCash() > currentHand->getCurrentBettingRound()->getHighestSet() * 10 &&
			! isPreflopBigBet() &&
			nbCalls < 2){

			if (canBluff(GAME_STATE_PREFLOP) &&
				myPosition > LATE && 
				nbRaises == 1 &&
				! isCardsInRange(myCard1, myCard2, ACES + BROADWAYS) &&
					raiserStats.getPreflopCall3BetsFrequency() < 30){ 				
				
				speculativeHandedAdded = true;
#ifdef LOG_POKER_EXEC
				cout << "\t\tManiac trying to steal this bet";
#endif
			}else{

				if (isCardsInRange(myCard1, myCard2, LOW_PAIRS + SUITED_CONNECTORS + SUITED_ONE_GAPED + SUITED_TWO_GAPED)){ 

					speculativeHandedAdded = true;
#ifdef LOG_POKER_EXEC
					cout << "\t\tManiac adding this speculative hand to our initial raising range";
#endif
				}else{
					if (! isCardsInRange(myCard1, myCard2, PAIRS + ACES + BROADWAYS) && 
						raiserStats.getPreflopCall3BetsFrequency() < 30){

						int rand = 0;
						Tools::GetRand(1, 3, 1, &rand);
						if (rand == 1){
							speculativeHandedAdded = true;
#ifdef LOG_POKER_EXEC
							cout << "\t\tManiac adding this junk hand to our initial raising range";
#endif
						}
					}
				}
			}
		}
	}

	// determine when to 4-bet without a real hand
	if (! speculativeHandedAdded && nbRaises == 2){

		const int lastRaiserID = currentHand->getPreflopLastRaiserID();
		std::shared_ptr<Player> lastRaiser = getPlayerByUniqueId(lastRaiserID);
		PreflopStatistics raiserStats = lastRaiser->getStatistics(nbPlayers).getPreflopStatistics();

		if (! isCardsInRange(myCard1, myCard2, stringRaisingRange) &&
			! isCardsInRange(myCard1, myCard2, OFFSUITED_BROADWAYS) &&
			getM() > 20 &&
			myCash > currentHand->getCurrentBettingRound()->getHighestSet() * 60 &&
			myPosition > MIDDLE_PLUS_ONE &&
			raiserStats.m_hands > MIN_HANDS_STATISTICS_ACCURATE &&
			myPosition > lastRaiser->getPosition() &&
			lastRaiser->getCash() > currentHand->getCurrentBettingRound()->getHighestSet() * 20 &&
			! isPreflopBigBet() && 
			nbCalls < 2){

			if (canBluff(GAME_STATE_PREFLOP) &&
				myPosition > LATE &&
				raiserStats.getPreflop3Bet() > 8){

				int rand = 0;
				Tools::GetRand(1, 5, 1, &rand);
				if (rand == 1){
					speculativeHandedAdded = true;
#ifdef LOG_POKER_EXEC
					cout << "\t\tManiac adding this speculative hand to our initial raising range";
#endif
				}
			}
		}
	}

	if (! speculativeHandedAdded && ! isCardsInRange(myCard1, myCard2, stringRaisingRange))
		return false;

	// sometimes, just call a single raise instead of raising, even with a strong hand
	if (! speculativeHandedAdded && 
		currentHand->getPreflopCallsNumber() == 0 &&
		currentHand->getPreflopRaisesNumber() == 1 &&
		raisingRange < 100 && 
		! (isCardsInRange(myCard1, myCard2, LOW_PAIRS + MEDIUM_PAIRS) && nbPlayers < 4) &&
		! (isCardsInRange(myCard1, myCard2, HIGH_PAIRS) && nbCalls > 0) &&
		isCardsInRange(myCard1, myCard2, getStringRange(nbPlayers, 4))){

		int rand = 0;
		Tools::GetRand(1, 10, 1, &rand);
		if (rand == 1){
#ifdef LOG_POKER_EXEC
			cout << "\t\twon't raise " << myCard1 << " " << myCard2 << " , to hide the hand strength";
#endif
			myShouldCall = true;
			return false;
		}
	}

	computePreflopRaiseAmount();

	return true;
}



bool ManiacPlayer::flopShouldBet(){

	const int pot = currentHand->getBoard()->getPot() + currentHand->getBoard()->getSets();
	PlayerList runningPlayers = currentHand->getRunningPlayerList();
	const int nbPlayers = currentHand->getActivePlayerList()->size(); 
	const int lastRaiserID = currentHand->getPreflopLastRaiserID();
	const bool bHavePosition = getHavePosition(myPosition, currentHand->getRunningPlayerList());

	assert(nbPlayers >= 2);

	const int nbPreflopRaises = currentHand->getPreflopRaisesNumber();
	const int nbFlopRaises = currentHand->getFlopBetsOrRaisesNumber();

	if (nbFlopRaises > 0)
		return false;

	if (shouldPotControl(myFlopState, myFlopHandSimulation, GAME_STATE_FLOP))
		return false;

	// donk bets :
	if (nbPreflopRaises > 0 && lastRaiserID != myID){

		std::shared_ptr<Player> lastRaiser = getPlayerByUniqueId(lastRaiserID);

		if (lastRaiser->getPosition() > myPosition){

			if (getDrawingProbability() > 25){
				int rand = 0;
				Tools::GetRand(1, 2, 1, &rand);
				if (rand == 1){
					myBetAmount = pot * 0.6;
					return true;
				}
			}

			if ((myFlopState.IsTwoPair || myFlopState.IsTrips || myFlopState.IsStraight) && myFlopState.IsFlushDrawPossible){
				myBetAmount = pot * 0.6;
				return true;
			}

			// if the flop is dry, try to get the pot
			if (nbPlayers < 3 && canBluff(GAME_STATE_FLOP) && 
				getBoardCardsHigherThan("Jh") < 2 && getBoardCardsHigherThan("Kh") == 0 &&
				! myFlopState.IsFlushDrawPossible){

				int rand = 0;
				Tools::GetRand(1, 2, 1, &rand);
				if (rand == 1){
					myBetAmount = pot * 0.6;
					return true;
				}
			}
		}
	}
	// don't bet if in position, and pretty good drawing probs
	if (getDrawingProbability() > 20 && bHavePosition)
		return false;


	// if pretty good hand
	if (myFlopHandSimulation.winRanged > 0.5 || myFlopHandSimulation.win > 0.9){

		// always bet if my hand will lose a lot of its value on next betting rounds
		if (myFlopHandSimulation.winRanged - myFlopHandSimulation.winSd > 0.1){
			myBetAmount = pot * 0.8;
			return true;
		}

		// if no raise preflop, or if more than 1 opponent
		if (currentHand->getPreflopRaisesNumber() == 0 || runningPlayers->size() > 2){

			if (runningPlayers->size() < 4)
				myBetAmount = pot * 0.8;
			else
				myBetAmount = pot * 1.2;
			return true;
		}

		// if i have raised preflop, bet
		if (lastRaiserID == myID && currentHand->getPreflopRaisesNumber() > 0){
			myBetAmount = pot * 0.8;
			return true;
		}

	}else{

		///////////  if bad flop for me

		// if there was a lot of action preflop, and i was not the last raiser : don't bet
		if (nbPreflopRaises > 2 && lastRaiserID != myID)
			return false;

		// if I was the last raiser preflop, I may bet with not much
		if (lastRaiserID == myID && runningPlayers->size() < 4 && myCash > pot * 4 && canBluff(GAME_STATE_FLOP)){

			if (myFlopHandSimulation.winRanged > 0.15 && myFlopHandSimulation.win > 0.3){ 

				myBetAmount = pot * 0.8;		
				return true;
			}
		}
	}

	return false;

}
bool ManiacPlayer::flopShouldCall(){

	const int nbRaises = currentHand->getFlopBetsOrRaisesNumber();

	if (nbRaises == 0)
		return false;

	if (isDrawingProbOk())
		return true;

	if (myFlopHandSimulation.winRanged == 1 && myFlopHandSimulation.win > 0.5)
		return true;

	if (myFlopHandSimulation.winRanged * 100 < getPotOdd() * 0.8 && myFlopHandSimulation.win < 0.9)
		return false;

	if (myFlopHandSimulation.winRanged < 0.25)
		return false;

	return true; 
}

bool ManiacPlayer::flopShouldRaise(){

	const int pot = currentHand->getBoard()->getPot() + currentHand->getBoard()->getSets();
	PlayerList runningPlayers = currentHand->getRunningPlayerList();
	const bool bHavePosition = getHavePosition(myPosition, currentHand->getRunningPlayerList());
	const int potOdd = getPotOdd();
	const int nbPlayers = currentHand->getActivePlayerList()->size(); 
	assert(nbPlayers >= 2);

	const int nbRaises = currentHand->getFlopBetsOrRaisesNumber();

	if (nbRaises == 0)
		return false;

	if (shouldPotControl(myFlopState, myFlopHandSimulation, GAME_STATE_FLOP))
		return false;

	if (nbRaises < 2 && runningPlayers->size() < 4 && canBluff(GAME_STATE_FLOP) && myFlopHandSimulation.winRanged < 0.3 &&
		getBoardCardsHigherThan("Jh") < 2 && getBoardCardsHigherThan("Kh") == 0){

		int rand = 0;
		Tools::GetRand(1, 3, 1, &rand);
		if (rand == 2){
			myRaiseAmount = pot * 2;
			return true;
		}
	}

	if (nbRaises == 2 && myFlopHandSimulation.win < 0.95)
		return false;

	if (nbRaises == 3 && myFlopHandSimulation.win < 0.98)
		return false;

	if (nbRaises > 3 && myFlopHandSimulation.win != 1)
		return false;

	if ((isDrawingProbOk() || bHavePosition) && runningPlayers->size() == 2 &&
		! (myFlopHandSimulation.winRanged * 100 < getPotOdd()) &&
		canBluff(GAME_STATE_FLOP) &&
		nbRaises < 2){

		int rand = 0;
		Tools::GetRand(1, 2, 1, &rand);
		if (rand == 2){
			myRaiseAmount = pot;
			return true;
		}
	}

	if (myFlopHandSimulation.winRanged > 0.9 && myFlopHandSimulation.win > 0.5 && nbRaises < 3){
		myRaiseAmount = pot;
		return true;
	}

	if (myFlopHandSimulation.winRanged > 0.8 && myFlopHandSimulation.win > 0.5 && nbRaises < 2){
		myRaiseAmount = pot;
		return true;
	}

	return false;
}

bool ManiacPlayer::turnShouldBet(){

	const bool bHavePosition = getHavePosition(myPosition, currentHand->getRunningPlayerList());

	const int pot = currentHand->getBoard()->getPot() + currentHand->getBoard()->getSets();
	PlayerList runningPlayers = currentHand->getRunningPlayerList();
	const int nbRaises = currentHand->getTurnBetsOrRaisesNumber();

	if (nbRaises > 0)
		return false;

	if (shouldPotControl(myTurnState, myTurnHandSimulation, GAME_STATE_TURN))
		return false;

	if (currentHand->getFlopBetsOrRaisesNumber() > 1 && ! isAgressor(GAME_STATE_FLOP) && myTurnHandSimulation.winRanged < 0.8)
		return false;

	if (currentHand->getFlopBetsOrRaisesNumber() == 0 && bHavePosition){
		myBetAmount = pot * 0.8;
		return true;
	}

	if (myTurnHandSimulation.winRanged < 0.3 && myTurnHandSimulation.win < 0.9 && ! bHavePosition)
		return false;

	if (myTurnHandSimulation.winRanged > 0.4 && myTurnHandSimulation.win > 0.5 && bHavePosition){
		myBetAmount = pot * 0.8;
		return true;
	}

	if (getDrawingProbability() > 15 && ! bHavePosition){
		myBetAmount = pot * 0.8;
		return true;
	}else{
		// no draw, not a good hand, but last to speak and nobody has bet
		if (bHavePosition && canBluff(GAME_STATE_TURN)){
			myBetAmount = pot * 0.8;
			return true;
		}
	}

	return false;
}

bool ManiacPlayer::turnShouldCall(){

	const int pot = currentHand->getBoard()->getPot() + currentHand->getBoard()->getSets();
	PlayerList runningPlayers = currentHand->getRunningPlayerList();
	std::vector<PlayerPosition> raisersPositions = currentHand->getRaisersPositions();
	std::vector<PlayerPosition> callersPositions = currentHand->getCallersPositions();
	const int highestSet = currentHand->getCurrentBettingRound()->getHighestSet();
	const int nbRaises = currentHand->getTurnBetsOrRaisesNumber();
	const int potOdd = getPotOdd();
	const bool bHavePosition = getHavePosition(myPosition, currentHand->getRunningPlayerList());
	const int lastRaiserID = currentHand->getLastRaiserID();
	std::shared_ptr<Player> lastRaiser = getPlayerByUniqueId(lastRaiserID);
	const int nbPlayers = currentHand->getActivePlayerList()->size();
	
	if (nbRaises == 0)
		return false;

	if (isDrawingProbOk())
		return true;
	
	assert(lastRaiser != NULL);

	TurnStatistics raiserStats = lastRaiser->getStatistics(nbPlayers).getTurnStatistics();

	// if not enough hands, then try to use the statistics collected for (nbPlayers + 1), they should be more accurate
	if (raiserStats.m_hands < MIN_HANDS_STATISTICS_ACCURATE && nbPlayers < 10 && 
		lastRaiser->getStatistics(nbPlayers + 1).getTurnStatistics().m_hands > MIN_HANDS_STATISTICS_ACCURATE)

		raiserStats = lastRaiser->getStatistics(nbPlayers + 1).getTurnStatistics();

	if (myTurnHandSimulation.winRanged * 100 < getPotOdd() && myTurnHandSimulation.winRanged < 0.94){
		return false;
	}

	if (nbRaises == 2 && myTurnHandSimulation.winRanged < 0.7 && myTurnHandSimulation.win < 0.9){
		if (raiserStats.m_hands <= MIN_HANDS_STATISTICS_ACCURATE)
			return false;
		if (raiserStats.getAgressionFrequency() < 20)
			return false;
	}
	if (nbRaises > 2 && myTurnHandSimulation.winRanged < 0.8 && myTurnHandSimulation.win < 0.9){
		if (raiserStats.m_hands <= MIN_HANDS_STATISTICS_ACCURATE)
			return false;
		if (raiserStats.getAgressionFrequency() < 20)
			return false;
	}

	if (myTurnHandSimulation.winRanged < 0.5 && (currentHand->getFlopBetsOrRaisesNumber() > 0 || raiserStats.getAgressionFrequency() < 30))
		return false;

	if (! isAgressor(GAME_STATE_PREFLOP) && ! isAgressor(GAME_STATE_FLOP) 
		&& myTurnHandSimulation.winRanged < 0.7 && raiserStats.getAgressionFrequency() < 30 && ! bHavePosition)
		return false;

	if (myTurnHandSimulation.winRanged < 0.25 && myTurnHandSimulation.win < 0.9)
		return false;

	return true;
}

bool ManiacPlayer::turnShouldRaise(){

	const int pot = currentHand->getBoard()->getPot() + currentHand->getBoard()->getSets();
	PlayerList runningPlayers = currentHand->getRunningPlayerList();
	std::vector<PlayerPosition> raisersPositions = currentHand->getRaisersPositions();
	std::vector<PlayerPosition> callersPositions = currentHand->getCallersPositions();
	const int highestSet = currentHand->getCurrentBettingRound()->getHighestSet();
	const int nbRaises = currentHand->getTurnBetsOrRaisesNumber();
	const bool bHavePosition = getHavePosition(myPosition, currentHand->getRunningPlayerList());

	if (nbRaises == 0)
		return false;

	if (shouldPotControl(myTurnState, myTurnHandSimulation, GAME_STATE_TURN))
		return false;

	if (nbRaises == 1 && myTurnHandSimulation.win < 0.9)
		return false;

	if (nbRaises == 2 && myTurnHandSimulation.win < 0.94)
		return false;

	if (nbRaises > 2 && myTurnHandSimulation.win != 1)
		return false;

	if (myTurnHandSimulation.win == 1 || (myTurnHandSimulation.winRanged == 1 && nbRaises < 3)){
		myRaiseAmount = pot * 0.6;
		return true;
	}

	if (myTurnHandSimulation.winRanged * 100 < getPotOdd() && myTurnHandSimulation.winRanged < 0.94)
		return false;

	if (myTurnHandSimulation.winRanged > 0.6 && myTurnHandSimulation.win > 0.6 &&
		nbRaises == 1 && currentHand->getFlopBetsOrRaisesNumber() < 2){

		if (runningPlayers->size() < 3){
			myRaiseAmount = pot* 0.6;
			return true;
		}	
	}
	if (myTurnHandSimulation.winRanged > 0.9 && myTurnHandSimulation.win > 0.9 && nbRaises < 4){
		myRaiseAmount = pot * 0.6;
		return true;
	}

	return false;
}


bool ManiacPlayer::riverShouldBet(){

	const int pot = currentHand->getBoard()->getPot() + currentHand->getBoard()->getSets();
	PlayerList runningPlayers = currentHand->getRunningPlayerList();
	std::vector<PlayerPosition> raisersPositions = currentHand->getRaisersPositions();
	std::vector<PlayerPosition> callersPositions = currentHand->getCallersPositions();
	const int highestSet = currentHand->getCurrentBettingRound()->getHighestSet();
	const int nbRaises = currentHand->getRiverBetsOrRaisesNumber();
	const bool bHavePosition = getHavePosition(myPosition, currentHand->getRunningPlayerList());
	
	if (nbRaises > 0)
		return false;

	// blocking bet if my chances to win are weak, but not ridiculous
	if (! bHavePosition && myRiverHandSimulation.winRanged < .7 && myRiverHandSimulation.winRanged > .4 && myRiverHandSimulation.winSd > 0.4){
		int rand = 0;
		Tools::GetRand(1, 2, 1, &rand);
		if (rand == 1){
			myBetAmount = pot * 0.33;
			return true;
		}
	}

	// bluff if no chance to win, and if I was the agressor on turn
	if (isAgressor(GAME_STATE_TURN)){

		if (myRiverHandSimulation.winRanged < .2  && 
			myRiverHandSimulation.winSd > 0.3 && 
			runningPlayers->size() < 4 && 
			getCash() >= currentHand->getBoard()->getPot() &&
			canBluff(GAME_STATE_RIVER)){ 

				int rand = 0;
				Tools::GetRand(1, 4, 1, &rand);
				if (rand == 1){
					myBetAmount = pot * 0.8;
					return true;
				}
		}
	}

	int rand = 0;
	Tools::GetRand(40, 90, 1, &rand);
	float coeff = (float)rand / (float)100;

	if ( myRiverHandSimulation.winSd > .9 || (bHavePosition && myRiverHandSimulation.winSd > .85)){ 
		int rand = 0;
		Tools::GetRand(1, 5, 1, &rand);
		if (rand != 1 || bHavePosition){
			myBetAmount = pot * coeff;
			return true;
		}
	}
	if (myRiverHandSimulation.winSd > 0.5 && (myRiverHandSimulation.winRanged > .8 || (bHavePosition && myRiverHandSimulation.winRanged > .7))){ 
		int rand = 0;
		Tools::GetRand(1, 3, 1, &rand);
		if (rand == 1 || bHavePosition){
			myBetAmount = pot * coeff;
			return true;
		}
	}
	return false;
}

bool ManiacPlayer::riverShouldCall(){

	const int nbRaises = currentHand->getRiverBetsOrRaisesNumber();
	
	if (nbRaises == 0)
		return false;

	if (myRiverHandSimulation.win > .95 && myRiverHandSimulation.winSd > 0.5){
		return true;
	}

	if (myRiverHandSimulation.winRanged * 100 < getPotOdd())
		return false;

	if (myRiverHandSimulation.winRanged < .3 && myRiverHandSimulation.winSd < 0.97){
		return false;
	}

	// if hazardous call may cost me my stack, don't call even with good odds
	if (getPotOdd() >  10 && 
		myRiverHandSimulation.winRanged < .4 && 
		myRiverHandSimulation.winSd < 0.97 &&
		currentHand->getCurrentBettingRound()->getHighestSet() >= myCash + mySet &&
		getM() > 8){ 

		return false;
	}
    return true;
}

bool ManiacPlayer::riverShouldRaise(){

	const int pot = currentHand->getBoard()->getPot() + currentHand->getBoard()->getSets();
	PlayerList runningPlayers = currentHand->getRunningPlayerList();
	std::vector<PlayerPosition> raisersPositions = currentHand->getRaisersPositions();
	std::vector<PlayerPosition> callersPositions = currentHand->getCallersPositions();
	const int highestSet = currentHand->getCurrentBettingRound()->getHighestSet();
	const int nbRaises = currentHand->getRiverBetsOrRaisesNumber();
	const bool bHavePosition = getHavePosition(myPosition, currentHand->getRunningPlayerList());

	if (nbRaises == 0)
		return false;

	//  raise if i have the nuts. TODO : analyze previous actions, and determine if we must bet for value, without the nuts
	if (nbRaises < 3 && myRiverHandSimulation.winRanged > .98 && myRiverHandSimulation.winSd > 0.5){

		myRaiseAmount = pot;
		return true;
	}

	if (nbRaises < 2 && myRiverHandSimulation.winRanged * 100 > getPotOdd() && myRiverHandSimulation.winRanged > 0.9){

		myRaiseAmount = pot * 0.6;
		return true;
	}

	return false;
}


