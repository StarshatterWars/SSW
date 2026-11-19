def ReturnToDroneBay(droneIDs):
    entity = eveMoniker.GetEntityAccess()
    if entity:
        ret = entity.CmdReturnBay(droneIDs)
        HandleMultipleCallError(droneIDs, ret, 'MultiDroneCmdResult')
    return
