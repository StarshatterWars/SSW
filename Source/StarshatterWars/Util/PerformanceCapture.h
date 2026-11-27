#pragma once
namespace SSWPerformanceCapture
{
    void Initialize();
    void Shutdown();
    void RecordEconomy(double Seconds);
    void RecordSimulation(double Seconds);
}
