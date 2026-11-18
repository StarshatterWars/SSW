#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "QuantumCameraPresentation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuantumPresentationTest, "Starshatter.Travel.CosmeticEnvelopeAndFov",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FQuantumPresentationTest::RunTest(const FString&)
{
    float Reference = 0;
    for (const int32 FPS : {30, 60, 120})
    {
        FQuantumTravelEnvelope Envelope;
        for (int32 Frame=0; Frame<FPS/2; ++Frame) Envelope.Update(1.f/FPS, EQuantumPresentationPhase::Departure);
        if (FPS==30) Reference=Envelope.Strength;
        TestTrue(TEXT("Equal elapsed time gives equal departure strength across frame rates"), FMath::IsNearlyEqual(Envelope.Strength,Reference,.0001f));
        const float Paused=Envelope.Strength;
        for(int32 Frame=0;Frame<FPS;++Frame) Envelope.Update(0,EQuantumPresentationPhase::Departure);
        TestEqual(TEXT("Pause freezes cosmetic progress"),Envelope.Strength,Paused);
        for(int32 Frame=0;Frame<FPS;++Frame) Envelope.Update(1.f/FPS,EQuantumPresentationPhase::Loading);
        TestEqual(TEXT("Loading reaches bounded peak"),Envelope.Strength,1.f);
        for(int32 Frame=0;Frame<FPS;++Frame) Envelope.Update(1.f/FPS,EQuantumPresentationPhase::Arrival);
        TestEqual(TEXT("Arrival restores zero contribution"),Envelope.Strength,0.f);
        for(const auto Phase:{EQuantumPresentationPhase::Countdown,EQuantumPresentationPhase::Departure,EQuantumPresentationPhase::Loading,EQuantumPresentationPhase::Arrival})
        {
            Envelope.Update(.3f,Phase); Envelope.Reset();
            TestEqual(TEXT("Abort cleans each phase immediately"),Envelope.Strength,0.f);
            TestTrue(TEXT("Abort returns to idle"),Envelope.Phase==EQuantumPresentationPhase::Idle);
        }
        Envelope.Update(1,EQuantumPresentationPhase::Departure);
        Envelope.Reset();
        Envelope.Update(.5f,EQuantumPresentationPhase::Departure);
        TestTrue(TEXT("Consecutive jumps restart the envelope"),FMath::IsNearlyEqual(Envelope.Strength,Reference,.0001f));
    }
    TestEqual(TEXT("Normal lens receives modest eight-degree offset"),FQuantumTravelEnvelope::TravelFov(60,1),68.f);
    TestEqual(TEXT("Wide lens stops at the presentation ceiling"),FQuantumTravelEnvelope::TravelFov(112,1),115.f);
    TestEqual(TEXT("A newer wider camera is never narrowed"),FQuantumTravelEnvelope::TravelFov(120,1),120.f);
    TestEqual(TEXT("Removing effect preserves the actual camera baseline"),FQuantumTravelEnvelope::TravelFov(73,0),73.f);
    for(int32 Frame=0;Frame<120;++Frame)
        TestEqual(TEXT("Fresh per-frame POV does not compound FOV"),FQuantumTravelEnvelope::TravelFov(60,.5f),64.f);
    return true;
}
#endif
