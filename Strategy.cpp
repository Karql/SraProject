// Strategy.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "Strategy.h"
#include "winDebugger/Client.h"

#include <math.h>
#include <string>

using namespace std;

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

typedef struct {
	double a, b;
} SlopeLineFun;


const double PI = 3.1415923;
char myMessage[200]; //big enough???

// Example debug
//char debug[250];
//sprintf(debug, "rot: %lf, x: %lf, y: %lf", rRot, rPos->x, rPos->y);
//Client::debugClient->SendMessages(debug);

void PredictBall(Environment* env);
void NearBound2(Robot* robot, double vl, double vr);
void Attack2(Robot* robot, Environment* env);

// by moon at 9/2/2002
void MoonAttack(Robot* robot, Environment* env);
// just for testing to check whether the &env->opponent works or not
void MoonFollowOpponent(Robot* robot, OpponentRobot* opponent);
void Velocity(Robot* robot, int vl, int vr);
void Angle(Robot* robot, int desired_angle);

enum SIDE {
	BLUE = 1,
	YELLOW = -1,
	NONE = 0
};

Vector3D CENTER;
double FLENGHT;

SIDE myTeam = NONE;
Vector3D LEN_POS;
double PENALTY_LINE = 0;
double GOAL_KEEPER_LINE = 0;

Robot* availableRobots[5];
OpponentRobot* nearestOpponentRobots[5];

// strategy related
void PrepareStrategy(Environment* env);
void NotDisturbStrategy(Environment* env);
void ChoseStrategy(Environment* env);
void Obrona(Environment* env);
void Atak(Environment* env);
void Bramkarz(Environment* env);
void Wybijacz(Environment* env);
void Kryjacy(Environment* env);
void Len(Environment* env);
void Kazior(Environment* env);
void Naparzacz(Environment* env);
void ObroncaL(Environment* env);
void ObroncaH(Environment* env);
void Obronca(Environment* env, Vector3D* defendPoint);

// helpers
void Init(Environment* env);
void SetTeam(Environment* env);
SIDE WhichSide(Vector3D* position);
void ResetRobots(Environment* env);
void SetAvailableRobotAsAssigned(Robot* r);
void SetNearestOpponentRobotAsAssigned(OpponentRobot* or);
double Length(Vector3D* p1, Vector3D* p2);
SlopeLineFun GetSlopeLineFun(Vector3D* p1, Vector3D* p2);

void Position(Robot* robot, Vector3D* pos);
void Position(Robot* robot, double x, double y);
void MyDefend(Environment* env, Robot* robot, Vector3D* defendPoint, double offSet);
void Shoot(Environment* env, Robot* robot, Vector3D* aim);


extern "C" STRATEGY_API void Create(Environment* env)
{
	// allocate user data and assign to env->userData
	// eg. env->userData = ( void * ) new MyVariables ();
}

extern "C" STRATEGY_API void Destroy(Environment* env)
{
	// free any user data created in Create ( Environment * )

	// eg. if ( env->userData != NULL ) delete ( MyVariables * ) env->userData;
}

extern "C" STRATEGY_API void Strategy(Environment* env)
{
	// not supported env->gameState

	Init(env);

	// reset robots
	ResetRobots(env);

	//NotDisturbStrategy(env);

	// chose atack or defense
	ChoseStrategy(env);
}

bool isInitialized = false;

void Init(Environment* env)
{
	if (!isInitialized) {
		isInitialized = true;

		// Points etc.
		CENTER.x = (GLEFT + GRIGHT) / 2.0;
		CENTER.y = (FBOT + FTOP) / 2.0;

		FLENGHT = FRIGHTX - FLEFTX;

		PrepareStrategy(env);
	}
}

void PrepareStrategy(Environment* env) 
{
	SetTeam(env);
	
	// Strategy fields etc.
	LEN_POS.x = CENTER.x + (myTeam * (CENTER.x - 40));
	LEN_POS.y = CENTER.y;

	PENALTY_LINE = CENTER.x + (myTeam * (CENTER.x - 21.5));
	GOAL_KEEPER_LINE = CENTER.x + (myTeam * (CENTER.x - 10.6663));
}

void SetTeam(Environment* env)
{
	if (myTeam == NONE) {
		myTeam = WhichSide(&env->home[0].pos);
	}
}

SIDE WhichSide(Vector3D* position)
{
	return (position->x < CENTER.x) ? YELLOW : BLUE;
}

void ResetRobots(Environment* env)
{
	for (int i = 0; i < 5; ++i) {
		availableRobots[i] = &env->home[i];
		nearestOpponentRobots[i] = &env->opponent[i];
	}
}

void SetAvailableRobotAsAssigned(Robot* r) {
	for (int i = 0; i < 5; ++i) {
		if (availableRobots[i] == r) {
			availableRobots[i] = NULL;
		}
	}
}

void SetNearestOpponentRobotAsAssigned(OpponentRobot* or) {
	for (int i = 0; i < 5; ++i) {
		if (nearestOpponentRobots[i] == or) {
			nearestOpponentRobots[i] = NULL;
		}
	}
}

/// For test other strategies
void NotDisturbStrategy(Environment* env)
{
	for (int i = 0; i < 5; ++i) {
		Position(&env->home[i], env->home[i].pos.x, env->home[i].pos.y >= CENTER.x ? FTOP : FBOT);
	}
}

void ChoseStrategy(Environment* env)
{
	// little offset
	if ((myTeam * (env->currentBall.pos.x - CENTER.x)) >= 0.5) {
		Obrona(env);
	}

	else {
		Atak(env);
	}
}

void Obrona(Environment* env)
{
	Bramkarz(env);
	Wybijacz(env);
	Kryjacy(env);
	Kryjacy(env);
	Len(env);
}

void Atak(Environment* env)
{
	Bramkarz(env);
	Kazior(env);
	Naparzacz(env);
	ObroncaL(env);
	ObroncaH(env);
}

void Bramkarz(Environment* env)
{
	Vector3D defendPoint;
	defendPoint.x = GOAL_KEEPER_LINE;
	defendPoint.y = CENTER.y;

	MyDefend(env, availableRobots[0], &defendPoint, 5);
	availableRobots[0] = NULL;	
}

void Wybijacz(Environment* env)
{
	double beforeLenght = 0;
	double closetLenght = 0;
	Robot* beforeR = NULL;
	Robot* closetR = NULL;

	Vector3D* bPos = &env->currentBall.pos;

	for (int i = 0; i < 5; ++i)
	{
		Robot* r = availableRobots[i];

		if (!r) {
			continue; // robot already assigned
		}

		Vector3D* rPos = &r->pos;
		double l = Length(rPos, bPos);
		
		// before the ball
		if ((rPos->x - bPos->x) * myTeam > 0) {		
			if (!beforeR || l < beforeLenght)
			{
				beforeR = r;
				beforeLenght = l;
			}
		}

		if (!closetR || l < closetLenght)
		{
			closetR = r;
			closetLenght = l;
		}
	}

	// wybijacz before the ball found
	if (beforeR) {
		SetAvailableRobotAsAssigned(beforeR);
		Position(beforeR, bPos);
	}

	else {
		SetAvailableRobotAsAssigned(closetR);
		Position(closetR, CENTER.x + myTeam * (FLENGHT / 2.0), closetR->pos.y);
		// TODO Improvments:
		// - ommit ball up & down to not kick suicide goal
	}
}

void Kryjacy(Environment* env)
{
	Robot* tempR = NULL;
	OpponentRobot* tempOR = NULL;
	double tempLength = 0;

	// search for the robot closet to the own goal line
	for (int i = 0; i < 5; ++i) {
		Robot* r = availableRobots[i];

		if (!r) {
			continue; // robot already assigned
		}

		double l = fabs(CENTER.x - r->pos.x);

		if (!tempR || l > tempLength) {
			tempLength = l;
			tempR = r;
		}
	}

	// search for the nearest opponent robot
	tempLength = 0;
	for (int i = 0; i < 5; ++i) {
		OpponentRobot* or = nearestOpponentRobots[i];

		if (!or ) {
			continue;
		}

		double l = Length(&tempR->pos, &or->pos);

		if (!tempOR || l < tempLength) {
			tempLength = l;
			tempOR = or;
		}
	}

	SetAvailableRobotAsAssigned(tempR);
	SetNearestOpponentRobotAsAssigned(tempOR);

	MoonFollowOpponent(tempR, tempOR);
}

void Len(Environment* env)
{
	for (int i = 1; i < 5; ++i) // without bramkarz
	{
		Robot* r = availableRobots[i];

		if (r) {
			Position(r, &LEN_POS);
		}
	}
}

void Kazior(Environment* env)
{
	Robot* tempR = NULL;
	double tempLength = 0;

	double defendLine = CENTER.x + (myTeam * (CENTER.x - 30.0)); // defend line

	for (int i = 0; i < 5; ++i) {
		Robot* r = availableRobots[i];

		if (!r) {
			continue; // robot already assigned
		}

		if (myTeam * (r->pos.x - defendLine) > 0) {
			continue; // do not touch Obronca*
		}

		double l = Length(&r->pos, &LEN_POS);

		if (!tempR || l < tempLength) {
			tempLength = l;
			tempR = r;
		}
	}

	SetAvailableRobotAsAssigned(tempR);

	// check if ball is on convenient position
	Vector3D* bPos = &env->currentBall.pos;
	if (bPos->y <= GTOPY + 2.0
		&& bPos->y >= GBOTY - 2.0
		&& myTeam * (CENTER.x- bPos->x) >= 0 )
	{
		Vector3D aim;
		aim.x = CENTER.x - (myTeam * (CENTER.x - FLEFTX));
		aim.y = CENTER.y;

		Shoot(env, tempR, &aim);
	}

	else
	{
		Position(tempR, &LEN_POS);
	}
}

void Naparzacz(Environment* env)
{
	Robot* tempR = NULL;
	double tempLength = 0;
	Vector3D* bPos = &env->currentBall.pos;

	for (int i = 0; i < 5; ++i) {
		Robot* r = availableRobots[i];

		if (!r) {
			continue; // robot already assigned
		}

		double l = Length(&r->pos, bPos);

		if (!tempR || l < tempLength) {
			tempLength = l;
			tempR = r;
		}
	}

	SetAvailableRobotAsAssigned(tempR);

	Vector3D aim;
	aim.x = CENTER.x - (myTeam * (CENTER.x - FLEFTX));
	aim.y = CENTER.y;

	Shoot(env, tempR, &aim);	
}

void ObroncaL(Environment* env)
{
	Vector3D defendPoint;
	defendPoint.x = PENALTY_LINE;
	defendPoint.y = (CENTER.y - FBOT) / 2.0;

	Obronca(env, &defendPoint);
}

void ObroncaH(Environment* env)
{
	Vector3D defendPoint;
	defendPoint.x = PENALTY_LINE;
	defendPoint.y = CENTER.y + ((CENTER.y - FBOT) / 2.0);

	Obronca(env, &defendPoint);
}

void Obronca(Environment* env, Vector3D* defendPoint)
{
	Robot* tempR = NULL;
	double tempLength = 0;

	for (int i = 0; i < 5; ++i) {
		Robot* r = availableRobots[i];

		if (!r) {
			continue; // robot already assigned
		}

		double l = Length(&r->pos, defendPoint);

		if (!tempR || l < tempLength) {
			tempLength = l;
			tempR = r;
		}
	}

	SetAvailableRobotAsAssigned(tempR);

	MyDefend(env, tempR, defendPoint, 12.0);
}

/// Lenght between two points
double Length(Vector3D* p1, Vector3D* p2)
{
	return sqrt(pow(fabs(p1->x - p2->x), 2) + pow(fabs(p1->y - p2->y), 2));
}

/// Get line fun by two points
SlopeLineFun GetSlopeLineFun(Vector3D* p1, Vector3D* p2)
{
	SlopeLineFun lFun;

	lFun.a = (p1->y - p2->y) / (p1->x - p2->x);
	lFun.b = (p1->y - (((p1->y - p2->y) / (p1->x - p2->x)) * p1->x));

	return lFun;
}

void Position(Robot* robot, Vector3D* pos)
{
	Position(robot, pos->x, pos->y);
}

void Position(Robot* robot, double x, double y)
{
	int desired_angle = 0, theta_e = 0, d_angle = 0, vl, vr, vc = 70;

	double dx, dy, d_e, Ka = 10.0 / 90.0;
	dx = x - robot->pos.x;
	dy = y - robot->pos.y;

	d_e = sqrt(dx * dx + dy * dy);
	if (dx == 0 && dy == 0)
		desired_angle = 90;
	else
		desired_angle = (int)(180. / PI * atan2((double)(dy), (double)(dx)));
	theta_e = desired_angle - (int)robot->rotation;

	while (theta_e > 180) theta_e -= 360;
	while (theta_e < -180) theta_e += 360;

	if (d_e > 100.)
		Ka = 17. / 90.;
	else if (d_e > 50)
		Ka = 19. / 90.;
	else if (d_e > 30)
		Ka = 21. / 90.;
	else if (d_e > 20)
		Ka = 23. / 90.;
	else
		Ka = 25. / 90.;

	if (theta_e > 95 || theta_e < -95)
	{
		theta_e += 180;

		if (theta_e > 180)
			theta_e -= 360;
		if (theta_e > 80)
			theta_e = 80;
		if (theta_e < -80)
			theta_e = -80;
if (d_e < 5.0 && abs(theta_e) < 40)
	Ka = 0.1;
vr = (int)(-vc * (1.0 / (1.0 + exp(-3.0 * d_e)) - 0.3) + Ka * theta_e);
vl = (int)(-vc * (1.0 / (1.0 + exp(-3.0 * d_e)) - 0.3) - Ka * theta_e);
	}

	else if (theta_e < 85 && theta_e > -85)
	{
	if (d_e < 5.0 && abs(theta_e) < 40)
		Ka = 0.1;
	vr = (int)(vc * (1.0 / (1.0 + exp(-3.0 * d_e)) - 0.3) + Ka * theta_e);
	vl = (int)(vc * (1.0 / (1.0 + exp(-3.0 * d_e)) - 0.3) - Ka * theta_e);
	}

	else
	{
	vr = (int)(+.17 * theta_e);
	vl = (int)(-.17 * theta_e);
	}

	Velocity(robot, vl, vr);
	//NearBound2(robot, -vl, -vr);
}

// Go to defend point
// Align robot at 90 or 270 degree
// Oscilate on y axis following the ball in offset range
void MyDefend(Environment* env, Robot* robot, Vector3D* defendPoint, double offSet)
{
	if (fabs(defendPoint->x - robot->pos.x) < 2.0)
	{
		double low = defendPoint->y - offSet;
		double high = defendPoint->y + offSet;

		// params for tune algorith
		double initialAlignDelta = 2.5;
		double oscylation = 0.5;

		Vector3D* rPos = &robot->pos;
		Vector3D* bPos = &env->currentBall.pos;

		double rRot = robot->rotation + 180.0; // rotation range [-180,180] +180.0 to work with positiv numbers
		double angle = rRot < 180.0 ? 90.0 : 270.0; // closer for robot

		// Robot is initialy align - oscylate base on ball
		if (fabs(rRot - angle) <= initialAlignDelta)
		{
			double direction = angle == 90.0 ? -1 : 1; // up or down
			double vr = 0;
			double vl = 0;

			if (rPos->y > bPos->y + oscylation && rPos->y > low)
			{
				vr = -100;
				vl = -100;
			}

			if (rPos->y < bPos->y - oscylation && rPos->y < high)
			{
				vr = 100;
				vl = 100;
			}

			if (rPos->y > high)
			{
				vr = -100;
				vl = -100;
			}

			if (rPos->y < low)
			{
				vr = 100;
				vl = 100;
			}

			robot->velocityRight = direction * vr;
			robot->velocityLeft = direction * vl;
		}

		// Align robot
		else
		{
			double turnDirection = rRot < angle ? 1.0 : -1.0; // 1 turn left, -1 turn right

			robot->velocityRight = turnDirection * fabs(angle - rRot);
			robot->velocityLeft = -(turnDirection * fabs(angle - rRot));
		}
	}

	else
	{
		Position(robot, defendPoint);
	}
}

void Shoot(Environment* env, Robot* robot, Vector3D* aimPos)
{
	double acceptedAngel = 30.0; // degree
	double acceptedRad = acceptedAngel * (PI / 180.0);

	Vector3D* rPos = &robot->pos;
	Vector3D* bPos = &env->currentBall.pos;

	// ball - aim line
	SlopeLineFun ballAimLine = GetSlopeLineFun(bPos, aimPos);
	// robot - ball line
	SlopeLineFun robotBallLine = GetSlopeLineFun(rPos, bPos);

	double rad = PI / 2.0; // perpendicural lines by default to avoid divide by 0

	if ((1.0 + (ballAimLine.a * robotBallLine.a)) != 0)
	{
		double tgO = (robotBallLine.a - ballAimLine.a) / (1.0 + (ballAimLine.a * robotBallLine.a));
		rad = fabs(atan(tgO));
	}

	double xMultiplier = bPos->x < aimPos->x ? -1.0 : 1.0;
	double robotBallLenght = xMultiplier * (rPos->x - bPos->x);

	// shoot
	if ((robotBallLenght >= 0 && robotBallLenght < 10.0 && fabs(rPos->y - bPos->y) < 4.0)  // just befor ball
		|| rad <= acceptedRad // or in right angel
		//|| rPos->y > FTOP - 4.0 // near bound to not block
		//|| rPos->y < FBOT + 4.0 // near bound to not block
	)  
	{
		//Position(robot, bPos);  // go as far as ball 
		//MoonAttack(robot, env); // little speed up 

		double x = bPos->x - (xMultiplier * 1.0);     // 1 inch after ball should be faster
		double y = ballAimLine.a * x + ballAimLine.b; // y on ball - aim line

		Position(robot, x, y);
	}

	// go to better position for shoot
	else 
	{
		double x = bPos->x + (xMultiplier * 10.0);    // go 10 inch before ball
		double y = ballAimLine.a * x + ballAimLine.b; // y on ball - aim line

		if (y > FTOP) y = FTOP - 2.0;
		if (y < FBOT) y = FBOT + 2.0;

		Position(robot, x, y);
	}
}


//
// --
//

void MoonAttack(Robot* robot, Environment* env)
{
	//Velocity (robot, 127, 127);
	//Angle (robot, 45);
	PredictBall(env);
	Position(robot, env->predictedBall.pos.x, env->predictedBall.pos.y);
	// Position(robot, 0.0, 0.0);
}

void MoonFollowOpponent(Robot* robot, OpponentRobot* opponent)
{
	Position(robot, opponent->pos.x, opponent->pos.y);
}

void Velocity(Robot* robot, int vl, int vr)
{
	robot->velocityLeft = vl;
	robot->velocityRight = vr;
}

void PredictBall(Environment* env)
{
	double dx = env->currentBall.pos.x - env->lastBall.pos.x;
	double dy = env->currentBall.pos.y - env->lastBall.pos.y;
	env->predictedBall.pos.x = env->currentBall.pos.x + dx;
	env->predictedBall.pos.y = env->currentBall.pos.y + dy;

}

void Attack2(Robot* robot, Environment* env)
{
	Vector3D t = env->currentBall.pos;
	double r = robot->rotation;
	if (r < 0) r += 360;
	if (r > 360) r -= 360;
	double vl = 0, vr = 0;

	if (t.y > env->fieldBounds.top - 2.5) t.y = env->fieldBounds.top - 2.5;
	if (t.y < env->fieldBounds.bottom + 2.5) t.y = env->fieldBounds.bottom + 2.5;
	if (t.x > env->fieldBounds.right - 3) t.x = env->fieldBounds.right - 3;
	if (t.x < env->fieldBounds.left + 3) t.x = env->fieldBounds.left + 3;

	double dx = robot->pos.x - t.x;
	double dy = robot->pos.y - t.y;

	double dxAdjusted = dx;
	double angleToPoint = 0;

	if (fabs(robot->pos.y - t.y) > 7 || t.x > robot->pos.x)
		dxAdjusted -= 5;

	if (dxAdjusted == 0)
	{
		if (dy > 0)
			angleToPoint = 270;
		else
			angleToPoint = 90;
	}
	else if (dy == 0)
	{
		if (dxAdjusted > 0)
			angleToPoint = 360;
		else
			angleToPoint = 180;

	}
	else
		angleToPoint = atan(fabs(dy / dx)) * 180.0 / PI;

	if (dxAdjusted > 0)
	{
		if (dy > 0)
			angleToPoint -= 180;
		else if (dy < 0)
			angleToPoint = 180 - angleToPoint;
	}
	if (dxAdjusted < 0)
	{
		if (dy > 0)
			angleToPoint = -angleToPoint;
		else if (dy < 0)
			angleToPoint = 90 - angleToPoint;
	}

	if (angleToPoint < 0) angleToPoint = angleToPoint + 360;
	if (angleToPoint > 360) angleToPoint = angleToPoint - 360;
	if (angleToPoint > 360) angleToPoint = angleToPoint - 360;

	double c = r;

	double angleDiff = fabs(r - angleToPoint);

	if (angleDiff < 40)
	{
		vl = 100;
		vr = 100;
		if (c > angleToPoint)
			vl -= 10;
		if (c < angleToPoint)
			vr -= 10;
	}
	else
	{
		if (r > angleToPoint)
		{
			if (angleDiff > 180)
				vl += 360 - angleDiff;
			else
				vr += angleDiff;
		}
		if (r < angleToPoint)
		{
			if (angleDiff > 180)
				vr += 360 - angleDiff;
			else
				vl += angleDiff;
		}
	}

	NearBound2(robot, vl, vr);
}

void NearBound2(Robot* robot, double vl, double vr)
{
	//Vector3D t = env->currentBall.pos;

	Vector3D a = robot->pos;
	double r = robot->rotation;

	if (a.y > FTOP - 15 && r > 45 && r < 130)
	{
		if (vl > 0)
			vl /= 3;
		if (vr > 0)
			vr /= 3;
	}

	if (a.y < FBOT + 15 && r < -45 && r > -130)
	{
		if (vl > 0) vl /= 3;
		if (vr > 0) vr /= 3;
	}

	if (a.x > FRIGHTX - 10)
	{
		if (vl > 0)
			vl /= 2;
		if (vr > 0)
			vr /= 2;
	}

	if (a.x < FLEFTX + 10)
	{
		if (vl > 0)
			vl /= 2;
		if (vr > 0)
			vr /= 2;
	}

	robot->velocityLeft = -vl;
	robot->velocityRight = -vr;
}