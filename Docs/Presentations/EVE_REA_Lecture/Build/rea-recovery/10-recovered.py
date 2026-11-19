def IsValidArc(self, epicArcID):
    arcRestartInterval = get_epic_arc_restart_interval(epicArcID)
    return arcRestartInterval > 1
