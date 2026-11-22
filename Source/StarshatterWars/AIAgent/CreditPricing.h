#pragma once
#include "CoreMinimal.h"

namespace CreditPricing
{
    // Reference price for optional credit purchases; never a cash-out value.
    int64 CreditsPerUsd();
    double UsdEquivalent(int64 Credits);
}
