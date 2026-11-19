def _GetMovementDistanceOrDefault(targetID, targetRange, action, **kwargs):
    if targetRange is None:
        return GetDefaultDist(action, targetID, **kwargs)
    return targetRange
