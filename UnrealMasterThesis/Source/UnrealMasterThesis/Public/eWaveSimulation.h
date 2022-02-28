#pragma once

/*
	State variables used for the eWave simulation.
	Av array of submerged triangle vertices for the obstruction map computation.
	Boat position and delta position variables for moving the simulation and sampling UVs.
	Boat speed to scale the wakes.
	Scale to multiply by in the scale shader pass after the iFFT
*/

struct eWaveSimulation {
	TArray<FVector4> submerged;

	float boatX = 0.0f;
	float boatY = 0.0f;
	float boatSpeed = 0.0f;
	float boatPrevX = 0.0f;
	float boatPrevY = 0.0f;
	int boatPrevXp = 0;
	int boatPrevYp = 0;
	float uvX = 0.0f;
	float uvY = 0.0f;
	float cmPerPixel;
	int boatXp;
	int boatYp;
	int dxp;
	int dyp;
	float scale;
	bool first = true;
};
