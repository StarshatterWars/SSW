#pragma once
#include "Game.h"
// Only the campaign scene's owned fallback tick uses this scope. The menu stays
// paused outside it; native docking, collision and damage see a real frame inside.
class FScopedCinematicSimulationStep
{
 bool WasPaused; double PreviousSeconds; uint32 PreviousRate;
public:
 explicit FScopedCinematicSimulationStep(double Seconds)
  : WasPaused(Game::Paused()),PreviousSeconds(Game::FrameTime()),PreviousRate(Game::GetSimulationRate())
 { Game::SetPausedState(false); Game::SetSimulationStep(Seconds,1); }
 ~FScopedCinematicSimulationStep()
 { Game::SetSimulationStep(PreviousSeconds,PreviousRate); Game::SetPausedState(WasPaused); }
 FScopedCinematicSimulationStep(const FScopedCinematicSimulationStep&)=delete;
 FScopedCinematicSimulationStep& operator=(const FScopedCinematicSimulationStep&)=delete;
};
