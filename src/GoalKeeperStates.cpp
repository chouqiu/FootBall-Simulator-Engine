#include "GoalKeeperStates.h"
#include "Debug/DebugConsole.h"
#include "SoccerPitch.h"
#include "PlayerBase.h"
#include "GoalKeeper.h"
#include "SteeringBehaviors.h"
#include "SoccerTeam.h"
#include "Goal.h"
#include "2D/geometry.h"
#include "FieldPlayer.h"
#include "ParamLoader.h"
#include "Messaging/Telegram.h"
#include "Messaging/MessageDispatcher.h"
#include "SoccerMessages.h"


//uncomment to send state info to debug window
#define GOALY_STATE_INFO_ON


//--------------------------- GlobalKeeperState -------------------------------
//-----------------------------------------------------------------------------

GlobalKeeperState* GlobalKeeperState::Instance()
{
  static GlobalKeeperState instance;

  return &instance;
}

void GlobalKeeperState::Execute(GoalKeeper *keeper)
{
  // game off, just fall back...
  /*
  if (FALSE == keeper->Pitch()->GameOn())
  {
    Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
                              keeper->ID(),
                              keeper->ID(),
                              Msg_GoHome,
                              NULL);
  }
  */
}


bool GlobalKeeperState::OnMessage(GoalKeeper* keeper, const Telegram& telegram)
{
  switch(telegram.Msg)
  {
    case Msg_GoHome:
    {
      keeper->SetDefaultHomeRegion();
      
      keeper->GetFSM()->ChangeState(ReturnHome::Instance());
    }

    break;

    case Msg_ReceiveBall:
      {
        keeper->GetFSM()->ChangeState(InterceptBall::Instance());
      }

      break;

  }//end switch

  return false;
}


//--------------------------- TendGoal -----------------------------------
//
//  This is the main state for the goalkeeper. When in this state he will
//  move left to right across the goalmouth using the 'interpose' steering
//  behavior to put himself between the ball and the back of the net.
//
//  If the ball comes within the 'goalkeeper range' he moves out of the
//  goalmouth to attempt to intercept it. (see next state)
//------------------------------------------------------------------------

TendGoal* TendGoal::Instance()
{
  static TendGoal instance;

  return &instance;
}


void TendGoal::Enter(GoalKeeper* keeper)
{
  //turn interpose on
  keeper->Steering()->InterposeOn(Prm.GoalKeeperTendingDistance);

  //interpose will position the agent between the ball position and a target
  //position situated along the goal mouth. This call sets the target
  keeper->Steering()->SetTarget(keeper->GetRearInterposeTarget());
}

void TendGoal::Execute(GoalKeeper* keeper)
{
  //the rear interpose target will change as the ball's position changes
  //so it must be updated each update-step 
  keeper->Steering()->SetTarget(keeper->GetRearInterposeTarget());

  //if the ball comes in range the keeper traps it and then changes state
  //to put the ball back in play
  if (keeper->BallWithinKeeperRange())
  {
    keeper->Ball()->Trap();

    keeper->Pitch()->SetGoalKeeperHasBall(true);

    keeper->GetFSM()->ChangeState(PutBallBackInPlay::Instance());

    return;
  }

  //if ball is within a predefined distance, the keeper moves out from
  //position to try and intercept it.
  if (keeper->BallWithinRangeForIntercept() && (!keeper->Team()->InControl() || keeper->isClosestPlayerOnPitchToBall()))
  {
    keeper->GetFSM()->ChangeState(InterceptBall::Instance());
  }

  //if the keeper has ventured too far away from the goal-line and there
  //is no threat from the opponents he should move back towards it
  if (keeper->TooFarFromGoalMouth() && keeper->Team()->InControl())
  {
    keeper->GetFSM()->ChangeState(ReturnHome::Instance());

    return;
  }
}


void TendGoal::Exit(GoalKeeper* keeper)
{
  keeper->Steering()->InterposeOff();
}


//------------------------- ReturnHome: ----------------------------------
//
//  In this state the goalkeeper simply returns back to the center of
//  the goal region before changing state back to TendGoal
//------------------------------------------------------------------------

ReturnHome* ReturnHome::Instance()
{
  static ReturnHome instance;

  return &instance;
}


void ReturnHome::Enter(GoalKeeper* keeper)
{
  keeper->Steering()->ArriveOn();
}

void ReturnHome::Execute(GoalKeeper* keeper)
{
  keeper->Steering()->SetTarget(keeper->HomeRegion()->Center());

  //if close enough to home or the opponents get control over the ball,
  //change state to tend goal
  if (keeper->InHomeRegion() || !keeper->Team()->InControl())
  {
    keeper->GetFSM()->ChangeState(TendGoal::Instance());
  }
}

void ReturnHome::Exit(GoalKeeper* keeper)
{
  keeper->Steering()->ArriveOff();
}



//----------------- InterceptBall ----------------------------------------
//
//  In this state the GP will attempt to intercept the ball using the
//  pursuit steering behavior, but he only does so so long as he remains
//  within his home region.
//------------------------------------------------------------------------

InterceptBall* InterceptBall::Instance()
{
  static InterceptBall instance;

  return &instance;
}


void InterceptBall::Enter(GoalKeeper* keeper)
{
  keeper->Steering()->PursuitOn();  

  #ifdef GOALY_STATE_INFO_ON
  debug_con << "Goaly " << keeper->ID() << " enters InterceptBall" <<  "";
  #endif
}

void InterceptBall::Execute(GoalKeeper* keeper)
{ 

	//if the ball becomes in range of the goalkeeper's hands he traps the 
	//ball and puts it back in play
	if (keeper->BallWithinKeeperRange())
	{
		keeper->Ball()->Trap();

		keeper->Pitch()->SetGoalKeeperHasBall(true);

		// keeper got the ball, stop the game now!
		// useless...
		//keeper->Pitch()->SetGameOff();

		keeper->GetFSM()->ChangeState(PutBallBackInPlay::Instance());

		return;
	}

  //if the goalkeeper moves to far away from the goal he should return to his
  //home region UNLESS he is the closest player to the ball, in which case,
  //he should keep trying to intercept it.
  //though, when player pass ball back to keeper, but keeper is not set receiver,
  //that's will be ok.
  if (keeper->TooFarFromGoalMouth() && !keeper->isClosestPlayerOnPitchToBall())
  {
    #ifdef GOALY_STATE_INFO_ON
    debug_con << "Goaly " << keeper->ID() << " stop intercept and turn back" << "";
    #endif

    keeper->GetFSM()->ChangeState(ReturnHome::Instance());

    return;
  }
}

void InterceptBall::Exit(GoalKeeper* keeper)
{
  keeper->Steering()->PursuitOff();
}



//--------------------------- PutBallBackInPlay --------------------------
//
//------------------------------------------------------------------------

PutBallBackInPlay* PutBallBackInPlay::Instance()
{
  static PutBallBackInPlay instance;

  return &instance;
}

void PutBallBackInPlay::Enter(GoalKeeper* keeper)
{
  //let the team know that the keeper is in control
  keeper->Team()->SetControllingPlayer(keeper);

  //send all the players home
  //keeper->Team()->Opponents()->ReturnAllFieldPlayersToHome();
  //keeper->Team()->ReturnAllFieldPlayersToHome();
  keeper->Team()->ChangeToAttacking();
  keeper->Team()->Opponents()->ChangeToDefending();
  keeper->Team()->ReturnAllFieldPlayersToHome(FALSE);
  keeper->Team()->Opponents()->ReturnAllFieldPlayersToHome(FALSE);

  // stop the game when keeper get the ball
  keeper->Pitch()->SetGameOff();
}


void PutBallBackInPlay::Execute(GoalKeeper* keeper)
{
  PlayerBase*  receiver = NULL;
  Vector2D     BallTarget;
    
  //test if there are players further forward on the field we might
  //be able to pass to. If so, make a pass.
  if (keeper->Team()->FindPass(keeper,
                              receiver,
                              BallTarget,
                              Prm.MaxPassingForce,
                              Prm.GoalkeeperMinPassDist))
  {
    //make the pass   
    keeper->Ball()->Kick(Vec2DNormalize(BallTarget - keeper->Ball()->Pos()),
                         Prm.MaxPassingForce);

    //goalkeeper no longer has ball 
    keeper->Pitch()->SetGoalKeeperHasBall(false);

    //let the receiving player know the ball's comin' at him
    Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
                          keeper->ID(),
                          receiver->ID(),
                          Msg_ReceiveBall,
                          &BallTarget);

    //go back to tending the goal   
    keeper->GetFSM()->ChangeState(TendGoal::Instance());

    return;
  }
  // if all players at home, the keeper won't find a good pass, then, just kick it!
  else if(keeper->Team()->AllPlayersAtHome() && keeper->Team()->Opponents()->AllPlayersAtHome())
  {
    //add some noise to the kick
    Vector2D BallTarget = AddNoiseToKick(keeper->Ball()->Pos(), keeper->Team()->OpponentsGoal()->Center());

    Vector2D KickDirection = BallTarget - keeper->Ball()->Pos();

    keeper->Ball()->Kick(KickDirection, Prm.MaxShootingForce);
    #ifdef GOALY_STATE_INFO_ON
    debug_con << "Goaly " << keeper->ID() << " kick the ball to the opponents goal!" <<  "";
    #endif

    //go back to tending the goal   
    keeper->GetFSM()->ChangeState(TendGoal::Instance());

    return;
  }

  keeper->SetVelocity(Vector2D());
}

void PutBallBackInPlay::Exit(GoalKeeper* keeper)
{
	// restart the game
	keeper->Pitch()->SetGameOn();
}
