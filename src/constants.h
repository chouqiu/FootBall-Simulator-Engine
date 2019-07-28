#ifndef CONSTANTS_H
#define CONSTANTS_H

// standard football field is: 105m x 68m
const int WindowWidth  = 900;
const int WindowHeight = 600;


//defines the size of a team -- do not adjust
const int TeamSize = 6;

struct FieldConst {
	enum player_role { goal_keeper, attacker, defender };

	static const int NumRegionsHorizontal = 7;
	static const int NumRegionsVertical = 3;

	static const int iMaxDistFromHomeRegion = 8;

	static const double MinTurnRadius()
	{
		return  WindowWidth / 10.0;
	}

	static const double MinBallSpeed()
	{
		return 2.0;
	}

	static const player_role* const DefaultBlueTeam()
	{
		//static const int regions[TeamSize] = { 1,6,8,3,5 };
		//static const int regions[TeamSize][2] = { {1,goal_keeper},{6,attacker},{8,attacker},
		//											{3,defender},{5,defender} };
		static const player_role team[TeamSize] = {goal_keeper, attacker, attacker, attacker, defender, defender};
		return team;
	}

	static const player_role* const DefaultRedTeam()
	{
		//static const int regions[TeamSize] = { 16,9,11,12,14 };
		//static const int regions[TeamSize][2] = { {19,goal_keeper},{12,attacker},{14,attacker},
		//											{15,defender},{17,defender} };
		static const player_role team[TeamSize] = {goal_keeper, attacker, attacker, defender, defender, defender};
		return team;
	}

	static const int* const BlueDefaultRegions()
	{
		static const int regions[TeamSize] = { 1,6,8,7,3,5 };
		return regions;
	}

	static const int* const RedDefaultRegions()
	{
		static const int regions[TeamSize] = { 19,12,14,13,15,17 };
		return regions;
	}
	
	static const int* const BlueAttackingRegions()
	{
		static const int regions[TeamSize] = { 1,15,14,17,9,4 };
		return regions;
	}
	//const int RedRegions[TeamSize] = {16,3,5,9,13};
	static const int* const RedAttackingRegions()
	{
		static const int regions[TeamSize] = { 19,3,5,10,15,17 };
		return regions;
	}

	static const int* const BlueDefendingRegions()
	{
		static const int regions[TeamSize] = { 1,16,10,5,3,4 };//{ 1,6,8,3,5 };
		return regions;
	}
    //const int RedRegions[TeamSize] = {16,9,11,12,14};
	static const int* const RedDefendingRegions()
	{
		static const int regions[TeamSize] = { 19,7,10,12,16,14 };
		return regions;
	}
};

#endif