def SetFilter(self, filterMethod):
    self._filterMethod = filterMethod
    self._UpdateSortedAndFiltered()
    return
