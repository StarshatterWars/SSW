def GetTotalNumSlots(self):
    return sum([len(group) for group in self.slotsByGroups.itervalues()])
